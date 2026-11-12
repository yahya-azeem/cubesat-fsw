#!/usr/bin/env bash
# Fetch the NAIF CSPICE toolkit (prebuilt Linux x86_64) into third_party/.
# No sudo required. Re-run if third_party/cspice is removed.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DEST="$ROOT/third_party/cspice"
URL="https://naif.jpl.nasa.gov/pub/naif/toolkit/C/PC_Linux_GCC_64bit/packages/cspice.tar.Z"

if [[ -f "$DEST/lib/cspice.a" && -f "$DEST/include/SpiceUsr.h" ]]; then
    echo "CSPICE already present at $DEST"
    exit 0
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

echo "==> downloading CSPICE (~43 MB)"
curl -fsSL -o "$tmp/cspice.tar.Z" "$URL"

echo "==> extracting"
tar -xf "$tmp/cspice.tar.Z" -C "$tmp"
mkdir -p "$DEST"
cp -r "$tmp/cspice/include" "$DEST/"
cp -r "$tmp/cspice/lib" "$DEST/"

echo "==> done: $DEST"
du -sh "$DEST"
