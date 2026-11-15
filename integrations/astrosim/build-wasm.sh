#!/usr/bin/env bash
# Build AstroSim with the live-CubeSat patch for the web (wasm).
# Requires: rust wasm32 target, ~/emsdk, wasm-bindgen-cli 0.2.125.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HERE="$ROOT/integrations/astrosim"
SRC="${ASTROSIM_SRC:-$HERE/upstream}"
EMSDK="${EMSDK:-$HOME/emsdk}"
SHA=$(cat "$HERE/upstream.sha")

if [[ ! -d "$SRC/.git" ]]; then
    echo "==> cloning AstroSim @ $SHA"
    git clone https://github.com/yahya-azeem/AstroSim.git "$SRC"
    git -C "$SRC" checkout "$SHA"
    git -C "$SRC" apply "$HERE/cubesat.patch"
else
    echo "==> using existing $SRC"
fi
cp "$HERE/cubesat.stl" "$SRC/src/render/cubesat.stl"
cp "$HERE/embed.html" "$SRC/embed.html"

# shellcheck disable=SC1091
source "$EMSDK/emsdk_env.sh" >/dev/null

export CFLAGS="-I$EMSDK/upstream/emscripten/cache/sysroot/include"
export CXXFLAGS="-I$EMSDK/upstream/emscripten/cache/sysroot/include -I$EMSDK/upstream/emscripten/cache/sysroot/include/c++/v1"
export RUSTFLAGS="-L $EMSDK/upstream/emscripten/cache/sysroot/lib/wasm32-emscripten/ -C link-arg=-lc -L $SRC/lib --cfg getrandom_backend=\"wasm_js\""

echo "==> building wasm"
(cd "$SRC" && cargo build --lib --target wasm32-unknown-unknown --release)

echo "==> wasm-bindgen"
wasm-bindgen --target web --out-dir "$SRC/pkg" \
    "$SRC/target/wasm32-unknown-unknown/release/AstroSim.wasm"

echo
echo "AstroSim ready. Start the telemetry bridge and serve the app:"
echo "  python3 $ROOT/integrations/telemetry/serve.py"
echo "  python3 -m http.server 8000 --directory $SRC"
echo "  open http://localhost:8000/"
