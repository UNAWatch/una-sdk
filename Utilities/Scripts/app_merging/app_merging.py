import struct
import zlib
import logging
import argparse
from pathlib import Path
import sys
import re
from PIL import Image

# ---------- helpers ----------
def verify_uapp_crc(filepath: Path) -> bytes:
    """Read file, verify last 4 bytes as little-endian CRC32 of the content, return content without CRC."""
    with open(filepath, "rb") as f:
        data = f.read()
    if len(data) < 4:
        raise ValueError(f"{filepath} is too short to contain a valid CRC.")
    content, expected_crc_bytes = data[:-4], data[-4:]
    expected_crc = struct.unpack("<I", expected_crc_bytes)[0]
    actual_crc = zlib.crc32(content) & 0xFFFFFFFF
    if actual_crc != expected_crc:
        raise ValueError(f"{filepath.name} failed CRC check! Expected 0x{expected_crc:08X}, got 0x{actual_crc:08X}")
    logging.debug(f"[OK] 0x{actual_crc:08X} check CRC passed: {filepath}")
    return content

APP_TYPES = {
    "Activity"  : 0,
    "Utility"   : 1,
    "Glance"    : 2,
    "Clockface" : 3
}

def convert_icon_to_abgr2222(png_path: Path) -> bytes:
    """Convert RGBA PNG to ABGR2222 (rotate 90 degrees, pack 2 bits per channel)."""
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
            bmp_data.append((a << 6) | (b << 4) | (g << 2) | r)
    return bytes(bmp_data)

def parse_appid_64(s: str) -> int:
    """Expect exactly 16 hex characters -> uint64."""
    s = s.strip()
    if len(s) != 16 or not re.fullmatch(r"[0-9a-fA-F]{16}", s):
        raise argparse.ArgumentTypeError("AppID must be exactly 16 hex characters (e.g., 0123ABCD89EF4560).")
    return int(s, 16)

def parse_semver_u32(s: str) -> tuple[int, str]:
    """Parse 'A.B.C' into uint32: 0x00AABBCC (A=major, B=minor, C=patch), allowing pre-release suffixes."""
    s = s.strip().lstrip('vV')
    m = re.match(r"(\d+)\.(\d+)\.(\d+)", s)
    if not m:
        raise argparse.ArgumentTypeError("Version must start with A.B.C format (e.g., 1.2.3 or 1.2.3-rc1).")
    a, b, c = (int(m.group(i)) for i in (1, 2, 3))
    for v, name in [(a, "A"), (b, "B"), (c, "C")]:
        if not (0 <= v <= 255):
            raise argparse.ArgumentTypeError(f"Version component {name} must be 0..255.")
    return (a << 16) | (b << 8) | c, s

def find_first_main_ld(scripts_root: Path) -> Path:
    """Pick the first linker script under <scripts_root>/linker/Main (lexicographically sorted)."""
    main_dir = scripts_root / "linker" / "Main"
    if not main_dir.is_dir():
        raise argparse.ArgumentTypeError(f"Directory not found: {main_dir}")
    candidates = sorted(p for p in main_dir.glob("*.ld") if p.is_file())
    if not candidates:
        raise argparse.ArgumentTypeError(f"No .ld files found in {main_dir}")
    return candidates[0]

def parse_libc_version_from_include(main_ld_path: Path) -> int:
    """Parse INCLUDE line with LibC version from the main linker script."""
    text = main_ld_path.read_text(encoding="utf-8", errors="ignore")
    m = re.search(
        r'^\s*INCLUDE\s+"LibC[\\/]+libc_exports_(\d+)\.(\d+)\.(\d+)\.ld"\s*$',
        text, flags=re.MULTILINE
    )
    if not m:
        raise argparse.ArgumentTypeError(
            f'Cannot find INCLUDE "LibC/libc_exports_A.B.C.ld" in {main_ld_path}'
        )
    a, b, c = map(int, m.groups())
    return (a << 16) | (b << 8) | c

def format_semver_u32(v: int) -> str:
    """Return string A.B.C from packed uint32 0x00AABBCC."""
    a = (v >> 16) & 0xFF
    b = (v >> 8)  & 0xFF
    c =  v        & 0xFF
    return f"{a}.{b}.{c}"

def make_file_safe_name(name: str, fallback: str = "app") -> str:
    if not name:
        return fallback

    safe = name.strip()
    safe = re.sub(r'\s+', '_', safe)
    safe = re.sub(r'[^\w.-]', '_', safe, flags=re.UNICODE)
    safe = re.sub(r'_+', '_', safe)
    safe = safe.strip(' ._')

    reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
    }
    if safe.upper() in reserved:
        safe = f"_{safe}"

    return safe or fallback

logging.basicConfig(level=logging.INFO)

# ---------- args ----------
parser = argparse.ArgumentParser(description="Merge Service and GUI images with extended MainHeader_t header")
parser.add_argument("-name", required=True, help="Application name (used in output file)")
parser.add_argument("-autostart", action="store_true", help="Set bit 3 (0x08) in flags for autostart")
parser.add_argument("-glance_capable", action="store_true",
                    help="Set bit 5 (0x20) in flags to mark the app as Glance-capable")
