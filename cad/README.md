# 1U CubeSat packaging model (OpenSCAD)

Parametric mechanical envelope for the 1U architecture in the main README.

This is a **packaging/visual model**. Structural and thermal margins require
FEA and a thermal solver; a CAD envelope cannot answer those questions.

| Parameter | Value |
|---|---|
| Envelope | 100 × 100 mm body, 113.5 mm rail length (1U) |
| Rails | 8.5 × 8.5 mm, 0.8 mm chamfer, four corners |
| Body | 100 mm cube between rails |
| Panels | 84 mm wide, 2 mm thick, body-mounted cells |
| Board stack | 82 × 96 mm PCBs on M3 standoffs |
| Units | `u = 1, 2, 3` (body and rails scale; stack stays 1U) |

## Views

| Assembly | Exploded | Stack cutaway |
|---|---|---|
| ![](cubesat.png) | ![](cubesat-exploded.png) | ![](cubesat-stack.png) |

## Render and verify

```bash
# system package (recommended)
sudo pacman -S openscad

make cad            # cubesat.stl + cubesat-exploded.stl
make cad-check      # manifold + degenerate-triangle verification
make cad-preview    # PNGs (needs a display or xvfb)
```

Without installing, the official AppImage works:

```bash
curl -LO https://files.openscad.org/OpenSCAD-2021.01-x86_64.AppImage
chmod +x OpenSCAD-2021.01-x86_64.AppImage
make cad cad-check OPENSCAD=./OpenSCAD-2021.01-x86_64.AppImage
```

The checked meshes are **edge-manifold**: every edge is shared by exactly two
triangles, with no degenerate triangles. The exploded view is intentionally
several disjoint shells; that is still a valid manifold.

Useful overrides:

```bash
openscad -D 'u=3'                 -o 3u.stl    cad/cubesat.scad
openscad -D 'explode=0.6'         -o x.stl     cad/cubesat.scad
openscad -D 'show_panels=false'   -o stack.stl cad/cubesat.scad
openscad -D 'show_mli=false'      -o bare.stl  cad/cubesat.scad
openscad -D 'show_antenna=false'  -o stowed.stl cad/cubesat.scad
```

## What maps to what

| Model feature | Subsystem | Colour |
|---|---|---|
| Board 1 box | EPS | orange |
| Board 2 box | OBC (VA41620) | blue |
| Board 3 box + heatsink | S-band / UHF radio | purple + grey |
| Board 4 box | payload | red |
| Green plates | PCBs | green |
| Gold plate on the nadir face | S-band patch antenna | gold |
| Copper whip on the top corner | deployable UHF telecommand antenna | copper |
| Dark blue 3×4 arrays | body-mounted solar strings | navy |
| Gold liners inside the panels | MLI thermal blanket | gold |
| Black tubes in the -X/-Y corner | harness bundle | black |
| Red tab on +X | remove-before-flight switch | red |
| Grey block on +X | access / data port | grey |

## What is still simplified

- **No structural/thermal analysis.** These are geometry only; run FEA and a
  thermal model before trusting any margin.
- Fasteners are simplified screw heads, not threaded hardware.
- The harness is a two-cable bundle, not a routed, connectorised harness.
- MLI is a flat liner without seams, strapping or kapton layering.
- The deployer shows its housing, hinge pin and burn wire; deployment
  kinematics and the stowed whip coil are not modelled.
- The 0.8 mm rail chamfer is modelled; the CubeSat Design Specification rail
  cut-outs and fastening holes are not.
- The internal stack is illustrative: real boards need keep-out, connector
  and harness routing checks against the rails.
