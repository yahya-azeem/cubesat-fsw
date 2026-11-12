// Live 1U CubeSat view fed by AstroNav telemetry through serve.py.
// J2000, Earth-centred, kilometres. One scene unit = one Earth radius.

import * as THREE from './three.module.js';

const KM_PER_UNIT = 6378.137;
const MAX_TRAIL = 1400;

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x02030a);

const camera = new THREE.PerspectiveCamera(48, innerWidth / innerHeight, 0.01, 300);
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
renderer.setSize(innerWidth, innerHeight);
document.body.appendChild(renderer.domElement);

scene.add(new THREE.AmbientLight(0x8296bd, 1.1));
const sunLight = new THREE.DirectionalLight(0xfff0c0, 2.2);
scene.add(sunLight);

// Earth + stylised graticule + atmosphere.
const earth = new THREE.Mesh(
  new THREE.SphereGeometry(1, 64, 48),
  new THREE.MeshStandardMaterial({ color: 0x1b4fa8, roughness: 0.8, metalness: 0.05 })
);
scene.add(earth);

const graticule = new THREE.Mesh(
  new THREE.SphereGeometry(1.0025, 36, 24),
  new THREE.MeshBasicMaterial({
    color: 0x3f7fe0, wireframe: true, transparent: true, opacity: 0.16
  })
);
scene.add(graticule);

const atmosphere = new THREE.Mesh(
  new THREE.SphereGeometry(1.035, 48, 32),
  new THREE.MeshBasicMaterial({
    color: 0x3b82f6, transparent: true, opacity: 0.09, side: THREE.BackSide
  })
);
scene.add(atmosphere);

// Stars.
{
  const count = 1200;
  const positions = new Float32Array(count * 3);
  for (let i = 0; i < count; i++) {
    const r = 40 + Math.random() * 60;
    const theta = Math.random() * Math.PI * 2;
    const phi = Math.acos(2 * Math.random() - 1);
    positions[i * 3 + 0] = r * Math.sin(phi) * Math.cos(theta);
    positions[i * 3 + 1] = r * Math.cos(phi);
    positions[i * 3 + 2] = r * Math.sin(phi) * Math.sin(theta);
  }
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  scene.add(new THREE.Points(
    geometry,
    new THREE.PointsMaterial({ color: 0x9fc4ff, size: 0.09, sizeAttenuation: true })
  ));
}

// 1U CubeSat: body + four deployed solar wings, exaggerated for visibility.
const satellite = new THREE.Group();
const body = new THREE.Mesh(
  new THREE.BoxGeometry(0.050, 0.050, 0.090),
  new THREE.MeshStandardMaterial({ color: 0xd7dbe2, roughness: 0.4, metalness: 0.75 })
);
satellite.add(body);
for (const side of [-1, 1]) {
  const panel = new THREE.Mesh(
    new THREE.BoxGeometry(0.160, 0.004, 0.090),
    new THREE.MeshStandardMaterial({ color: 0x1b2f7a, roughness: 0.3, metalness: 0.5 })
  );
  panel.position.set(side * 0.108, 0, 0);
  satellite.add(panel);
}
const beacon = new THREE.Mesh(
  new THREE.SphereGeometry(0.022, 12, 12),
  new THREE.MeshBasicMaterial({ color: 0x8ff3ff, transparent: true, opacity: 0.55 })
);
satellite.add(beacon);
scene.add(satellite);

// Full predicted orbit (circular approximation from r, v).
const orbitGeometry = new THREE.BufferGeometry();
const orbitLine = new THREE.Line(
  orbitGeometry,
  new THREE.LineBasicMaterial({ color: 0x35d0ff, transparent: true, opacity: 0.85 })
);
scene.add(orbitLine);

// Actual driven trail.
const trailPositions = new Float32Array(MAX_TRAIL * 3);
const trailGeometry = new THREE.BufferGeometry();
trailGeometry.setAttribute('position', new THREE.BufferAttribute(trailPositions, 3));
trailGeometry.setDrawRange(0, 0);
const trailLine = new THREE.Line(
  trailGeometry,
  new THREE.LineBasicMaterial({ color: 0x7fe7ff, transparent: true, opacity: 0.35 })
);
scene.add(trailLine);
let trailCount = 0;
const lastPoint = new THREE.Vector3(1e9, 0, 0);

// Nadir marker on the Earth surface.
const nadir = new THREE.Mesh(
  new THREE.SphereGeometry(0.012, 12, 12),
  new THREE.MeshBasicMaterial({ color: 0xff5d5d })
);
scene.add(nadir);

// Sun direction arrow.
const sunArrow = new THREE.ArrowHelper(
  new THREE.Vector3(-1, 0, 0), new THREE.Vector3(0, 0, 0), 2.4, 0xffcc33, 0.12, 0.07
);
scene.add(sunArrow);

// Camera orbit controls (drag / wheel).
let theta = 0.9;
let phi = 1.15;
let radius = 2.6;
let dragging = false;
let lastX = 0;
let lastY = 0;

