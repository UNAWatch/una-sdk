#!/usr/bin/env python3
"""Regenerate the RunLVGL GUI's font and image C files.

Fonts:  Poppins faces -> LVGL fonts through lv_font_conv, run via npx so Node is
        the only prerequisite. 2 bits per pixel like the TouchGFX build, and
        uncompressed because lv_conf.h builds LVGL without the decompressor. The
        large numeric faces only carry the glyphs the value fields print.
Images: PNG icons -> lv_image_dsc_t C arrays via LVGL's own LVGLImage.py (needs
        the pypng and lz4 Python packages). Single-colour icons are stored as
        A8 (alpha only, one byte per pixel) and tinted at draw time through
        LVGL's image recolour; two-colour icons keep RGB565A8. Icons that only
        differ in colour from another, and the heart-rate zone group (which the
        GUI composes from the five zone shapes), are not converted at all.

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
# value-only faces. The SemiBold 60 face also needs A/P/M for the 12-hour clock
# suffix and O/p/e/n for the interval timer's "Open" readout. SemiBold 35 stays
# full ASCII: it is also the selected "Start" item's face.
# Pinned: a different converter release could lay the tables out differently,
# and the committed files must regenerate byte for byte.
LV_FONT_CONV_VERSION = "1.5.3"

ASCII = "0x20-0x7E"
NUMERIC = "0x20-0x3A"
BIG = "0x20-0x3A,0x41,0x4D,0x4F,0x50,0x65,0x6E,0x70"

# Single-colour icons (checked with a PNG colour census): stored as alpha only
# and coloured by the GUI. Keys are the lower-cased PNG stem.
ALPHA_IMAGES = {
    "circlecross_50x50", "circletick_50x50", "crosswhite_17x17", "pause_14x14",
    "sensorgpslight", "sensorhrlight", "tickgreen_22x17",
}

# Same shape as another icon in a different colour, or composed in code.
SKIP_IMAGES = {
    "crossamber_17x17", "tickamber_22x17", "tickred_22x17",
    "sensorgpsdark", "sensorhrdark",
    # The heart-rate zone bar and its marker are drawn with lv_arc and a
    # triangle (Widgets.cpp), fitted from these bitmaps.
    "heartratezonegroup",
    "heartratezone1", "heartratezone2", "heartratezone3", "heartratezone4", "heartratezone5",
}

FONTS = [
    ("Poppins-Italic",   18, ASCII),
    ("Poppins-Italic",   20, ASCII),
    ("Poppins-Light",    60, NUMERIC),
    ("Poppins-Medium",   18, ASCII),
    ("Poppins-Medium",   25, ASCII),
    ("Poppins-Medium",   40, NUMERIC),
    ("Poppins-Regular",  14, ASCII),
    ("Poppins-Regular",  16, ASCII),
    ("Poppins-Regular",  18, ASCII),
    ("Poppins-SemiBold", 20, ASCII),
    ("Poppins-SemiBold", 25, ASCII),
    ("Poppins-SemiBold", 30, ASCII),
    ("Poppins-SemiBold", 35, ASCII),
    ("Poppins-SemiBold", 40, NUMERIC),
    ("Poppins-SemiBold", 60, BIG),
]


def run(cmd, cwd=None):
    print("  " + " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=cwd)


def sdk_relative(path):
    """Path relative to the SDK root with forward slashes, so the tools' own
    records of their arguments read the same on every machine."""
    return os.path.relpath(path, SDK_ROOT).replace(os.sep, "/")


def normalise_newlines(directory):
    """The converters write native line endings; the committed files use LF so
    a regeneration on another OS reproduces them byte for byte."""
    for name in os.listdir(directory):
        if name.endswith(".c"):
            path = os.path.join(directory, name)
            with open(path, "rb") as f:
                data = f.read()
            fixed = data.replace(b"\r\n", b"\n")
            if fixed != data:
                with open(path, "wb") as f:
                    f.write(fixed)


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
        # lv_font_conv records its arguments in the file header; run it from
        # the SDK root with relative paths so that header is reproducible.
        run([npx, "--yes", f"lv_font_conv@{LV_FONT_CONV_VERSION}",
             "--font", sdk_relative(ttf), "--size", str(size), "--bpp", "2",
             "--format", "lvgl", "--no-compress", "-r", rng,
             "-o", sdk_relative(os.path.join(font_out, f"{name}.c"))], cwd=SDK_ROOT)

    script = os.path.join(SDK_ROOT, "ThirdParty", "lvgl", "scripts", "LVGLImage.py")
    for png in sorted(os.listdir(args.image_dir)):
        if not png.lower().endswith(".png"):
            continue
        stem = os.path.splitext(png)[0]
        key = stem.lower()
        if key in SKIP_IMAGES:
            continue
        name = "img_" + "".join(c if c.isalnum() else "_" for c in stem).lower()
        cf = "A8" if key in ALPHA_IMAGES else "RGB565A8"
        print(f"image {name} ({cf})")
        run([sys.executable, script, "--ofmt", "C", "--cf", cf, "--name", name,
             "-o", image_out, os.path.join(args.image_dir, png)])

    normalise_newlines(font_out)
    normalise_newlines(image_out)

    n_fonts = len([f for f in os.listdir(font_out) if f.endswith(".c")])
    n_images = len([f for f in os.listdir(image_out) if f.endswith(".c")])
    print(f"done: {n_fonts} fonts, {n_images} images")


if __name__ == "__main__":
    main()
