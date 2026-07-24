"""Build a code-less variant-alias .uapp (see Docs/Multi-Activity-Apps-Design.md).

An alias reuses the real app's MainHeader layout, discriminated by flag bit
0x40 (FLAG_APP_VARIANT_ALIAS), carries no code (serviceSize = 0, libcVersion
= 0), and embeds the app-facing variant config after a fixed-size payload:

    [MainHeader 48][normal icon 3600][small icon 900]
    [VariantAliasPayload 32][config JSON, configSize bytes][CRC32 4]

The emitted file is the byte-level contract shared with the kernel validator
(Validator::aliasIsValid) and the phone; the layout is locked by host tests
in the kernel repo (Tests/Host/kernel/VariantAlias_test.cpp).
"""

import argparse
import json
import logging
import re
import struct
import sys
import zlib
from pathlib import Path

APP_TYPES = {
    "Activity"  : 0,
    "Utility"   : 1,
    "Glance"    : 2,
    "Clockface" : 3
}

FLAG_APP_VARIANT_ALIAS = 0x40

MAIN_HEADER_SIZE  = 48
NORMAL_ICON_SIZE  = 60 * 60   # ABGR2222: 1 byte per pixel
SMALL_ICON_SIZE   = 30 * 30
PAYLOAD_VERSION   = 1
CONFIG_SIZE_MAX   = 8192

ORIGINS = {"shipped": 0, "user": 1}


def parse_appid_64(s: str) -> int:
    s = s.strip()
    if len(s) != 16 or not re.fullmatch(r"[0-9a-fA-F]{16}", s):
        raise argparse.ArgumentTypeError("AppID must be exactly 16 hex characters (e.g., 0123ABCD89EF4560).")
    return int(s, 16)


def parse_semver_u32(s: str) -> tuple[int, str]:
    s = s.strip().lstrip('vV')
    m = re.match(r"(\d+)\.(\d+)\.(\d+)", s)
    if not m:
        raise argparse.ArgumentTypeError("Version must start with A.B.C format (e.g., 1.2.3).")
    a, b, c = (int(m.group(i)) for i in (1, 2, 3))
    for v, name in [(a, "A"), (b, "B"), (c, "C")]:
        if not (0 <= v <= 255):
            raise argparse.ArgumentTypeError(f"Version component {name} must be 0..255.")
    return (a << 16) | (b << 8) | c, s


def make_file_safe_name(name: str, fallback: str = "variant") -> str:
    safe = re.sub(r'_+', '_', re.sub(r'[^\w.-]', '_', re.sub(r'\s+', '_', name.strip()), flags=re.UNICODE)).strip(' ._')
    return safe or fallback


def convert_icon_to_abgr2222(png_path: Path) -> bytes:
    """Convert RGBA PNG to ABGR2222 (same packing as app_merging.py)."""
    from PIL import Image
    img = Image.open(png_path).convert("RGBA")
    if img.width != img.height:
        raise ValueError("Image must be square")
    bmp_data = bytearray()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = img.getpixel((x, y))
            bmp_data.append((((a >> 6) & 3) << 6) | (((b >> 6) & 3) << 4) | (((g >> 6) & 3) << 2) | ((r >> 6) & 3))
    return bytes(bmp_data)