renderer.domElement.addEventListener('pointerdown', (e) => {
  dragging = true;
  lastX = e.clientX;
  lastY = e.clientY;
});
addEventListener('pointerup', () => { dragging = false; });
addEventListener('pointermove', (e) => {
  if (!dragging) return;
  theta -= (e.clientX - lastX) * 0.005;
  phi = Math.max(0.15, Math.min(Math.PI - 0.15, phi - (e.clientY - lastY) * 0.005));
  lastX = e.clientX;
  lastY = e.clientY;
});
renderer.domElement.addEventListener('wheel', (e) => {
  radius = Math.max(1.5, Math.min(12, radius * (1 + Math.sign(e.deltaY) * 0.08)));
  e.preventDefault();
}, { passive: false });

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

const $ = (id) => document.getElementById(id);
const fmt = (x, n = 2) => Number.isFinite(x) ? x.toFixed(n) : '—';

function setOrbit(radiusUnits, position, velocity) {
  const r = position.clone().normalize();
  const h = new THREE.Vector3().crossVectors(position, velocity);
  const normal = h.lengthSq() > 1e-18 ? h.normalize() : new THREE.Vector3(0, 1, 0);
  const w = new THREE.Vector3().crossVectors(normal, r).normalize();

  const segments = 256;
  const points = new Float32Array((segments + 1) * 3);
  for (let i = 0; i <= segments; i++) {
    const a = (i / segments) * Math.PI * 2;
    const p = r.clone().multiplyScalar(Math.cos(a) * radiusUnits)
      .add(w.clone().multiplyScalar(Math.sin(a) * radiusUnits));
    points[i * 3 + 0] = p.x;
    points[i * 3 + 1] = p.y;
    points[i * 3 + 2] = p.z;
  }
  orbitGeometry.setAttribute('position', new THREE.BufferAttribute(points, 3));
  orbitGeometry.attributes.position.needsUpdate = true;
}

function pushTrail(point) {
  if (trailCount > 0 && point.distanceTo(lastPoint) < 0.0015) return;
  if (trailCount === MAX_TRAIL) {
    trailPositions.copyWithin(0, 3);
    trailCount -= 1;
  }
  trailPositions[trailCount * 3 + 0] = point.x;
  trailPositions[trailCount * 3 + 1] = point.y;
  trailPositions[trailCount * 3 + 2] = point.z;
  trailCount += 1;
  lastPoint.copy(point);
  trailGeometry.setDrawRange(0, trailCount);
  trailGeometry.attributes.position.needsUpdate = true;
}

let haveState = false;

function applyState(s) {
  const position = new THREE.Vector3(s.pos[0], s.pos[1], s.pos[2]);
  const velocity = new THREE.Vector3(s.velocity[0], s.velocity[1], s.velocity[2]);
  const unitPos = position.clone().divideScalar(KM_PER_UNIT);
  const radiusUnits = unitPos.length();

  satellite.position.copy(unitPos);
  nadir.position.copy(unitPos.clone().normalize().multiplyScalar(1.004));

  // Body +Z along velocity, +X towards the Sun.
  const forward = velocity.clone().normalize();
  const sunDir = new THREE.Vector3(s.sun[0], s.sun[1], s.sun[2]).normalize();
  const side = sunDir.clone().sub(forward.clone().multiplyScalar(sunDir.dot(forward)));
  if (side.lengthSq() < 1e-9) side.set(1, 0, 0);
  side.normalize();
  const up = new THREE.Vector3().crossVectors(forward, side).normalize();
  satellite.quaternion.setFromRotationMatrix(
    new THREE.Matrix4().makeBasis(side, up, forward)
  );

  setOrbit(radiusUnits, unitPos, velocity.clone().divideScalar(KM_PER_UNIT));
  pushTrail(unitPos);

  sunArrow.setDirection(sunDir);
  sunArrow.position.copy(new THREE.Vector3(0, 0, 0));
  sunLight.position.copy(sunDir.clone().multiplyScalar(10));

  $('alt').textContent = `${fmt(s.alt, 1)} km`;
  $('speed').textContent = `${fmt(s.speed, 3)} km/s`;
  $('latlon').textContent = `${fmt(s.lat, 2)}° / ${fmt(s.lon, 2)}°`;
  $('et').textContent = `${fmt(s.et, 0)} s`;
  $('status').textContent = `live · ${new Date().toLocaleTimeString()}`;
  $('status').className = '';
  haveState = true;
}

async function poll() {
  try {
    const response = await fetch('/state', { cache: 'no-store' });
    const state = await response.json();
    if (state.ok) applyState(state);
    else {
      $('status').textContent = state.reason || 'waiting for GDS telemetry…';
      $('status').className = 'bad';
    }
  } catch (error) {
    $('status').textContent = 'bridge offline';
    $('status').className = 'bad';
  }
  setTimeout(poll, 500);
}

poll();

function animate() {
  requestAnimationFrame(animate);
  if (!dragging) theta += 0.0009;
  camera.position.set(
    radius * Math.sin(phi) * Math.cos(theta),
    radius * Math.cos(phi),
    radius * Math.sin(phi) * Math.sin(theta)
  );
  camera.lookAt(0, 0, 0);
  renderer.render(scene, camera);
}
animate();
