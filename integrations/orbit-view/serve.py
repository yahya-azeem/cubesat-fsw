#!/usr/bin/env python3
"""Serve the orbit view and a /state feed parsed from the newest GDS log.

The F´ GDS writes each telemetry channel to logs/fprime-gds-*/channel.log.
This reads the tail of that file, so nothing in the flight software needs to
know about the web UI.
"""

from __future__ import annotations

import json
import math
import os
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
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


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(HERE), **kwargs)

    def do_GET(self) -> None:
        if self.path.split("?", 1)[0] == "/state":
            body = json.dumps(read_state()).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            super().do_GET()

    def log_message(self, *args) -> None:  # keep the terminal clean
        pass


if __name__ == "__main__":
    print(f"orbit view: http://localhost:{PORT}  (/state from {LOG_DIR})")
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
