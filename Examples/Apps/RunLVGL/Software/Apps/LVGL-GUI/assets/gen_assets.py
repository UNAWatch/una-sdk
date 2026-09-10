#!/usr/bin/env python3
"""Regenerate the RunLVGL GUI's font and image C files.

Fonts:  Poppins faces -> LVGL fonts through lv_font_conv, run via npx so Node is
        the only prerequisite. 2 bits per pixel like the TouchGFX build, and
        uncompressed because lv_conf.h builds LVGL without the decompressor. The
        large numeric faces only carry the glyphs the value fields print.
Images: PNG icons -> lv_image_dsc_t C arrays via LVGL's own LVGLImage.py (needs
        the pypng and lz4 Python packages). RGB565A8 keeps the icons' alpha
        while staying compact.

The generated files are committed, so building the app needs neither tool.
Run this only after changing a font range or an icon:

    python gen_assets.py [--font-dir DIR] [--image-dir DIR]

Defaults point at the Run app's copies of the same assets.
"""

import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SDK_ROOT = os.path.abspath(os.path.join(HERE, *([".."] * 7)))
RUN_ASSETS = os.path.join(SDK_ROOT, "Examples", "Apps", "Running", "Software", "Apps",
                          "TouchGFX-GUI", "assets")

# Full printable ASCII for text faces; digits and punctuation only for the large
# value-only faces (the 60 px face also needs A/P/M for the 12-hour clock suffix).
# SemiBold 35 stays full ASCII: it is also the selected "Start" item's face.
ASCII = "0x20-0x7E"
NUMERIC = "0x20-0x3A"
CLOCK = "0x20-0x3A,0x41,0x4D,0x50"

FONTS = [
    ("Poppins-Italic",   18, ASCII),
    ("Poppins-Medium",   18, ASCII),
    ("Poppins-Medium",   25, ASCII),
    ("Poppins-Regular",  14, ASCII),
    ("Poppins-Regular",  16, ASCII),
    ("Poppins-Regular",  18, ASCII),
    ("Poppins-SemiBold", 20, ASCII),
    ("Poppins-SemiBold", 25, ASCII),
    ("Poppins-SemiBold", 30, ASCII),
    ("Poppins-SemiBold", 35, ASCII),
    ("Poppins-SemiBold", 40, NUMERIC),
    ("Poppins-SemiBold", 60, CLOCK),
]


def run(cmd):
    print("  " + " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--font-dir", default=os.path.join(RUN_ASSETS, "fonts"))
    ap.add_argument("--image-dir", default=os.path.join(RUN_ASSETS, "images"))
    args = ap.parse_args()

    font_out = os.path.join(HERE, "fonts")
    image_out = os.path.join(HERE, "images")
    os.makedirs(font_out, exist_ok=True)
    os.makedirs(image_out, exist_ok=True)

    npx = shutil.which("npx") or shutil.which("npx.cmd")
    if not npx:
        sys.exit("npx not found: install Node.js to convert fonts")

    for face, size, rng in FONTS:
        name = f"{face.replace('-', '_').lower()}_{size}"
        ttf = os.path.join(args.font_dir, f"{face}.ttf")
        if not os.path.isfile(ttf):
            sys.exit(f"font not found: {ttf}")
        print(f"font  {name}")
        run([npx, "--yes", "lv_font_conv", "--font", ttf, "--size", str(size), "--bpp", "2",
             "--format", "lvgl", "--no-compress", "-r", rng,
             "-o", os.path.join(font_out, f"{name}.c")])

    script = os.path.join(SDK_ROOT, "ThirdParty", "lvgl", "scripts", "LVGLImage.py")
    for png in sorted(os.listdir(args.image_dir)):
        if not png.lower().endswith(".png"):
            continue
        stem = os.path.splitext(png)[0]
        name = "img_" + "".join(c if c.isalnum() else "_" for c in stem).lower()
        print(f"image {name}")
        run([sys.executable, script, "--ofmt", "C", "--cf", "RGB565A8", "--name", name,
             "-o", image_out, os.path.join(args.image_dir, png)])

    n_fonts = len([f for f in os.listdir(font_out) if f.endswith(".c")])
    n_images = len([f for f in os.listdir(image_out) if f.endswith(".c")])
    print(f"done: {n_fonts} fonts, {n_images} images")


if __name__ == "__main__":
    main()
