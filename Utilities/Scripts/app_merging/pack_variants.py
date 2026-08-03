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
FLAGS_OFFSET = 20
FLAG_APP_VARIANT_ALIAS = 0x40
APP_TYPE_MASK = 0x03
APP_TYPES = {"Activity": 0, "Utility": 1, "Glance": 2, "Clockface": 3}


def fail(msg: str) -> None:
    print(f"pack_variants: ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def header_of(uapp: Path) -> tuple[int, int]:
    """(uappID, flags) from a .uapp MainHeader."""
    with open(uapp, "rb") as f:
        raw = f.read(MAIN_HEADER_SIZE)
    if len(raw) != MAIN_HEADER_SIZE:
        fail(f"{uapp} is too short to carry a MainHeader")
    uappid = struct.unpack_from("<Q", raw, 0)[0]
    flags = struct.unpack_from("<I", raw, FLAGS_OFFSET)[0]
    return uappid, flags


def uappid_of(uapp: Path) -> int:
    return header_of(uapp)[0]


def is_alias(uapp: Path) -> bool:
    """True for a code-less variant alias. In CI the artifacts directory can
    already contain OTHER variants' outputs (matrix jobs upload as they
    finish), so alias files must never count as compiled-app inputs."""
    return bool(header_of(uapp)[1] & FLAG_APP_VARIANT_ALIAS)


def version_of(uapp: Path) -> int:
    """uappVersion (u32 at offset 8) of a built .uapp."""
    with open(uapp, "rb") as f:
        raw = f.read(12)
    if len(raw) != 12:
        fail(f"{uapp} is too short to carry a MainHeader")
    return struct.unpack_from("<I", raw, 8)[0]


def version_string_of(uapp: Path, app_name: str) -> str:
    """The full version string of a built app, filename first.

    app_merging.py names outputs <safe-name>_<version>.uapp where <version>
    keeps any pre-release suffix (1.3.0-rc4) that the packed u32 cannot carry,
    so the filename is the richest source. Falls back to the header's A.B.C
    for a bare <name>.uapp.
    """
    stem = uapp.stem
    prefix = app_name + "_"
    if stem.startswith(prefix) and stem != prefix:
        return stem[len(prefix):]
    v = version_of(uapp)
    return f"{(v >> 16) & 0xFF}.{(v >> 8) & 0xFF}.{v & 0xFF}"


def find_built_uapp(name: str, search_roots: list[Path]) -> Path | None:
    """Newest compiled <Name>*.uapp under any search root (aliases excluded)."""
    candidates = []
    for root in search_roots:
        candidates += list(root.rglob(f"{name}_*.uapp")) + list(root.rglob(f"{name}.uapp"))
    candidates = [c for c in candidates if not is_alias(c)]
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
    if flags & ~0x7F:
        fail(f"{path.name}: flags outside the alias mask (0x{flags:08X})")
    if 0 not in data[24:40]:
        fail(f"{path.name}: app_name is not NUL-terminated within 16 bytes")
    normal_icon, small_icon = struct.unpack_from("<II", data, 40)
    if normal_icon != 3600 or small_icon != 900:
        fail(f"{path.name}: icon size fields must be 3600/900")

    (payload_ver, target, _min_ver, _origin, config_size) = \
        struct.unpack_from("<IQIBI", data, PAYLOAD_OFFSET)
    if payload_ver != 1:
        fail(f"{path.name}: unexpected payloadVersion {payload_ver}")
    if config_size > 8192:
        fail(f"{path.name}: configSize {config_size} exceeds the kernel's 8192-byte bound")
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
    # Alias files are skipped: in CI they are other variants' outputs, whose
    # IDs are already accounted for by the manifest-vs-manifest check below.
    ids: dict[int, str] = {}
    seen_files: dict[int, str] = {}
    for uapp in search_roots[0].rglob("*.uapp"):
        if is_alias(uapp):
            continue
        appid = uappid_of(uapp)
        if appid in seen_files and seen_files[appid] != uapp.name:
            fail(f"built apps {seen_files[appid]} and {uapp.name} share appID "
                 f"{appid:016X}")
        seen_files[appid] = uapp.name
        ids.setdefault(appid, f"built app {uapp.name}")
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
        missing = [k for k in ("name", "appid", "target", "type", "config")
                   if k not in spec]
        if missing:
            fail(f"{m}: manifest is missing required key(s): {', '.join(missing)}")
        if "appver" in spec:
            fail(f"{m}: 'appver' is no longer a manifest key -- variants version "
                 "in lockstep with their target app (same release, same version)")
        name = spec["name"]
        dirname = m.parent.name
        print(f"packing variant {name} (from {m})")

        # The artifact name and upload glob are keyed on the DIRECTORY name,
        # so a directory name that shadows a compiled app would collide with
        # its app-<Name> artifact.
        if (args.apps_root / dirname).is_dir():
            fail(f"{dirname}: variant directory name shadows an app directory")

        target_uapp = find_built_uapp(spec["target"], search_roots)
        if target_uapp is None:
            fail(f"{name}: no built .uapp found for target '{spec['target']}' "
                 f"under {search_roots[0]}")
        print(f"  target binary: {target_uapp}")

        # The manifest's type must match the target binary's (the kernel
        # adopts the target's type bits regardless), and glance targets are
        # rejected outright, mirroring the kernel's scan rules -- catch both
        # at packaging time, not on a watch.
        _, target_flags = header_of(target_uapp)
        target_type = target_flags & APP_TYPE_MASK
        if target_type == APP_TYPES["Glance"]:
            fail(f"{name}: target '{spec['target']}' is glance-typed; "
                 "variants of glance apps are not supported")
        if APP_TYPES.get(spec["type"]) != target_type:
            fail(f"{name}: manifest type '{spec['type']}' does not match the "
                 f"target binary's type bits ({target_type})")

        # Variants carry the same version as the app they release alongside:
        # derived from the freshly built target, never from the manifest.
        target_version = version_string_of(target_uapp, spec["target"])
        print(f"  version (lockstep with target): {target_version}")

        out_dir = args.out / dirname
        out_dir.mkdir(parents=True, exist_ok=True)
        for stale in out_dir.glob("*.uapp"):
            stale.unlink()

        cmd = [sys.executable, str(SCRIPT_DIR / "make_variant.py"),
               "-name", name,
               "-appid", spec["appid"],
               "-target_uapp", str(target_uapp),
               "-config", str(m.parent / spec["config"]),
               "-type", spec["type"],
               "-appver", target_version,
               "-min_target_version", spec.get("min_target_version", "0.0.0"),
               "-origin", spec.get("origin", "shipped"),
               "-out", str(out_dir)]

        icons = spec.get("icons", "target")
        if icons == "target":
            cmd += ["-icons_from", str(target_uapp)]
        else:
            cmd += ["-normal_icon", str(m.parent / icons["normal"]),
                    "-small_icon", str(m.parent / icons["small"])]

        result = subprocess.run(cmd)
        if result.returncode != 0:
            fail(f"{name}: make_variant.py failed (exit {result.returncode})")

        packed = list(out_dir.glob("*.uapp"))
        if len(packed) != 1:
            fail(f"{name}: expected exactly one packed .uapp, found {len(packed)}")
        verify_alias(packed[0], int(spec["appid"], 16), uappid_of(target_uapp))
        if version_of(packed[0]) != version_of(target_uapp):
            fail(f"{name}: alias uappVersion 0x{version_of(packed[0]):08X} is not "
                 f"in lockstep with the target's 0x{version_of(target_uapp):08X}")

    print(f"pack_variants: {len(manifests)} variant(s) packed and verified -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
