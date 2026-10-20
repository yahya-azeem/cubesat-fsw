#!/usr/bin/env python3
"""Check that an ASCII STL is edge-manifold.

Every undirected edge must be shared by exactly two triangles, and no
triangle may be degenerate. This is the check that matters before printing
or importing the mesh into a slicer.

usage: check_stl.py file.stl [more.stl ...]
"""

from __future__ import annotations

import sys
from collections import Counter


def load_ascii_stl(path: str) -> list[tuple[tuple[float, float, float], ...]]:
    vertices: list[tuple[float, float, float]] = []
    with open(path, encoding="ascii", errors="replace") as handle:
        for line in handle:
            stripped = line.lstrip()
            if stripped.startswith("vertex"):
                values = stripped.split()
                vertices.append(
                    (round(float(values[1]), 4),
                     round(float(values[2]), 4),
                     round(float(values[3]), 4))
                )
    if not vertices:
        raise ValueError(f"{path}: no vertices (binary STL? export ASCII)")
    return [tuple(vertices[i:i + 3]) for i in range(0, len(vertices), 3)]


def check(path: str) -> bool:
    triangles = load_ascii_stl(path)
    edges: Counter[tuple[tuple[float, float, float],
                          tuple[float, float, float]]] = Counter()
    degenerate = 0

    for tri in triangles:
        if len(set(tri)) < 3:
            degenerate += 1
            continue
        for a, b in ((0, 1), (1, 2), (2, 0)):
            edges[tuple(sorted((tri[a], tri[b])))] += 1

    bad = sum(1 for count in edges.values() if count != 2)

    points = [vertex for tri in triangles for vertex in tri]
    lo = [min(p[i] for p in points) for i in range(3)]
    hi = [max(p[i] for p in points) for i in range(3)]

    print(f"{path}:")
    print(f"  triangles     : {len(triangles)}")
    print(f"  non-manifold  : {bad} edge(s)")
    print(f"  degenerate    : {degenerate} triangle(s)")
    print(f"  bbox (mm)     : "
          f"X {lo[0]:.2f}..{hi[0]:.2f}  "
          f"Y {lo[1]:.2f}..{hi[1]:.2f}  "
          f"Z {lo[2]:.2f}..{hi[2]:.2f}")

    return bad == 0 and degenerate == 0


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print(__doc__)
        return 2
    return 0 if all(check(path) for path in argv[1:]) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
