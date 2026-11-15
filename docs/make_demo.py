#!/usr/bin/env python3
"""Build the README hero, social preview and the 48 s captioned demo reel.

Run from the repo root:  python3 docs/make_demo.py
Requires Pillow and ffmpeg. The video is assembled from real screenshots and
the real GDS demo output; it is a captioned reel, not a live screen capture.
"""

from __future__ import annotations

import subprocess
import textwrap
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
FRAMES = DOCS / "frames"
FRAMES.mkdir(parents=True, exist_ok=True)

BG = (5, 7, 13)
PANEL = (10, 16, 29)
BORDER = (32, 52, 84)
TEXT = (222, 235, 255)
DIM = (127, 167, 216)
ACCENT = (53, 208, 255)
GOLD = (255, 204, 51)

FONT_BOLD = "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"
FONT_REG = "/usr/share/fonts/TTF/DejaVuSans.ttf"
FONT_MONO = "/usr/share/fonts/TTF/DejaVuSansMono.ttf"


def font(path: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(path, size)


def cover(path: Path, size: tuple[int, int]) -> Image.Image:
    image = Image.open(path).convert("RGB")
    return ImageOps.fit(image, size, Image.Resampling.LANCZOS)


def rounded_panel(draw: ImageDraw.ImageDraw, box, radius=14):
    draw.rounded_rectangle(box, radius=radius, fill=PANEL, outline=BORDER, width=2)


def wrap(draw, text, font_obj, width):
    words = text.split()
    lines, line = [], ""
    for word in words:
        candidate = f"{line} {word}".strip()
        if draw.textlength(candidate, font=font_obj) <= width:
            line = candidate
        else:
            lines.append(line)
            line = word
    if line:
        lines.append(line)
    return lines


def hero() -> Image.Image:
    width, height = 1600, 800
    image = Image.new("RGB", (width, height), BG)
    draw = ImageDraw.Draw(image)

    draw.text((60, 74), "1U CubeSat", font=font(FONT_BOLD, 78), fill=TEXT)
    draw.text((64, 176), "Flight-software sandbox", font=font(FONT_REG, 36), fill=DIM)

    lines = [
        "F´ · CSPICE · LittleFS",
        "live telemetry · wasm 3D",
    ]
    y = 250
    for line in lines:
        draw.text((64, y), line, font=font(FONT_REG, 29), fill=TEXT)
        y += 44

    draw.line((64, y + 26, 520, y + 26), fill=BORDER, width=2)
    draw.text((64, y + 56), "Demo-grade — not flight-qualified",
              font=font(FONT_BOLD, 24), fill=GOLD)

    earth = cover(ROOT / "integrations/orbit-view/live.png", (470, 700))
    universe = cover(ROOT / "integrations/astrosim/astrosim-universe.png", (470, 700))
    image.paste(earth, (630, 50))
    image.paste(universe, (1120, 50))
    draw.rectangle((630, 50, 1099, 749), outline=BORDER, width=2)
    draw.rectangle((1120, 50, 1589, 749), outline=BORDER, width=2)
    draw.text((646, 706), "ORBIT · LIVE", font=font(FONT_BOLD, 20), fill=ACCENT)
    draw.text((1136, 706), "UNIVERSE", font=font(FONT_BOLD, 20), fill=GOLD)
    return image


def social() -> Image.Image:
    width, height = 1280, 640
    image = Image.new("RGB", (width, height), BG)
    draw = ImageDraw.Draw(image)
    draw.text((54, 58), "1U CubeSat", font=font(FONT_BOLD, 62), fill=TEXT)
    draw.text((56, 136), "F´ + CSPICE + live 3D", font=font(FONT_REG, 30), fill=DIM)
    draw.text((56, 210), "Flight-software sandbox", font=font(FONT_REG, 26), fill=TEXT)
    draw.text((56, 258), "Demo-grade — not flight-qualified",
              font=font(FONT_BOLD, 20), fill=GOLD)
    draw.text((56, 330), "146 host assertions", font=font(FONT_MONO, 20), fill=ACCENT)
    draw.text((56, 360), "14 F´ component tests", font=font(FONT_MONO, 20), fill=ACCENT)
    draw.text((56, 390), "github.com/yahya-azeem/cubesat-fsw",
              font=font(FONT_MONO, 18), fill=DIM)

    image.paste(cover(ROOT / "integrations/orbit-view/live.png", (360, 540)), (470, 50))
    image.paste(cover(ROOT / "integrations/astrosim/astrosim-universe.png", (360, 540)), (870, 50))
    draw.rectangle((470, 50, 829, 589), outline=BORDER, width=2)
    draw.rectangle((870, 50, 1229, 589), outline=BORDER, width=2)
    return image


def frame(title: str, subtitle: str, body: list[str], image_path: Path | None = None,
          accent=ACCENT) -> Image.Image:
    width, height = 1920, 1080
    canvas = Image.new("RGB", (width, height), BG)
    draw = ImageDraw.Draw(canvas)

    draw.text((90, 62), title, font=font(FONT_BOLD, 58), fill=TEXT)
    draw.text((94, 136), subtitle, font=font(FONT_REG, 28), fill=DIM)
    draw.line((94, 190, 760, 190), fill=accent, width=2)

    if image_path is not None:
        art = cover(image_path, (900, 760))
        canvas.paste(art, (930, 210))
        draw.rectangle((930, 210, 1829, 969), outline=BORDER, width=2)
        text_x, text_w = 94, 760
    else:
        text_x, text_w = 94, 1700

    y = 250
    for line in body:
        if line.startswith("$") or line.startswith(">"):
            color, font_obj = ACCENT, font(FONT_MONO, 26)
        elif line.startswith("#"):
            color, font_obj = GOLD, font(FONT_BOLD, 26)
        else:
            color, font_obj = TEXT, font(FONT_REG, 30)
        for wrapped in wrap(draw, line, font_obj, text_w):
            draw.text((text_x, y), wrapped, font=font_obj, fill=color)
            y += 42
        y += 8
    return canvas


def build_frames() -> list[Path]:
    frames: list[Path] = []

    f = frame(
        "1U CubeSat", "Flight-software sandbox · F´ · CSPICE · LittleFS",
        [
            "A demo-grade 1U avionics stack:",
            "power supervisor, fail-safe file system,",
            "CSPICE ephemerides, live telemetry.",
            "",
            "# 146 host assertions · 14 F´ component tests",
        ],
    )
    frames.append(FRAMES / "01.png")
    f.save(frames[-1])

    f = frame(
        "Live orbit", "CSPICE at 1 Hz · altitude and velocity from the SPK",
        [
            "AstroNav publishes J2000 position,",
            "velocity, Sun vector and sub-point.",
            "",
            "The view reads the F´ GDS channel log;",
            "flight software knows nothing about it.",
            "",
            "# 550.0 km · 7.585 km/s",
        ],
        ROOT / "integrations/orbit-view/live.png",
    )
    frames.append(FRAMES / "02.png")
    f.save(frames[-1])

    f = frame(
        "Big picture", "AstroSim wasm · full galactic-scale simulation",
        [
            "The same telemetry drives the",
            "CUBESAT entity inside AstroSim:",
            "Rust, wgpu, ImGui.",
            "",
            "Universe mode zooms out to the",
            "full simulation.",
        ],
        ROOT / "integrations/astrosim/astrosim-universe.png",
        GOLD,
    )
    frames.append(FRAMES / "03.png")
    f.save(frames[-1])

    f = frame(
        "Specific mode", "Earth limb · literal OpenSCAD 1U mesh",
        [
            "The entity is the exact STL export",
            "of cad/cubesat.scad: 7,920 triangles.",
            "",
            "Scaled as a visibility marker;",
            "true 1U is 113.5 mm.",
            "",
            "ImGui panel: Earth / Universe.",
        ],
        ROOT / "integrations/astrosim/astrosim-earth.png",
    )
    frames.append(FRAMES / "04.png")
    f.save(frames[-1])

    f = frame(
        "Duty-cycled power", "make gds-demo · command the state machine",
        [
            "$ make gds-demo",
            "> CLEAR_FAULT        Mode 1  Charge",
            "> PASS_SCHEDULED     Mode 2  AwaitPass",
            "> PASS_ACQUIRED      Mode 3  Downlink   PaKeyed",
            "> DOWNLINK_COMPLETE  Mode 1  Charge     PaInhibited",
            "> PASS_ACQUIRED      Mode 0  Safe       FaultLatched 0xE0B1",
            "",
            "# the battery guard refuses a second burst",
        ],
        None,
        GOLD,
    )
    frames.append(FRAMES / "05.png")
    f.save(frames[-1])

    f = frame(
        "Reproducible", "Clean clone · CI · one command",
        [
            "$ git clone --recurse-submodules <repo>",
            "$ ./setup/install-fprime.sh",
            "$ ./setup/install-cspice.sh",
            "$ make check",
            "",
            "# all host and component tests pass",
            "# demo-grade, not flight-qualified",
            "github.com/yahya-azeem/cubesat-fsw",
        ],
    )
    frames.append(FRAMES / "06.png")
    f.save(frames[-1])

    return frames


def main() -> None:
    hero().save(DOCS / "hero.png")
    social().save(DOCS / "social-preview.png")
    frames = build_frames()

    concat = DOCS / "frames" / "list.txt"
    with concat.open("w") as handle:
        for path in frames:
            handle.write(f"file '{path.name}'\n")
            handle.write("duration 8\n")
    subprocess.run(
        [
            "ffmpeg", "-y", "-f", "concat", "-safe", "0", "-i", str(concat),
            "-vf", "fps=30,format=yuv420p", "-c:v", "libx264",
            "-preset", "medium", "-crf", "20", "-movflags", "+faststart",
            str(DOCS / "demo.mp4"),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    print("wrote docs/hero.png, docs/social-preview.png, docs/demo.mp4")


if __name__ == "__main__":
    main()
