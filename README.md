# embedded — F´/C++ flight software sandbox

Headless toolchain for the 1U CubeSat: F´ components in C++, emulators driven
from the shell, VCD waveforms, GDB on `localhost:1234`. KiCad stays separate.

**Language policy** (`AGENTS.md`): flight components are **C++20 / F´**,
board and vendor code is **C17**, Rust is only for `Svc::WasmSequencer`
sequences and host-side reference models. Both C and C++ build with
warnings-as-errors and a no-heap rule.

```
embedded/
├── fsw/                      # F´ 4.4 flight software + host tests
│   ├── tests/{test_power,test_storage}.cpp
│   └── fprime/cubesat-fsw/
│       ├── Cubesat/{PowerCore,Storage,Components}
│       └── CubesatDeployment     # topology + 1 Hz rate group
├── avr/                      # atmega328p C17 demo + simavr VCD harness
├── riscv/                    # bare-metal rv32 C17 demo (QEMU virt)
├── rust/                     # host reference model + QEMU sandbox
├── cad/                      # OpenSCAD 1U packaging model
├── tools/gen-pac.sh          # VA41620 PAC from the vendor SVD
├── setup/{install,install-fprime,verify}.sh
└── esp32/README.md
```

## Install

```bash
cd ~/Projects/embedded
./setup/install.sh         # system packages (sudo)
./setup/install-fprime.sh  # F´ tooling venv (no sudo)
./setup/verify.sh          # 20/20 passing on this machine
```

## Verify

```bash
cd ~/Projects/embedded
./setup/verify.sh      # 20/20 passing on this machine
```

Installed: `avr-gcc 16`, `avr-gdb`, `avrdude`, `simavr 1.8`, QEMU 11
(`avr`, `riscv32`, `xtensa`), `gtkwave`, `riscv64-elf-gcc`, Rust 1.97 with
`thumbv7em-none-eabihf` + `riscv32imac/imc-none-elf`.

Shell note: the F´ venv has both activate scripts. **fish** cannot parse
bash's `activate`, so use:

```fish
source ~/Projects/embedded/.venv-fprime/bin/activate.fish
```

For bash/zsh use `bin/activate` instead. The Makefile sources it under
`/bin/bash`, so `make` targets work from either shell.

## Flight software (C++ / F´)

```bash
make fsw           # 146 host assertions across two suites
make fprime        # build the F´ deployment (PowerManager + DataStore)
make fprime-ut     # 11 F´ component tests (fprime-util check + GTest)
make fprime-run    # boot it headlessly; LittleFS prints StorageReady
make fprime-gds    # browser GDS on http://localhost:5000
```

`fsw/fprime/cubesat-fsw` is a real F´ 4.4 project, C++20:

- **PowerCore** — milliwatt-second battery accounting with a 20 % reserve,
  the `Safe → Charge → AwaitPass → Downlink` supervisor, CCSDS TM header.
- **Storage** — vendored LittleFS 2.11.2 behind a static `FileStore`;
  formats only blank media and fails closed on corruption.
- **PowerManager** — passive component; checks `discharge(13 W, 300 s)`
  before keying the PA, latches a fault otherwise.
- **DataStore** — passive component; appends 8-byte housekeeping records to
  `hk.log` at 1 Hz.
- **CubesatDeployment** — both instances on `rateGroup_1Hz` slots 6 and 7,
  with the deployment's telemetry/event/time pattern connections.

The test suite covers the full 4×7 mode/event matrix, a four-orbit energy
simulation, CCSDS edge cases, LittleFS brown-out recovery, append across
remounts, device-full, corrupt-superblock fail-closed and wear erases. It
already caught a real defect: `charge(1500 mW, 1 s)` truncates to 0 mWh, so
accounting is now in milliwatt-seconds.

Compiled with `-std=c++20 -Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion -Werror -fno-exceptions -fno-rtti`, no heap, no virtual
dispatch in the rate-group path.

Note: the venv pins **CMake 3.31** because F´ 4.4's install step fails under
CMake 4.x. `fprime-util` needs the venv on `PATH`.

### Rust inside F´

Correcting my earlier claim: F´ does support Rust, but only through
`Svc::WasmSequencer` — sequences compile to Wasm and run isolated; the
component/topology layer stays C++ and autocoded. The build system picks up
`cargo`/`rustc` when present. That is the boundary this repo follows:
`rust/` is a host-side oracle and QEMU sandbox, never a flight component.

## GDS showcase

Start it in one terminal:

```bash
make fprime-gds      # http://localhost:5000
```

Then in another terminal, drive a whole pass:

```bash
make gds-demo
```

What the UI gives you:

- **Channels** — tick `CubesatDeployment.dataStore.Records`, `powerManager.Mode`,
  `powerManager.SocPermille`, `radioDriver.PaState` to plot them live.
- **Events** — `StorageReady`, `PaKeyed`, `PaInhibited`, `FaultLatched`, with
  severity filters.
