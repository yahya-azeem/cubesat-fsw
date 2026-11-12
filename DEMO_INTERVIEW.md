# Full-project interview demo (10 minutes)

Repo: `~/Projects/showcases/embedded` (GitHub: `yahya-azeem/cubesat-fsw`).
There is also a compatibility symlink at `~/Projects/embedded`.
Everything below is local and offline. The F´ venv and build artifacts exist
in the new location.

## Before the call (one-time check)

```bash
cd ~/Projects/showcases/embedded
make fsw          # 146 host assertions
make fprime-ut    # 11 F´ component tests
make fprime-run   # boots headless, prints StorageReady
```

Do not start the GDS until the call: it opens a port.

---

## Minute 0–1: what the project is

Show only:

```bash
cd ~/Projects/showcases/embedded
cat README.md | sed -n '1,55p'
```

Say:

> “One repo, four layers: F´/C++ flight software, AVR and RISC-V demos,
> Rust host reference models, and a parametric OpenSCAD packaging model.
> The flight path is C++20: PowerCore, LittleFS storage, PowerManager,
> DataStore, and a deployment with a 1 Hz rate group.”

## Minute 1–2: policy tests (no framework)

```bash
make fsw
```

Expected tail:

```
all C++ policy checks passed
all LittleFS checks passed
```

This proves the core builds with `-fno-exceptions -fno-rtti`, warnings as
errors, and no heap.

## Minute 2–3: real F´ component tests

```bash
make fprime-ut
```

PowerManager, DataStore, RadioDriver under `fprime-util check` + GoogleTest.
Mention: **11 tests**, including the battery guard and PA history.

## Minute 3–4: boot the deployment

```bash
make fprime-run
```

Point at these lines:

```
CubesatDeployment.dataStore StorageReady : LittleFS mounted
Framework Version: v4.4.1
```

Then explain the deployment contains PowerManager + DataStore on
`rateGroup_1Hz` slots 6 and 7, with telemetry/events/commands wired.

## Minute 4–7: live GDS pass (the centerpiece)

Terminal A:

```bash
make fprime-gds      # http://localhost:5000
```

Terminal B:

```bash
make gds-demo
```

Narrate while it sends commands:

1. `CLEAR_FAULT` → Charge; SOC climbs at 1 Hz.
2. `PASS_SCHEDULED` → AwaitPass; PA still inhibited.
3. `PASS_ACQUIRED` → Downlink; PA keyed, burst energy reserved.
4. `DOWNLINK_COMPLETE` → Charge; PA inhibited.
5. Second `PASS_ACQUIRED` → **refused**: guard latches fault, Safe, PA stays off.

In the GDS, plot:
- `powerManager.Mode`
- `powerManager.SocPermille`
- `dataStore.Records`
- `radioDriver.PaState`

Events to show: `PaKeyed`, `PaInhibited`, `FaultLatched`.

## Minute 7–9: code, three files only

```bash
sed -n '1,90p'  fsw/fprime/cubesat-fsw/Cubesat/PowerCore/power.hpp
sed -n '1,80p'  fsw/fprime/cubesat-fsw/Cubesat/PowerCore/power.cpp
sed -n '30,90p' fsw/fprime/cubesat-fsw/Cubesat/Components/PowerManager/PowerManager.cpp
```

Three claims:

- policy core is framework-free and deterministic
- `discharge(13 W, 300 s)` happens **before** `paEnable_out(1)`
- milliwatt-seconds keeps the 1 Hz charge tick from truncating to zero

## Minute 9–10: breadth and limits

Optional, depending on time:

```bash
make rust-test          # host mission-logic model
make fprime-cli ...     # command-line equivalent of the GDS script
```

Show `cad/cubesat.png` if they care about the physical model.

Limits to state clearly:

- Linux host deployment; no flight hardware
- single linear battery model, no thermal coupling
- primary CCSDS header only
- next steps: temperature-dependent limits, persistent fault log, HIL test rig

## Fallbacks

| Problem | Do this |
|---|---|
| GDS won't start | Use `make fprime-run` + the command-line `fprime-cli` commands in `scripts/gds-demo.sh` |
| Build fails | `make clean && make fprime` (venv is prebuilt) |
| No browser | Read `logs/fprime-gds-*/channel.log` and `event.log` (gds-demo prints the paths) |
| Time is short | `make fsw` + `make fprime-run` + the diagram; skip GDS |

## The one-line close

> “The guard is enforced in the component, not the UI, so the same refusal
> happens from the GDS, a script, or the flight sequencer.”
