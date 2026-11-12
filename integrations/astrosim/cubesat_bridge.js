// Feed AstroSim's wasm export from the live F´/CSPICE bridge.
// The orbit view's /state is J2000 equatorial km; AstroSim works in
// Earth-centred ecliptic metres, so rotate about X by the obliquity.

const OBLIQUITY = (23.4392911 * Math.PI) / 180.0;
const METRES = 1000.0;

export function startCubesatBridge(setState, endpoint = 'http://127.0.0.1:8787/state', onState = () => {}) {
  let failures = 0;

  async function tick() {
    try {
      const response = await fetch(endpoint, { cache: 'no-store' });
      const state = await response.json();
      if (state.ok) {
        const [x, y, z] = state.pos.map((v) => v * METRES);
        const [vx, vy, vz] = state.velocity.map((v) => v * METRES);
        const c = Math.cos(OBLIQUITY);
        const s = Math.sin(OBLIQUITY);
        setState(x, (y * c) + (z * s), (-y * s) + (z * c),
                 vx, (vy * c) + (vz * s), (-vy * s) + (vz * c));
        failures = 0;
        onState(state);
      }
    } catch (error) {
      failures += 1;
      if (failures === 1) console.warn('CubeSat bridge offline:', error);
    }
    setTimeout(tick, 500);
  }

  tick();
}