def icons_from_uapp(uapp_path: Path) -> tuple[bytes, bytes]:
    """Extract both icons from an existing .uapp (real app or alias) at the
    fixed post-MainHeader offsets -- the same copy the kernel's CreateVariant
    handler performs for user-created variants."""
    data = uapp_path.read_bytes()
    need = MAIN_HEADER_SIZE + NORMAL_ICON_SIZE + SMALL_ICON_SIZE
    if len(data) < need:
        raise ValueError(f"{uapp_path} is too short to contain the standard icons")
    normal = data[MAIN_HEADER_SIZE:MAIN_HEADER_SIZE + NORMAL_ICON_SIZE]
    small = data[MAIN_HEADER_SIZE + NORMAL_ICON_SIZE:need]
    return normal, small


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(message)s")

    parser = argparse.ArgumentParser(description="Build a code-less variant-alias .uapp")
    parser.add_argument("-name", required=True, help="Variant launcher name (max 15 chars, ASCII)")
    parser.add_argument("-appid", type=parse_appid_64, required=True,
                        help="The variant's OWN unique 16-hex AppID (not the target's)")
    parser.add_argument("-target_appid", type=parse_appid_64, required=True,
                        help="16-hex AppID of the base app this variant runs")
    parser.add_argument("-config", type=Path, required=True,
                        help="Path to the variant config JSON to embed")
    parser.add_argument("-type", required=True, choices=list(APP_TYPES.keys()),
                        help="Application type (must match the target's)")
    parser.add_argument("-appver", type=parse_semver_u32, default=(0x00010000, "1.0.0"),
                        help="Alias content revision A.B.C (default 1.0.0)")
    parser.add_argument("-min_target_version", type=parse_semver_u32, default=(0, "0.0.0"),
                        help="Minimum target app version A.B.C (default: any)")
    parser.add_argument("-origin", choices=list(ORIGINS.keys()), default="shipped",
                        help="Who owns this variant dir on update (default: shipped)")
    parser.add_argument("-normal_icon", type=Path, default=None, help="Path to normal 60x60 icon PNG")
    parser.add_argument("-small_icon", type=Path, default=None, help="Path to small 30x30 icon PNG")
    parser.add_argument("-icons_from", type=Path, default=None,
                        help="Copy both icons out of an existing .uapp instead of PNGs")
    parser.add_argument("-out", type=Path, default=Path("Output"), help="Output directory")
    args = parser.parse_args()

    name_utf8 = args.name.encode("utf-8")
    if len(name_utf8) > 15:
        parser.error("-name must fit 15 bytes (NUL-terminated 16-byte field)")
    if not args.name.isascii():
        parser.error("-name must be ASCII (the wildcard fonts cover ASCII only)")
    if args.appid == args.target_appid:
        parser.error("-appid must differ from -target_appid (an alias cannot claim "
                     "the target's own identity; the kernel rejects the collision at scan)")

    if args.icons_from is not None:
        if args.normal_icon or args.small_icon:
            parser.error("-icons_from and -normal_icon/-small_icon are mutually exclusive")
        normal_icon, small_icon = icons_from_uapp(args.icons_from)
    else:
        if args.normal_icon is None or args.small_icon is None:
            parser.error("provide -normal_icon and -small_icon, or -icons_from")
        normal_icon = convert_icon_to_abgr2222(args.normal_icon)
        small_icon = convert_icon_to_abgr2222(args.small_icon)

    if len(normal_icon) != NORMAL_ICON_SIZE or len(small_icon) != SMALL_ICON_SIZE:
        raise ValueError("Icon payloads must be exactly 3600 / 900 bytes")

    config_bytes = args.config.read_bytes()
    config_json = json.loads(config_bytes)  # authoring-side syntax check; the kernel never parses it
    if config_json.get("schema") != 1:
        parser.error('config JSON must carry "schema": 1 -- the app-side reader '
                     "falls back to classic defaults on anything else, so the "
                     "variant would build but never activate")
    if len(config_bytes) > CONFIG_SIZE_MAX:
        raise ValueError(f"Config exceeds the kernel's {CONFIG_SIZE_MAX}-byte bound")

    app_version_u32, app_version = args.appver
    min_target_u32, min_target = args.min_target_version
    flags = APP_TYPES[args.type] | FLAG_APP_VARIANT_ALIAS

    # MainHeader: [AppID u64][AppVersion u32][LibCVersion u32 = 0]
    #             [service_size u32 = 0][flags u32][AppName char[16]]
    #             [normal_icon_size u32][small_icon_size u32]
    header = struct.pack("<QIIII16sII",
                         args.appid, app_version_u32, 0,
                         0, flags, name_utf8.ljust(16, b"\0"),
                         NORMAL_ICON_SIZE, SMALL_ICON_SIZE)

    # VariantAliasPayload: [payloadVersion u32][targetAppID u64]
    #                      [minTargetVersion u32][origin u8]
    #                      [configSize u32][reserved u8[11]]
    payload = struct.pack("<IQIBI11s",
                          PAYLOAD_VERSION, args.target_appid,
                          min_target_u32, ORIGINS[args.origin],
                          len(config_bytes), b"\0" * 11)

    blob = header + normal_icon + small_icon + payload + config_bytes
    final = blob + struct.pack("<I", zlib.crc32(blob) & 0xFFFFFFFF)

    args.out.mkdir(parents=True, exist_ok=True)
    output_path = args.out / f"{make_file_safe_name(args.name)}_{app_version}.uapp"
    output_path.write_bytes(final)

    logging.info(f"Name            : {args.name}")
    logging.info(f"ID              : {args.appid:016X}")
    logging.info(f"Target ID       : {args.target_appid:016X}")
    logging.info(f"Alias revision  : {app_version}")
    logging.info(f"Min target ver  : {min_target}")
    logging.info(f"Origin          : {args.origin}")
    logging.info(f"Flags           : 0x{flags:08X}")
    logging.info(f"Config          : {args.config} ({len(config_bytes)} bytes)")
    logging.info(f"Alias           : {output_path} ({len(final)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
