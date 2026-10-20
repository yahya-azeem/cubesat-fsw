#!/usr/bin/env bash
# Drive the mission state machine through the live F´ GDS.
# Start the GDS first (in another terminal):  make fprime-gds
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PROJ="$ROOT/fsw/fprime/cubesat-fsw"
cd "$PROJ"

# shellcheck disable=SC1091
source "$ROOT/.venv-fprime/bin/activate"

if ! curl -sf --max-time 2 http://127.0.0.1:5000/ >/dev/null; then
    echo "GDS is not running. Start it with: make fprime-gds" >&2
    exit 1
fi

CLI=(fprime-cli command-send -d build-artifacts/Linux/CubesatDeployment)

send() {
    echo
    echo ">>> $1"
    "${CLI[@]}" "$1"
    sleep 0.8
}

echo "PowerManager is fail-closed: it boots in Safe (0) and needs a clear."
echo "(run this once after starting the GDS; a second run will refuse the burst"
echo " because the battery has not recharged yet - that is the guard working)"
send CubesatDeployment.powerManager.CLEAR_FAULT
echo "    -> Charge (1): 1 Hz harvesting, battery telemetry climbing"

send CubesatDeployment.powerManager.PASS_SCHEDULED
echo "    -> AwaitPass (2): pass is close, PA still inhibited"

send CubesatDeployment.powerManager.PASS_ACQUIRED
echo "    -> Downlink (3): PA keyed, RadioDriver.PaKeyed, burst energy reserved"

send CubesatDeployment.powerManager.DOWNLINK_COMPLETE
echo "    -> Charge (1): PA inhibited"

send CubesatDeployment.powerManager.PASS_SCHEDULED
send CubesatDeployment.powerManager.PASS_ACQUIRED
echo "    -> second burst refused: energy guard -> FaultLatched, Safe (0)"

sleep 1
latest=$(ls -dt logs/fprime-gds-* | head -1)

echo
echo "== Mode timeline (0 Safe, 1 Charge, 2 AwaitPass, 3 Downlink) =="
grep 'powerManager.Mode' "$latest/channel.log" | tail -8 | awk -F, '{print $1, "->", $5}'

echo
echo "== PA and fault events =="
grep -hE 'PaKeyed|PaInhibited|FaultLatched' "$latest/event.log" | tail -6 |
    awk -F, '{print $1, $3, $6}'

echo
echo "Open http://localhost:5000 to see the same data plotted live."
