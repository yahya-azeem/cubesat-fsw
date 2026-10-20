#!/usr/bin/env bash
# Create the F´ tooling venv. No sudo required.
# Re-run this after cloning the repo, or if the venv is ever moved.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
VENV="$ROOT/.venv-fprime"

if [[ ! -x "$VENV/bin/python" ]]; then
    python3 -m venv "$VENV"
fi

"$VENV/bin/pip" install --upgrade pip
# CMake < 4: F´ 4.4 installs hashes.txt at the root, which CMake 4 rejects.
"$VENV/bin/pip" install \
    fprime-tools \
    fprime-bootstrap \
    fprime-fpp \
    'fprime-gds==4.4.0' \
    'cmake<4'

echo
echo "F´ tooling ready. Activate with:"
echo "  bash/zsh: source $VENV/bin/activate"
echo "  fish:     source $VENV/bin/activate.fish"
