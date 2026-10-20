# Flight software (C++ / F´)

The mission FSW is a real F´ 4.4 project at
`fprime/cubesat-fsw/`, with two project components and two libraries.

```
fsw/
├── Makefile                      # host tests (power + LittleFS)
├── tests/
│   ├── test_power.cpp            # 87 checks
│   └── test_storage.cpp          # 59 checks
└── fprime/cubesat-fsw/
    ├── Cubesat/
    │   ├── PowerCore/            # battery, mode FSM, CCSDS header
    │   ├── Storage/              # LittleFS 2.11.2 + FileStore (RAM / SPI NOR)
    │   └── Components/
    │       ├── PowerManager/     # passive, gates the PA
    │       ├── DataStore/        # passive, appends hk.log
    │       └── RadioDriver/      # PA enable consumer
    └── CubesatDeployment/        # topology, 1 Hz rate group
```

## Run the tests

```bash
make -C fsw          # builds and runs both suites, ~1 s
make fprime-ut       # 11 F´ component tests via fprime-util check
```

They are dependency-free (no gtest, no heap, no exceptions) and run on the
host. Coverage:

- energy conversion, SOC clamping, reserve floor, saturation, atomic failed
  discharges, negative inputs;
- the **complete 4 modes × 7 events transition matrix** (28 cases), PA gate,
  redundant events, fault stickiness;
- a four-orbit simulation at 1 Hz charging and a 300 s burst per orbit,
  asserting the 20 % reserve is never crossed;
- CCSDS header round-trips over APID/sequence sweeps, length-field edges
  (1, 256, 65535), bit masking and short buffers;
- LittleFS persistence across remount, brown-out mid-write, append across
  mounts, overwrite, multi-file isolation, 4 KiB multi-block files, device
  full, corrupted-superblock fail-closed, and wear-leveling erases.

The mission test caught a real bug: `charge(1500 mW, 1 s)` truncates to
0 mWh. Accounting is now in milliwatt-seconds internally (`Battery::stored_mws`),
so a 1 Hz rate group charges correctly.

The F´ unit tests caught two more: `PowerManager` booted into `Safe` with no
way to clear the fault (added `clearFault`), and it published `Downlink`
before checking the energy guard (it now checks first and never emits a mode
it cannot sustain).

## Build / run the F´ deployment

```bash
make fprime        # warning: first build is ~900 targets
make fprime-run    # boots and shows events for 5 s
make fprime-gds    # browser GDS at http://localhost:5000
```

`make fprime-gds` launches the deployment under the F´ GDS with the generated
dictionary; open `http://localhost:5000` for telemetry plots and event log.
To run it by hand:

```bash
cd fsw/fprime/cubesat-fsw
# bash / zsh
source ~/Projects/embedded/.venv-fprime/bin/activate
# fish:
# source ~/Projects/embedded/.venv-fprime/bin/activate.fish
fprime-gds -d build-artifacts/Linux/CubesatDeployment --skip-browser-open
```

The build needs the venv:

```bash
# bash / zsh
source ~/Projects/embedded/.venv-fprime/bin/activate

# fish (bash's activate script is not valid fish syntax)
source ~/Projects/embedded/.venv-fprime/bin/activate.fish
```

`CMake 3.31` is pinned in that venv because F´ 4.4's install step fails under
CMake 4.x (`file INSTALL given no DESTINATION` for the root `hashes.txt`).
The project sets `CMAKE_CXX_STANDARD 20`; F´ itself defaults to C++14.

Expected boot line:

```
EVENT: ... ACTIVITY_HI: (CubesatDeployment.dataStore) StorageReady : LittleFS mounted
```

### Topology wiring

| Instance | Type | Connection |
|---|---|---|
| `powerManager` | `Cubesat.PowerManager` (passive) | `rateGroup_1Hz.RateGroupMemberOut[6]` |
| `dataStore` | `Cubesat.DataStore` (passive) | `rateGroup_1Hz.RateGroupMemberOut[7]` |
| `radioDriver` | `Cubesat.RadioDriver` (passive) | `powerManager.paEnable -> radioDriver.paEnableIn` |

Telemetry and events use the deployment's pattern connections
(`tlmSend`, `events`, `textLogger`, `chronoTime`), so no extra plumbing.
`powerManager.paEnable` is an output port; leave it for the real radio driver
and never drive it from telemetry.

### PowerManager

Wraps `cubesat::Supervisor` + `cubesat::Battery`; no F´ dependency leaks into
the policy. On entering `Downlink` it checks `discharge(13 W, 300 s)` **before**
keying the PA; if the guard trips it latches a fault and stays off.

### DataStore

Mounts `LittleFS` lazily on the first 1 Hz tick, then appends an 8-byte
housekeeping record to `hk.log`, exposing `Records`, `LastBytes`, `Mounted`.

### Storage backends

`FileStore` has two constructors:

- `FileStore(RamBlock&)` — host/QEMU, 512-byte blocks;
- `FileStore(SpiNorDevice&)` — flight. Fill `SpiNorOps` with the transport
  (`read`, `program`, `erase_sector`) and the real geometry
  (`sector_size`, `sector_count`, `page_size`). LittleFS sectors map 1:1 to
  NOR sectors, `prog_size` to the page size.

The SPI NOR path is unit-tested against a simulated part that enforces NOR
bit semantics (program can only clear bits) and 256-byte page boundaries; the
suite asserts zero violations and multiple sector erases under sustained
appends.

### RadioDriver

Consumes `powerManager.paEnable` and records the real RF state (`PaState`,
`KeyCount`, `PaKeyed` / `PaInhibited`). Key-ups are idempotent, so a repeated
gate value does not double count.

LittleFS is vendored (`Storage/third_party/littlefs`, BSD-1-Clause) so the
flight build needs no network. `FileStore` formats only blank media and
fails closed on a corrupt filesystem rather than erasing the only copy of the
telemetry.

## Rust boundary

Rust is used only for `Svc::WasmSequencer` sequences (Rust → Wasm, sandboxed).
Components and topology remain C++/autocoded. The host-side Rust tree at
`../../rust/` is a reference model, not flight code.

## PAC / register access

There is no public VA41620 SVD. Generate a PAC from the vendor DFP with
`../../tools/gen-pac.sh`; the C++ build can keep using vendor headers.
The memory map in `../../rust/firmware-va41620/linker.ld` is a placeholder.
