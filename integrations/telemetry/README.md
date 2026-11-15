# Telemetry bridge

A ~100-line Python bridge between the F´ GDS and the visualisations.

The GDS writes every telemetry channel to
`fsw/fprime/cubesat-fsw/logs/fprime-gds-*/channel.log`. This service tails the
newest one and exposes the AstroNav channels as JSON:

```bash
python3 integrations/telemetry/serve.py
curl http://127.0.0.1:8787/state
```

```json
{
  "ok": true,
  "pos": [5938.8, 3564.7, 148.6],
  "velocity": [-2.53, 3.97, 5.94],
  "sun": [-1.0, 0.0, 0.0],
  "alt": 550.0,
  "speed": 7.585,
  "lat": 1.35,
  "lon": -19.83,
  "et": 847713694.18
}
```

Positions are J2000 equatorial kilometres, Earth-centred. `/state` sends
`Access-Control-Allow-Origin: *`, so the AstroSim wasm build can fetch it from
a different local port.

There is no JavaScript in this repo for the orbit view. The only browser code
is the small AstroSim bridge and the iframe API:

- `integrations/astrosim/cubesat_bridge.js`
- `integrations/astrosim/embed.html`

The renderer itself is Rust/wgpu/ImGui (AstroSim).
