#!/usr/bin/env python3
"""Derive an app's minimum kernel firmware version from the SDK ABI.

The compatibility contract between an app and the watch is the SDK ABI version
(KERNEL_INTERFACE_VERSION, in Libs/Header/SDK/Interfaces/IKernel.hpp) -- NOT any
marketing version. An app built against a given SDK requires a kernel whose ABI
is >= that SDK's ABI (the on-device check in AppSystem/system.cpp enforces this
at runtime). Because the ABI increases monotonically and each value maps to the
minimum kernel firmware version that provides it (abi_kernel_map.json), that
requirement is a **floor** on the kernel firmware version -- which is what the
mobile app already gates on: config.json "minKernelVersion", compared against
the watch's firmware version (BLE DIS 0x2A26).

"minKernelVersion" is a MINIMUM. The ABI gives the floor; a developer MAY declare
a higher value (e.g. to require a firmware bugfix at the same ABI), but never a
lower one. So:

  --print               print the ABI-derived floor for this SDK (default)
  --stamp CONFIG_JSON   raise minKernelVersion up to the floor (keeps a higher value)
  --check CONFIG_JSON   verify minKernelVersion is >= the floor (CI / submission gate)

It exits non-zero if the SDK's ABI has no mapping entry (the guard that forces
abi_kernel_map.json to be updated when KERNEL_INTERFACE_VERSION is bumped) or if
the map is malformed / non-monotonic.

Note: config.json must be strict JSON. The annotated example in
Docs/app-config-json.md uses `//` comments for illustration only.
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
MAP_PATH = SCRIPT_DIR / "abi_kernel_map.json"
_SEMVER = re.compile(r"^(\d+)\.(\d+)\.(\d+)$")


def _ver(value):
    """'X.Y.Z' -> (x, y, z) for comparison, or None if it doesn't match."""
    m = _SEMVER.match(value) if isinstance(value, str) else None
    return tuple(int(g) for g in m.groups()) if m else None


def find_sdk_root(explicit=None):
    if explicit:
        return Path(explicit)
    script_root = SCRIPT_DIR.parents[2]          # Utilities/Scripts/app_packer -> repo root
    env = os.environ.get("UNA_SDK")
    if env:
        # The header is read from the SDK root; the map is always read next to this
        # script. Warn if those look like different checkouts.
        if Path(env).resolve() != script_root:
            print(f"warning: UNA_SDK ({env}) differs from this script's checkout "
                  f"({script_root}); reading IKernel.hpp from UNA_SDK but the map from "
                  f"the script's tree", file=sys.stderr)
        return Path(env)
    return script_root


def read_abi(sdk_root):
    header = Path(sdk_root) / "Libs" / "Header" / "SDK" / "Interfaces" / "IKernel.hpp"
    if not header.is_file():
        sys.exit(f"error: IKernel.hpp not found at {header} (pass --sdk-root or set UNA_SDK)")
    match = re.search(r"#define\s+KERNEL_INTERFACE_VERSION\s*\(?\s*(\d+)",
                      header.read_text(encoding="utf-8", errors="replace"))
    if not match:
        sys.exit(f"error: could not find KERNEL_INTERFACE_VERSION in {header}")
    return int(match.group(1))


def load_map(map_path=MAP_PATH):
    """Load + validate the ABI->version map: every value is X.Y.Z, and versions are
    non-decreasing in ascending ABI order (the property 'ABI >= N <=> firmware >= X.Y.Z'
    rests on). The map's *content* is trusted, not machine-verified against firmware."""
    name = Path(map_path).name
    try:
        data = json.loads(Path(map_path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        sys.exit(f"error: {name}: invalid JSON: {exc}")
    mapping = data.get("map") if isinstance(data, dict) else None
    if not isinstance(mapping, dict):
        sys.exit(f'error: {name} must contain a "map" object')
    for key in mapping:
        if not (isinstance(key, str) and key.isdigit()):
            sys.exit(f"error: {name}: ABI keys must be non-negative integers, got {key!r}")
    prev = None
    for key in sorted(mapping, key=int):
        ver = _ver(mapping[key])
        if ver is None:
            sys.exit(f"error: {Path(map_path).name}: invalid version for ABI {key}: "
                     f"{mapping[key]!r} (expected MAJOR.MINOR.PATCH)")
        if prev is not None and ver < prev:
            sys.exit(f"error: {Path(map_path).name}: ABI {key} maps to {mapping[key]}, lower "
                     f"than a smaller ABI -- the map must be monotonic")
        prev = ver
    return mapping


def resolve(sdk_root):
    abi = read_abi(sdk_root)
    mapping = load_map()
    key = str(abi)
    if key not in mapping:
        sys.exit(
            f"error: SDK ABI {abi} (KERNEL_INTERFACE_VERSION) has no entry in "
            f"{MAP_PATH.name}.\n"
            f"       The ABI was bumped without recording the kernel version that provides it.\n"
            f'       Add "{abi}": "<minimum kernel version providing this ABI>" to the map.')
    return abi, mapping[key]


def _read_config(path):
    try:
        cfg = json.loads(Path(path).read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        sys.exit(f"error: {path}: invalid JSON: {exc}")
    if not isinstance(cfg, dict):
        sys.exit(f"error: {path}: config must be a JSON object")
    return cfg


def main():
    ap = argparse.ArgumentParser(description="Derive the minimum kernel firmware version from the SDK ABI.")
    ap.add_argument("--sdk-root", help="SDK repo root (default: $UNA_SDK, else inferred)")
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--print", action="store_true", help="print the ABI-derived floor (default)")
    group.add_argument("--stamp", metavar="CONFIG_JSON", help="raise minKernelVersion up to the floor")
    group.add_argument("--check", metavar="CONFIG_JSON", help="verify minKernelVersion is >= the floor")
    args = ap.parse_args()

    abi, floor = resolve(find_sdk_root(args.sdk_root))
    floor_v = _ver(floor)

    if args.stamp:
        path = Path(args.stamp)
        cfg = _read_config(path)
        declared = cfg.get("minKernelVersion")
        declared_v = _ver(declared)
        if declared_v is not None and declared_v >= floor_v:
            print(f"{path}: minKernelVersion {declared!r} already >= floor {floor!r} (ABI {abi}); unchanged")
        else:
            cfg["minKernelVersion"] = floor
            path.write_text(json.dumps(cfg, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            print(f"{path}: minKernelVersion {declared!r} -> {floor!r} (ABI {abi} floor)")
    elif args.check:
        path = Path(args.check)
        declared = _read_config(path).get("minKernelVersion")
        declared_v = _ver(declared)
        if declared_v is None:
            sys.exit(f"error: {path}: minKernelVersion is {declared!r}; expected MAJOR.MINOR.PATCH >= {floor} (ABI {abi} floor)")
        if declared_v < floor_v:
            sys.exit(f"error: {path}: minKernelVersion {declared!r} is below the ABI {abi} floor {floor!r}")
        print(f"{path}: minKernelVersion {declared!r} >= floor {floor!r} (ABI {abi}) OK")
    else:
        print(floor)


if __name__ == "__main__":
    main()
