import struct
import glob
import zlib
import argparse
from pathlib import Path
import sys
from PIL import Image

def verify_uapp_crc(filepath: Path) -> bytes:
    with open(filepath, "rb") as f:
        data = f.read()

    if len(data) < 4:
        raise ValueError(f"{filepath} is too short to contain a valid CRC.")

    content, expected_crc_bytes = data[:-4], data[-4:]
    expected_crc = struct.unpack("<I", expected_crc_bytes)[0]
    actual_crc = zlib.crc32(content) & 0xFFFFFFFF

    if actual_crc != expected_crc:
        raise ValueError(f"{filepath.name} failed CRC check! Expected 0x{expected_crc:08X}, got 0x{actual_crc:08X}")

    print(f"[OK] 0x{actual_crc:08X} check CRC passed: {filepath}")
    return content

# Header format: uint32_t serviceSize, uint32_t flags, uint32_t crc
HEADER_FORMAT = "<II16sII"

# App type to flag value mapping
APP_TYPES = {
    "Activity"  : 0,
    "Utility"   : 1,
    "Glance"    : 2,
    "Clockface" : 3
}

def convert_icon_to_abgr2222(png_path: Path) -> bytes:
    """
    Converts a PNG image to ABGR2222 format (square input required).
    """
    img = Image.open(png_path).convert("RGBA")

    if img.width != img.height:
        raise ValueError("Image must be square")

    img = img.rotate(90, expand=True)
    bmp_data = bytearray()

    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = img.getpixel((x, y))
            a = (a >> 6) & 0x03
            b = (b >> 6) & 0x03
            g = (g >> 6) & 0x03
            r = (r >> 6) & 0x03
            packed_pixel = (a << 6) | (b << 4) | (g << 2) | r
            bmp_data.append(packed_pixel)

    return bytes(bmp_data)

# Argument parsing
parser = argparse.ArgumentParser(description="Merge Service and GUI .uapp files with MainHeader_t header")
parser.add_argument("-name", required=True, help="Application name (used in output file)")
parser.add_argument("-autostart", action="store_true", help="Set bit 3 (0x08) in flags for autostart")
parser.add_argument("-type", required=True, help="Application type: Activity, Utility, Glance, Clockface")
parser.add_argument("-out", type=str, help="Custom output file path (optional)")
parser.add_argument("-header", action="store_true", help="Generate .h file with uapp binary as C array")
parser.add_argument("-normal_icon", type=Path, required=True, help="Path to normal 60x60 icon PNG")
parser.add_argument("-small_icon", type=Path, required=True, help="Path to small 30x30 icon PNG")
args = parser.parse_args()

# Explicit dimension validation after parsing
if Image.open(args.normal_icon).size != (60, 60):
    raise ValueError(f"Normal icon must be 60x60, got {Image.open(args.normal_icon).size}")
if Image.open(args.small_icon).size != (30, 30):
    raise ValueError(f"Small icon must be 30x30, got {Image.open(args.small_icon).size}")

# Validate app type
if args.type not in APP_TYPES:
    print(f"Unknown app type: {args.type}")
    sys.exit(1)

# Construct flags
flags = APP_TYPES[args.type]
if args.autostart:
    flags |= 0x00000008  # Set bit 3

# Find .uapp files
service_files = glob.glob("../../../../../Service/Output/userapp*.uapp")
gui_files = glob.glob("../../../../../GUI/Output/userapp*.uapp")

if not service_files:
    print("Missing .uapp files for Service")
    sys.exit(2)

if not gui_files:
    print("Missing .uapp files for GUI")
    sys.exit(2)

# Read and validate file contents
service_data = verify_uapp_crc(Path(service_files[0]))
gui_data = verify_uapp_crc(Path(gui_files[0]))
normal_icon = convert_icon_to_abgr2222(args.normal_icon)
small_icon = convert_icon_to_abgr2222(args.small_icon)

service_size = len(service_data)

# Header with name and icon sizes
name_bytes = args.name.encode('utf-8')[:15].ljust(16, b'\0')
normal_icon_size = len(normal_icon)
small_icon_size = len(small_icon)
header = struct.pack(HEADER_FORMAT, service_size, flags, name_bytes, normal_icon_size, small_icon_size)

# Final binary blob (header + icons + data)
blob_without_crc = header + normal_icon + small_icon + service_data + gui_data

# Compute CRC of the whole blob
crc = zlib.crc32(blob_without_crc) & 0xFFFFFFFF

# Append CRC at the end of file
final_data = blob_without_crc + struct.pack("<I", crc)

# Determine output file name
output_file = args.out if args.out else f"Output/{args.name}.uapp"
output_path = Path(output_file)
output_path.parent.mkdir(parents=True, exist_ok=True)

# Write to output file
with open(output_path, "wb") as f:
    f.write(final_data)

# Report
print(f"\nMerge complete : {output_path} ({len(final_data)} bytes)")
print(f"App name        : {args.name}")
print(f"flags           : 0x{flags:08X}")
print(f"crc             : 0x{crc:08X}")

# Optional: generate .h file with C array
if args.header:
    header_filename = f"{args.name}_merged.h"
    header_path = output_path.parent / header_filename
    macro_guard = '__' + header_path.stem.upper().replace('.', '_').replace('-', '_') + '_H__'
    array_name = f"{args.name}_merged".replace("-", "_")

    with open(header_path, "w") as f:
        f.write(f"#ifndef {macro_guard}\n")
        f.write(f"#define {macro_guard}\n\n")
        f.write(f"#include <stdint.h>\n\n")
        f.write(f"const uint8_t {array_name}[] = {{\n    ")
        for i, byte in enumerate(final_data):
            f.write(f"0x{byte:02X}, ")
            if (i + 1) % 12 == 0:
                f.write("\n    ")
        f.write("\n};\n\n")
        f.write(f"#endif // {macro_guard}\n")

    print(f"Header file     : {header_path}")
