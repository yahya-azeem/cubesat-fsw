#!/usr/bin/env python3
"""Expose AstroNav telemetry from the newest F´ GDS log as JSON.

The GDS writes every channel to logs/fprime-gds-*/channel.log. This bridge
tails that file and serves the latest AstroNav values on /state, so neither
the flight software nor AstroSim needs to know about the log format.

    python3 integrations/telemetry/serve.py        # :8787/state
"""

from __future__ import annotations

import json
import math
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LOG_DIR = ROOT / "fsw/fprime/cubesat-fsw/logs"
PORT = int(os.environ.get("PORT", "8787"))

FIELDS = ("PosX", "PosY", "PosZ", "VelX", "VelY", "VelZ",
          "SunX", "SunY", "SunZ", "AltKm", "SpeedKmS", "LatDeg", "LonDeg", "Et")


def newest_channel_log() -> Path | None:
    logs = sorted(LOG_DIR.glob("fprime-gds-*"),
                  key=lambda p: p.stat().st_mtime, reverse=True)
    for run in logs:
        candidate = run / "channel.log"
        if candidate.is_file():
            return candidate
    return None


def read_state() -> dict:
    path = newest_channel_log()
    if path is None:
        return {"ok": False, "reason": "no GDS telemetry yet"}

    try:
        lines = path.read_text(errors="ignore").splitlines()[-800:]
    except OSError:
        return {"ok": False, "reason": "cannot read the GDS log"}

    values: dict[str, float] = {}
    for line in lines:
        parts = line.split(",")
        if len(parts) < 5 or ".astroNav." not in parts[2]:
            continue
        key = parts[2].rsplit(".", 1)[-1]
        if key not in FIELDS:
            continue
        try:
            value = float(parts[4])
        except ValueError:
            continue
        if math.isfinite(value):
            values[key] = value

    missing = [name for name in FIELDS if name not in values]
    if missing:
        return {"ok": False, "reason": f"waiting for {', '.join(missing[:3])}"}

    return {
        "ok": True,
        "pos": [values["PosX"], values["PosY"], values["PosZ"]],
        "velocity": [values["VelX"], values["VelY"], values["VelZ"]],
        "sun": [values["SunX"], values["SunY"], values["SunZ"]],
        "alt": values["AltKm"],
        "speed": values["SpeedKmS"],
        "lat": values["LatDeg"],
        "lon": values["LonDeg"],
        "et": values["Et"],
    }


class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        path = self.path.split("?", 1)[0]
        if path == "/state":
            self._json(read_state())
        elif path == "/health":
            self._json({"ok": True})
        else:
            self._json({"ok": False, "reason": "use /state"}, status=404)

    def _json(self, payload: dict, status: int = 200) -> None:
        body = json.dumps(payload).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args) -> None:
        pass


if __name__ == "__main__":
    print(f"telemetry bridge: http://localhost:{PORT}/state  (source: {LOG_DIR})")
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
