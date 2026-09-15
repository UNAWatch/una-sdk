#!/usr/bin/env python3
"""Convert an LVGL app's fonts and images to C files from a manifest.

Usage:
    python lvgl_assets.py path/to/assets.json

The manifest sits in the app's assets directory and lists what to convert;
the C files land in fonts/ and images/ next to it and are committed, so
building the app needs neither converter. Run this only after changing a font
range or an icon.

    {
      "fonts": [
        {"name": "poppins_regular_18", "file": "../../TouchGFX-GUI/assets/fonts/Poppins-Regular.ttf",
         "size": 18, "range": "0x20-0x7E", "bpp": 2}
      ],
      "images": [
        {"name": "img_tick", "file": "../../TouchGFX-GUI/assets/images/tick.png", "format": "A8"}
      ]
    }

Fonts go through lv_font_conv, run via npx so Node.js is the only
prerequisite. "bpp" defaults to 2 (the TouchGFX apps' setting); fonts are
uncompressed because the SDK's lv_conf.h builds LVGL without the decompressor.
"range" is lv_font_conv's -r syntax; keep the large numeric faces to the
glyphs they print.

Images go through LVGL's own LVGLImage.py (needs the pypng and lz4 Python
packages). "format" is the LVGL colour format: A8 for a single-colour icon
stored as an alpha mask and tinted at draw time (SDK::LVGL::Draw::imageTinted),
RGB565A8 for a multi-colour one. Indexed formats are not a saving on the
watch: LVGL v9 decodes them to 32-bit ARGB at draw time.

Paths in the manifest are relative to the manifest file. The converters record
their arguments in the generated files, so they are run from the SDK root with
SDK-relative paths, and the output is normalised to LF; a regeneration on
another machine or OS then reproduces the committed files byte for byte.
"""

import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SDK_ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
LVGL_IMAGE_SCRIPT = os.path.join(SDK_ROOT, "ThirdParty", "lvgl", "scripts", "LVGLImage.py")


def run(cmd, cwd=None):
    print("  " + " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=cwd)


def sdk_relative(path):
    """Path relative to the SDK root with forward slashes, so the tools' own
    records of their arguments read the same on every machine."""
    return os.path.relpath(path, SDK_ROOT).replace(os.sep, "/")


def normalise_newlines(directory):
    """The converters write native line endings; the committed files use LF."""
    if not os.path.isdir(directory):
        return
    for name in os.listdir(directory):
        if name.endswith(".c"):
            path = os.path.join(directory, name)
            with open(path, "rb") as f:
                data = f.read()
            fixed = data.replace(b"\r\n", b"\n")
            if fixed != data:
                with open(path, "wb") as f:
                    f.write(fixed)


def convert_fonts(fonts, base, out_dir):
    if not fonts:
        return 0
    npx = shutil.which("npx") or shutil.which("npx.cmd")
    if not npx:
        sys.exit("npx not found: install Node.js to convert fonts")
    os.makedirs(out_dir, exist_ok=True)
    for font in fonts:
        ttf = os.path.normpath(os.path.join(base, font["file"]))
        if not os.path.isfile(ttf):
            sys.exit(f"font not found: {ttf}")
        name = font["name"]
        print(f"font  {name}")
        run([npx, "--yes", "lv_font_conv",
             "--font", sdk_relative(ttf),
             "--size", str(font["size"]),
             "--bpp", str(font.get("bpp", 2)),
             "--format", "lvgl", "--no-compress",
             "-r", font.get("range", "0x20-0x7E"),
             "-o", sdk_relative(os.path.join(out_dir, f"{name}.c"))],
            cwd=SDK_ROOT)
    return len(fonts)


def convert_images(images, base, out_dir):
    if not images:
        return 0
    if not os.path.isfile(LVGL_IMAGE_SCRIPT):
        sys.exit(f"{LVGL_IMAGE_SCRIPT} not found: run 'git submodule update --init ThirdParty/lvgl'")
    os.makedirs(out_dir, exist_ok=True)
    for image in images:
        png = os.path.normpath(os.path.join(base, image["file"]))
        if not os.path.isfile(png):
            sys.exit(f"image not found: {png}")
        name = image["name"]
        cf = image.get("format", "RGB565A8")
        print(f"image {name} ({cf})")
        run([sys.executable, sdk_relative(LVGL_IMAGE_SCRIPT), "--ofmt", "C", "--cf", cf, "--name", name,
             "-o", sdk_relative(out_dir), sdk_relative(png)], cwd=SDK_ROOT)
    return len(images)


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    manifest_path = os.path.abspath(sys.argv[1])
    with open(manifest_path, encoding="utf-8") as f:
        manifest = json.load(f)
    base = os.path.dirname(manifest_path)
    font_out = os.path.join(base, "fonts")
    image_out = os.path.join(base, "images")

    n_fonts = convert_fonts(manifest.get("fonts", []), base, font_out)
    n_images = convert_images(manifest.get("images", []), base, image_out)
    normalise_newlines(font_out)
    normalise_newlines(image_out)
    print(f"done: {n_fonts} fonts, {n_images} images")


if __name__ == "__main__":
    main()
