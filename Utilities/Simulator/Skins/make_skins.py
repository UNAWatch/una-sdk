#!/usr/bin/env python3
"""Make the TouchGFX simulator skins that show the UNA Watch around the screen.

    python3 Utilities/Simulator/Skins/make_skins.py

Reads the three watch renders (white, black, teal body) from the Figma UI kit
(Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig, a zip), scales each so
its screen hole matches the 240x240 display, and writes landscape_<body>.png next to
this script. The example apps' simulator build copies them next to simulator.exe;
SDK/Simulator/WatchHAL.hpp shows them and cycles through them with F4.

All three are cut with the same box, so they share one display offset (printed at the
end; it must equal kSkinX / kSkinY in WatchHAL.hpp) and any of them can replace
another, or an app's simulator/landscape.png.

The skins are fully opaque: the watch on a plain dark background, its screen hole
filled black. A skin with any transparency makes HALSDL2 cut a shaped window, and the
window shape also cuts out the screen hole, so the display never shows. With an
opaque skin the window is a plain rectangle; TouchGFX draws the skin first and the
display on top, and WatchHAL paints the display's square corners from the skin so
the screen reads as round.

Needs Pillow (pip install pillow).
"""
import io
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
SDK = HERE.parents[2]
FIG = SDK / "Docs/Templates/Figma-UI-Kit/UNA-Watch-UI-Resource-Pack.fig"

# Render file-name suffix per body colour, inside the .fig zip.
BODIES = {"white": "fc144d36", "black": "7214c36d", "teal": "f66563f5"}
HOLE_CX, HOLE_CY, HOLE_D = 999.5, 1010.5, 734.0  # screen hole in the 2000x2000 renders
SCREEN = 240
HOLE_ON_SCREEN = 238  # a touch under 240, so the bezel overlaps the display edge by 1 px
MARGIN = 4
BACKGROUND = (40, 40, 44)  # around the watch; dark, so every body colour stands out


def load(fig: zipfile.ZipFile, suffix: str) -> Image.Image:
    name = next(n for n in fig.namelist() if n.endswith(suffix))
    return Image.open(io.BytesIO(fig.read(name))).convert("RGBA")


def main() -> None:
    fig = zipfile.ZipFile(FIG)
    s = HOLE_ON_SCREEN / HOLE_D

    scaled = {}
    box = None
    for body, suffix in BODIES.items():
        render = load(fig, suffix)
        size = round(render.width * s)
        img = render.resize((size, size), Image.LANCZOS)
        scaled[body] = img
        # One crop box for all bodies: the union of the watch outlines.
        alpha = img.getchannel("A").point(lambda v: 255 if v >= 128 else 0)
        b = alpha.getbbox()
        box = b if box is None else (min(box[0], b[0]), min(box[1], b[1]),
                                     max(box[2], b[2]), max(box[3], b[3]))

    size = next(iter(scaled.values())).width
    box = (max(0, box[0] - MARGIN), max(0, box[1] - MARGIN),
           min(size, box[2] + MARGIN), min(size, box[3] + MARGIN))
    hx, hy, hr = HOLE_CX * s, HOLE_CY * s, HOLE_D * s / 2 + 2

    for body, img in scaled.items():
        # Opaque: background, a black disc behind the screen hole, then the watch.
        flat = Image.new("RGBA", img.size, BACKGROUND + (255,))
        ImageDraw.Draw(flat).ellipse((hx - hr, hy - hr, hx + hr, hy + hr), fill=(0, 0, 0, 255))
        flat.alpha_composite(img)
        out = HERE / f"landscape_{body}.png"
        flat.convert("RGB").crop(box).save(out, optimize=True)
        print(f"wrote {out.name} ({box[2] - box[0]}x{box[3] - box[1]})")

    # Where the 240x240 display's top-left goes in the skin: centred on the hole.
    sx = round(hx - SCREEN / 2) - box[0]
    sy = round(hy - SCREEN / 2) - box[1]
    print(f"display offset: x = {sx}, y = {sy} (WatchHAL kSkinX / kSkinY)")


if __name__ == "__main__":
    main()
