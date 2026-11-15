# v0.1.1 — one renderer, less JavaScript

Packaging cleanup; no flight-software behaviour changes.

## Changed

- Removed the standalone Three.js orbit view and its **1.3 MB** vendored
  `three.module.js`.
- AstroSim (Rust + wgpu + ImGui) is now the only 3D renderer.
- Telemetry moved to `integrations/telemetry/serve.py`, a ~100-line Python
  bridge that turns the newest F´ GDS `channel.log` into `/state` JSON.
- The only tracked JavaScript is the 4 KB AstroSim bridge plus the iframe
  demo page.
- README, hero image and the 48 s demo reel updated to match.

## Verification

- CI green (host suites, F´ component tests, deployment build)
- Fresh clone → `setup/install-fprime.sh`, `setup/install-cspice.sh`,
  `make check` → `check=0`
- Telemetry bridge verified against a real GDS log

Demo-grade, not flight-qualified.
