"""Pack every shipped variant under Examples/Apps/Variants/ (CI + local driver).

For each `Variants/<Name>/manifest.json` this script locates the freshly built
target `.uapp`, enforces the AppID allocation rule (a variant's uappID must
collide with nothing: any built app or any other manifest — see
Examples/Apps/Variants/README.md), drives `make_variant.py` to pack the alias,
and structurally verifies the emitted file against the on-disk contract the
kernel validator enforces (layout offsets, flag, sizes, trailing CRC).

Usage:
    pack_variants.py --apps-root Examples/Apps --out Output/variants
        [--built-apps <dir>]    # where to find built .uapp files
                                # (default: search app build dirs under apps-root;
                                #  in CI, point it at the downloaded artifacts)

Exit codes: 0 = all variants packed + verified, 1 = any failure.
"""

import argparse
import json
import struct
import subprocess
import sys
import zlib
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent

MAIN_HEADER_SIZE = 48
ICONS_SIZE = 3600 + 900
PAYLOAD_OFFSET = MAIN_HEADER_SIZE + ICONS_SIZE
PAYLOAD_SIZE = 32
FLAG_APP_VARIANT_ALIAS = 0x40


def fail(msg: str) -> None:
    print(f"pack_variants: ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def uappid_of(uapp: Path) -> int:
    with open(uapp, "rb") as f:
        return struct.unpack("<Q", f.read(8))[0]


def find_built_uapp(name: str, search_roots: list[Path]) -> Path | None:
    """Newest <Name>*.uapp under any search root (CI artifacts or build dirs)."""
    candidates = []
    for root in search_roots:
        candidates += list(root.rglob(f"{name}_*.uapp")) + list(root.rglob(f"{name}.uapp"))
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


def verify_alias(path: Path, expected_appid: int, expected_target: int) -> None:
    """Structural check mirroring the kernel's Validator::aliasIsValid rules."""
    data = path.read_bytes()
    if len(data) < PAYLOAD_OFFSET + PAYLOAD_SIZE + 4:
        fail(f"{path.name}: too short ({len(data)} bytes)")

    (uappid, _ver, libc, service_size, flags) = struct.unpack_from("<QIIII", data, 0)
    if uappid != expected_appid:
        fail(f"{path.name}: header uappID {uappid:016X} != manifest {expected_appid:016X}")
    if libc != 0 or service_size != 0:
        fail(f"{path.name}: alias must carry libcVersion=0 and serviceSize=0")
    if not (flags & FLAG_APP_VARIANT_ALIAS):
        fail(f"{path.name}: FLAG_APP_VARIANT_ALIAS missing (flags=0x{flags:02X})")

    (payload_ver, target, _min_ver, _origin, config_size) = \
        struct.unpack_from("<IQIBI", data, PAYLOAD_OFFSET)
    if payload_ver != 1:
        fail(f"{path.name}: unexpected payloadVersion {payload_ver}")
    if target != expected_target:
        fail(f"{path.name}: payload target {target:016X} != built target {expected_target:016X}")
    if len(data) != PAYLOAD_OFFSET + PAYLOAD_SIZE + config_size + 4:
        fail(f"{path.name}: file size does not match configSize {config_size}")

    stored_crc = struct.unpack_from("<I", data, len(data) - 4)[0]
    if zlib.crc32(data[:-4]) & 0xFFFFFFFF != stored_crc:
        fail(f"{path.name}: trailing CRC32 mismatch")

    print(f"  verified {path.name}: appid={uappid:016X} target={target:016X} "
          f"config={config_size}B crc=0x{stored_crc:08X}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apps-root", type=Path, required=True,
                        help="Examples/Apps directory (manifests live in Variants/)")
    parser.add_argument("--built-apps", type=Path, default=None,
                        help="Directory searched recursively for built .uapp files "
                             "(default: the apps-root itself, i.e. local build dirs)")
    parser.add_argument("--only", default=None,
                        help="Pack a single variant by directory name (the AppID "
                             "uniqueness check still runs over everything)")
    parser.add_argument("--out", type=Path, required=True, help="Output directory")
    args = parser.parse_args()

    variants_root = args.apps_root / "Variants"
    manifests = sorted(variants_root.glob("*/manifest.json"))
    if not manifests:
        print(f"pack_variants: no manifests under {variants_root}, nothing to do")
        return 0

    search_roots = [args.built_apps if args.built_apps else args.apps_root]

    # ---- AppID allocation rule: collide with nothing ------------------------
    ids: dict[int, str] = {}
    for uapp in search_roots[0].rglob("*.uapp"):
        ids.setdefault(uappid_of(uapp), f"built app {uapp.name}")
    for m in manifests:
        spec = json.loads(m.read_text())
        appid = int(spec["appid"], 16)
        if appid in ids:
            fail(f"{m.parent.name}: appid {spec['appid']} collides with {ids[appid]}")
        ids[appid] = f"variant {m.parent.name}"

    # ---- pack + verify -------------------------------------------------------
    if args.only is not None:
        manifests = [m for m in manifests if m.parent.name == args.only]
        if not manifests:
            fail(f"--only {args.only}: no such variant under {variants_root}")

    args.out.mkdir(parents=True, exist_ok=True)
    for m in manifests:
        spec = json.loads(m.read_text())
        name = spec["name"]
        print(f"packing variant {name} (from {m})")

        target_uapp = find_built_uapp(spec["target"], search_roots)
        if target_uapp is None:
            fail(f"{name}: no built .uapp found for target '{spec['target']}' "
                 f"under {search_roots[0]}")
        print(f"  target binary: {target_uapp}")

        cmd = [sys.executable, str(SCRIPT_DIR / "make_variant.py"),
               "-name", name,
               "-appid", spec["appid"],
               "-target_uapp", str(target_uapp),
               "-config", str(m.parent / spec["config"]),
               "-type", spec["type"],
               "-appver", spec["appver"],
               "-min_target_version", spec.get("min_target_version", "0.0.0"),
               "-origin", spec.get("origin", "shipped"),
               "-out", str(args.out / name)]

        icons = spec.get("icons", "target")
        if icons == "target":
            cmd += ["-icons_from", str(target_uapp)]
        else:
            cmd += ["-normal_icon", str(m.parent / icons["normal"]),
                    "-small_icon", str(m.parent / icons["small"])]

        result = subprocess.run(cmd)
        if result.returncode != 0:
            fail(f"{name}: make_variant.py failed (exit {result.returncode})")

        packed = list((args.out / name).glob("*.uapp"))
        if len(packed) != 1:
            fail(f"{name}: expected exactly one packed .uapp, found {len(packed)}")
        verify_alias(packed[0], int(spec["appid"], 16), uappid_of(target_uapp))

    print(f"pack_variants: {len(manifests)} variant(s) packed and verified -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
