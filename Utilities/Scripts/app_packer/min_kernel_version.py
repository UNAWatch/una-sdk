#!/usr/bin/env python3
"""Derive an app's minimum kernel firmware version from the SDK ABI.

The compatibility contract between an app and the watch is the SDK ABI version
(KERNEL_INTERFACE_VERSION, in Libs/Header/SDK/Interfaces/IKernel.hpp) -- NOT any
marketing version. An app built against a given SDK requires a kernel whose ABI
is >= that SDK's ABI (the on-device check in AppSystem/system.cpp enforces this
at runtime). Because the ABI increases monotonically and each value maps to the
first kernel release that shipped it (abi_kernel_map.json), that requirement is
equivalent to a minimum kernel firmware version -- which is exactly what the
mobile app already gates install/update on: config.json "minKernelVersion",
compared against the watch's firmware version (BLE DIS 0x2A26).

This tool computes that value so it is never hand-typed:

  --print               print the derived minKernelVersion for this SDK (default)
  --stamp CONFIG_JSON   set/overwrite the minKernelVersion field in a config.json
  --check CONFIG_JSON   verify a config.json's minKernelVersion matches (CI gate)

It exits non-zero if the SDK's ABI has no mapping entry -- the guard that forces
abi_kernel_map.json to be updated whenever KERNEL_INTERFACE_VERSION is bumped.
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
MAP_PATH = SCRIPT_DIR / "abi_kernel_map.json"


def find_sdk_root(explicit=None):
    if explicit:
        return Path(explicit)
    env = os.environ.get("UNA_SDK")
    if env:
        return Path(env)
    # Utilities/Scripts/app_packer/ -> the SDK repo root is three levels up.
    return SCRIPT_DIR.parents[2]


def read_abi(sdk_root):
    header = Path(sdk_root) / "Libs" / "Header" / "SDK" / "Interfaces" / "IKernel.hpp"
    if not header.is_file():
        sys.exit(f"error: IKernel.hpp not found at {header} (pass --sdk-root or set UNA_SDK)")
    match = re.search(r"#define\s+KERNEL_INTERFACE_VERSION\s*\(?\s*(\d+)",
                      header.read_text(encoding="utf-8", errors="replace"))
    if not match:
        sys.exit(f"error: could not find KERNEL_INTERFACE_VERSION in {header}")
    return int(match.group(1))


def resolve(sdk_root):
    abi = read_abi(sdk_root)
    mapping = json.loads(MAP_PATH.read_text(encoding="utf-8")).get("map")
    if not isinstance(mapping, dict):
        sys.exit(f'error: {MAP_PATH.name} must contain a "map" object')
    key = str(abi)
    if key not in mapping:
        sys.exit(
            f"error: SDK ABI {abi} (KERNEL_INTERFACE_VERSION) has no entry in "
            f"{MAP_PATH.name}.\n"
            f"       The ABI was bumped without recording the kernel release that ships it.\n"
            f'       Add "{abi}": "<first stable kernel version with this ABI>" to the map.')
    min_kernel = mapping[key]
    if not isinstance(min_kernel, str) or not re.fullmatch(r"\d+\.\d+\.\d+", min_kernel):
        sys.exit(f"error: {MAP_PATH.name}: invalid version for ABI {abi}: {min_kernel!r} "
                 f"(expected MAJOR.MINOR.PATCH)")
    return abi, min_kernel


def main():
    ap = argparse.ArgumentParser(description="Derive minKernelVersion from the SDK ABI.")
    ap.add_argument("--sdk-root", help="SDK repo root (default: $UNA_SDK, else inferred)")
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--print", action="store_true", help="print the derived minKernelVersion (default)")
    group.add_argument("--stamp", metavar="CONFIG_JSON", help="set minKernelVersion in a config.json")
    group.add_argument("--check", metavar="CONFIG_JSON", help="verify a config.json's minKernelVersion")
    args = ap.parse_args()

    abi, min_kernel = resolve(find_sdk_root(args.sdk_root))

    if args.stamp:
        path = Path(args.stamp)
        cfg = json.loads(path.read_text(encoding="utf-8"))
        old = cfg.get("minKernelVersion")
        cfg["minKernelVersion"] = min_kernel
        path.write_text(json.dumps(cfg, indent=2) + "\n", encoding="utf-8")
        print(f"{path}: minKernelVersion {old!r} -> {min_kernel!r} (ABI {abi})")
    elif args.check:
        path = Path(args.check)
        got = json.loads(path.read_text(encoding="utf-8")).get("minKernelVersion")
        if got != min_kernel:
            sys.exit(f"error: {path}: minKernelVersion is {got!r}, expected {min_kernel!r} (ABI {abi})")
        print(f"{path}: minKernelVersion {got!r} OK (ABI {abi})")
    else:
        print(min_kernel)


if __name__ == "__main__":
    main()