parser.add_argument("-type", required=True, choices=list(APP_TYPES.keys()), help="Application type")
parser.add_argument("-out", type=str, help="Custom output directory (also the place where Tmp/*.srv and Tmp/*.gui are searched)")
parser.add_argument("-header", action="store_true", help="Generate .h file with merged binary as C array")
parser.add_argument("-normal_icon", type=Path, required=True, help="Path to normal 60x60 icon PNG")
parser.add_argument("-small_icon", type=Path, required=True, help="Path to small 30x30 icon PNG")
parser.add_argument("-appid", type=parse_appid_64, required=True, help="16-hex AppID (e.g., 0123ABCD89EF4560)")
parser.add_argument("-appver", type=parse_semver_u32, required=True, help="App version A.B.C (e.g., 1.2.3)")
parser.add_argument("-scripts", type=Path, required=True, help="Path to SDK Utilities/Scripts (PATH_SCRIPTS)")
args = parser.parse_args()

# Validate icon sizes
if Image.open(args.normal_icon).size != (60, 60):
    raise ValueError(f"Normal icon must be 60x60, got {Image.open(args.normal_icon).size}")
if Image.open(args.small_icon).size != (30, 30):
    raise ValueError(f"Small icon must be 30x30, got {Image.open(args.small_icon).size}")

# Flags
flags = APP_TYPES[args.type]
if args.autostart:
    flags |= 0x00000008  # bit 3
if args.glance_capable:
    flags |= 0x00000020  # bit 5

# ---------- input discovery under <out>/Tmp ----------
out_dir = Path(args.out) if args.out else Path("Output")
tmp_dir = out_dir / "Tmp"

def pick_newest(pattern: str):
    """Return newest file in tmp_dir matching pattern or None."""
    files = list(tmp_dir.glob(pattern))
    if not files:
        return None
    return max(files, key=lambda f: f.stat().st_mtime)

srv_path = pick_newest("*.srv")
gui_path = pick_newest("*.gui")

if not srv_path:
    print(f"Missing .srv file in {tmp_dir}")
    sys.exit(2)

# GUI presence rule:
# - GUI may be absent for type Glance
gui_optional = (args.type == "Glance")
if not gui_path and not gui_optional:
    print(f"Missing .gui file in {tmp_dir} (required for type {args.type})")
    sys.exit(2)

service_data = verify_uapp_crc(srv_path)
gui_data     = verify_uapp_crc(gui_path) if gui_path else b""
normal_icon  = convert_icon_to_abgr2222(args.normal_icon)
small_icon   = convert_icon_to_abgr2222(args.small_icon)

service_size       = len(service_data)
display_name       = args.name
file_name_base     = make_file_safe_name(display_name)
name_bytes         = display_name.encode('utf-8')[:15].ljust(16, b'\0')
normal_icon_size   = len(normal_icon)
small_icon_size    = len(small_icon)

# Build extra fields
app_id_u64                      = args.appid
app_version_u32, app_version    = args.appver
main_ld_path                    = find_first_main_ld(args.scripts)
libc_version_u32                = parse_libc_version_from_include(main_ld_path)

# ---------- header ----------
# [AppID u64][AppVersion u32][LibCVersion u32][service_size u32][flags u32]
# [AppName char[16]][normal_icon_size u32][small_icon_size u32]
prefix = struct.pack("<QII", app_id_u64, app_version_u32, libc_version_u32)
oldhdr = struct.pack("<II16sII", service_size, flags, name_bytes, normal_icon_size, small_icon_size)
header = prefix + oldhdr

# ---------- blob + CRC ----------
# GUI payload is appended only if present (non-Glance or Glance with .gui provided).
blob_without_crc = header + normal_icon + small_icon + service_data + gui_data
crc = zlib.crc32(blob_without_crc) & 0xFFFFFFFF
final_data = blob_without_crc + struct.pack("<I", crc)

# ---------- output ----------
out_dir.mkdir(parents=True, exist_ok=True)
output_path = out_dir / f"{file_name_base}_{app_version}.uapp"
with open(output_path, "wb") as f:
    f.write(final_data)

logging.info("Merge complete")
logging.info(f"Service file   : {srv_path}")
if gui_path:
    logging.info(f"GUI file       : {gui_path}")
else:
    logging.info("GUI file       : (absent, allowed for type Glance)")
logging.info(f"Name           : {args.name}")
logging.info("ID             : %016X", app_id_u64)
logging.info("App Version    : %s", format_semver_u32(app_version_u32))
logging.info("LibC Version   : %s", format_semver_u32(libc_version_u32))
logging.info(f"Flags          : 0x{flags:08X}")
logging.info(f"Glance-capable : {'yes' if args.glance_capable else 'no'}")
logging.info(f"CRC            : 0x{crc:08X}")
logging.info(f"Image          : {output_path} ({len(final_data)} bytes)")

# ---------- optional .h ----------
if args.header:
    header_filename = f"{file_name_base}_{app_version}.h"
    header_path = output_path.parent / header_filename
    macro_guard = '__' + header_path.stem.upper().replace('.', '_').replace('-', '_') + '_H__'
    array_name = f"{args.name}_merged".replace("-", "_")
    with open(header_path, "w") as f:
        f.write(f"#ifndef {macro_guard}\n#define {macro_guard}\n\n#include <stdint.h>\n\n")
        f.write(f"const uint8_t {array_name}[] = {{\n    ")
        for i, byte in enumerate(final_data):
            f.write(f"0x{byte:02X}, ")
            if (i + 1) % 12 == 0:
                f.write("\n    ")
        f.write(f"\n}};\n\n#endif // {macro_guard}\n")

print("")
