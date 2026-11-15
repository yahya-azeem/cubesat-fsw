# v0.1.0 — 1U CubeSat flight-software sandbox

First public, reproducible snapshot. **Demo-grade, not flight-qualified.**

## Highlights

- **F´ 4.4.1 deployment** with a 1 Hz rate group:
  `PowerManager` (battery reserve + PA gate), `DataStore` (LittleFS),
  `RadioDriver` (PA consumer) and `AstroNav` (CSPICE telemetry).
- **PowerCore** — milliwatt-second accounting, 20 % reserve guard,
  fail-closed `Safe → Charge → AwaitPass → Downlink` supervisor,
  CCSDS TM primary header.
- **Storage** — vendored LittleFS 2.11.2 with RAM and SPI-NOR backends,
  power-loss recovery and corrupt-superblock fail-closed behaviour.
- **Spice** — C++23, `std::expected`-based CSPICE wrapper: bounded strings,
  RETURN error mode, finite-state and orthonormal-frame checks, RAII kernels.
- **AstroNav** — synthetic LEO SPK, 1 Hz position/velocity/Sun/sub-point
  telemetry.
- **Live visuals** — Three.js orbit view and a patched AstroSim wasm build
  with ImGui Earth/Universe modes, literal OpenSCAD 1U STL mesh, and an
  iframe `postMessage` API.
- **CAD** — parametric OpenSCAD 1U model with a manifold checker.

## Verification

- 146 host assertions: power, LittleFS, CSPICE (incl. ASan/UBSan pass)
- 14 F´ component tests (`fprime-util check`)
- GitHub Actions CI on Ubuntu 24.04
- Fresh-clone test with user-level installers

## Install

```bash
git clone --recurse-submodules https://github.com/yahya-azeem/cubesat-fsw.git
cd cubesat-fsw
./setup/install.sh && ./setup/install-fprime.sh && ./setup/install-cspice.sh
make check
```

`install.sh` targets Arch/CachyOS; Debian/Ubuntu use the apt block in
`.github/workflows/ci.yml`.

## Known limits

- Synthetic ephemeris; furnish a mission SPK for real work.
- No hardware, RF, ADCS or thermal qualification.
- CAD is packaging only, not structural/thermal analysis.
- AstroSim is patched locally and built from a pinned commit; not redistributed.

## Demo

`docs/demo.mp4` — 48-second captioned reel: live telemetry, Earth/Universe
modes, GDS command sequence and the battery-guard fault.
