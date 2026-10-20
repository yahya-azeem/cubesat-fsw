#!/usr/bin/env bash
# Open a VCD in gtkwave. Builds it first if missing.
# usage: wave.sh [path/to/file.vcd]
set -euo pipefail

cd "$(dirname "$0")/.."
vcd=${1:-avr/build/firmware.vcd}

if [[ ! -f $vcd ]]; then
    echo "== $vcd missing, generating it"
    make -s vcd
fi

exec gtkwave "$vcd"