- **Commands** — the four `powerManager.*` commands appear with their opcodes;
  the same ones the script sends.
- **Uplink / sequences** — upload a file or a compiled command sequence; the
  deployment already has `Svc.FileUplink` and `Svc.CmdSequencer` wired.

Command-line equivalents (from `fsw/fprime/cubesat-fsw`, venv active):

```bash
export D=build-artifacts/Linux/CubesatDeployment

fprime-cli command-send -d $D --list -c CubesatDeployment.powerManager
fprime-cli command-send -d $D CubesatDeployment.powerManager.CLEAR_FAULT
fprime-cli channels     -d $D -c CubesatDeployment.dataStore
fprime-cli events       -d $D -s Fault
```

The demo sequence and the expected telemetry:

| Command | Mode | What happens |
|---|---|---|
| `CLEAR_FAULT` | 1 Charge | 1 Hz harvest starts, `SocPermille` climbs |
| `PASS_SCHEDULED` | 2 AwaitPass | pass imminent, PA still inhibited |
| `PASS_ACQUIRED` | 3 Downlink | PA keyed, `PaKeyed`, 1.08 Wh reserved |
| `DOWNLINK_COMPLETE` | 1 Charge | `PaInhibited` |
| `PASS_ACQUIRED` again | 0 Safe | energy guard refuses: `FaultLatched 0xE0B1` |

That last row is the point: the battery guard is enforced in the component,
not in the UI, so the same refusal happens from the GDS, a script, or the
flight sequencer.

## Physical model (OpenSCAD)

![1U CubeSat assembly](cad/cubesat.png)

Parametric 1U envelope in `cad/cubesat.scad`: 100 mm body, 8.5 mm rails,
body-mounted solar cells, four-board stack, S-band patch and deployable UHF
whip. `cad/README.md` maps each feature to the subsystem and documents the
caveats (it is a packaging model, not structural analysis).

```bash
make cad            # cubesat.stl + cubesat-exploded.stl
make cad-preview    # assembly / exploded / stack PNGs
```

`u = 1..3` changes the envelope; `explode`, `show_panels`, `show_stack` and
`show_antenna` control the view. OpenSCAD is not installed by
`setup/install.sh` because it is optional: `sudo pacman -S openscad`, or use
the upstream AppImage.

## Emulation

```bash
make avr          # AVR build -> build/firmware.{elf,hex}
make sim          # simavr, UART to stdout (5 s cap)
make vcd          # 1.2 s of execution -> VCD
make wave         # gtkwave: PB0..PB7, PD0..PD7
make riscv        # C rv32 build (QEMU virt)
make run-riscv    # headless boot
make debug-avr    # simavr -g + avr-gdb
make debug-riscv  # qemu -S + gdb
```

AVR note: Arch's `simavr` is built **without libelf**, so it cannot read
`.elf`. `make sim`/`make vcd` use `firmware.hex`; the VCD harness links
`libsimavr` and loads Intel HEX directly. Verified transitions: PB5 at
18 ms, 525 ms, 1030 ms with a 500 ms blink.

QEMU has no VCD backend. For RISC-V use `-d in_asm,cpu -D qemu.log` or
`-trace`; gtkwave is wired for AVR only.

### ESP32 reality

Arch's `qemu-system-xtensa` ships only DC232b boards — **no `esp32`
machine** (checked with `-M help`), and `qemu-system-riscv32` has no
`esp32c3`. Emulating ESP32 needs Espressif's QEMU fork or ESP-IDF's
`idf.py qemu`; see `esp32/README.md`.

## Rust sandbox

```bash
make rust             # core tests + both images
make rust-test        # 15 host tests
make rust-riscv-run   # boots in QEMU, prints ticks
make rust-arm         # links the Cortex-M4 skeleton
```

Rust editions of the same policy (`rust/cubesat-core`) exist as an executable
spec: they are useful for cross-checking the C++ implementation, not for
flight. The VA41620 HAL question is covered by `tools/gen-pac.sh`; there is
no public SVD, so point it at the vendor DFP.

## Design-doc reality checks

- **1.5 W orbit-average from a 1U body-mounted array is optimistic**
  (measured units: ~0.6–1.2 W). At 0.8 W the 300 s burst is ~60 % of the
  harvest, not 48 %. Keep the reserve and load shedding.
- **−105 dBm sensitivity is not valid at 4.3 Mbps.** Required carrier is
  ≈ `−174 + 10log₁₀(Rb) + Eb/N0` ≈ −103 dBm before ~2 dB implementation
  loss. Your −98 dBm gives ~3 dB margin at max rate, not 7; 2 Mbps is the
  robust mode.
- **12 W DC for 33 dBm ≈ 10 W of heat** in a 10 cm cube. Duty cycling fixes
  energy, not junction temperature — model the burst thermal transient.
- **No public VA41620 SVD**; the memory map in
  `rust/firmware-va41620/linker.ld` is a placeholder, not a datasheet.
