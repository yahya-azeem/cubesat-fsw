#!/usr/bin/env bash
# Guided live demo: start the GDS first (make fprime-gds), then run this.
# It walks one command at a time and tells you what to look at in the browser.
set -uo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROJ="$ROOT/fsw/fprime/cubesat-fsw"
cd "$PROJ"

# shellcheck disable=SC1091
source "$ROOT/.venv-fprime/bin/activate"

if ! curl -sf --max-time 2 http://127.0.0.1:5000/ >/dev/null; then
    echo "GDS is not running. In another terminal run:" >&2
    echo "  cd $ROOT && make fprime-gds" >&2
    exit 1
fi

if [[ "${1:-}" == "--open" ]]; then
    xdg-open http://127.0.0.1:5000 >/dev/null 2>&1 || true
fi

CLI=(fprime-cli command-send -d build-artifacts/Linux/CubesatDeployment)

step() {
    local n="$1" title="$2" look="$3" cmd="$4"
    echo
    echo "────────────────────────────────────────────────────────────"
    echo "STEP $n — $title"
    echo "Look at: $look"
    echo "Sending: $cmd"
    read -r -p "Press Enter (or wait) … " _ || true
    "${CLI[@]}" "$cmd" >/dev/null
    sleep 1
}

echo "Guided pass — browser at http://localhost:5000"
echo "Tip: open Channels and tick powerManager.Mode and powerManager.SocPermille."

step 1 "Clear the latched fault" \
     "Mode becomes 1 (Charge); SOC starts climbing at 1 Hz" \
     CubesatDeployment.powerManager.CLEAR_FAULT

step 2 "Schedule a pass" \
     "Mode becomes 2 (AwaitPass); PA stays inhibited" \
     CubesatDeployment.powerManager.PASS_SCHEDULED

step 3 "Acquire the pass" \
     "Mode becomes 3 (Downlink); RadioDriver.PaState goes ON" \
     CubesatDeployment.powerManager.PASS_ACQUIRED

step 4 "Complete the downlink" \
     "Mode returns to 1 (Charge); PaInhibited event" \
     CubesatDeployment.powerManager.DOWNLINK_COMPLETE

step 5 "Schedule a second pass" \
     "Mode becomes 2 (AwaitPass) again" \
     CubesatDeployment.powerManager.PASS_SCHEDULED

step 6 "Acquire without recharging" \
     "Guard refuses: FaultLatched 0xE0B1, Mode 0 (Safe), PA stays OFF" \
     CubesatDeployment.powerManager.PASS_ACQUIRED

echo
echo "Done. In the browser:"
echo "  Channels → powerManager.Mode, SocPermille, dataStore.Records"
echo "  Events   → PaKeyed / PaInhibited / FaultLatched"
echo "  Commands → the same four powerManager commands"
latest=$(ls -dt logs/fprime-gds-* 2>/dev/null | head -1)
[[ -n "${latest:-}" ]] && echo "  Logs     → $latest"
