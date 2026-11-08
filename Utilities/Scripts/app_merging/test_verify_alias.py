#!/usr/bin/env python3
"""verify_alias rejects alias flags the kernel rejects.

Bit 0x04 is unassigned. Validator::aliasIsValid uses 0x7B, not 0x7F, so an
alias with that bit set must fail here instead of at scan time (#327).
No pytest: the apps CI prepare image has plain python3.
"""
import struct
import sys
import tempfile
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pack_variants import ALIAS_FLAGS_MASK, verify_alias  # noqa: E402

APPID = 0x1111111111111111
TARGET = 0x2222222222222222
failures = []


def expect(cond, msg):
    print(("ok:   " if cond else "FAIL: ") + msg)
    if not cond:
        failures.append(msg)


def alias_bytes(flags):
    header = struct.pack(
        "<QIIII16sII",
        APPID, 0x00010000, 0, 0, flags,
        b"Alias".ljust(16, b"\0"),
        3600, 900)
    payload = struct.pack("<IQIBI11s", 1, TARGET, 0, 0, 0, b"\0" * 11)
    blob = header + (b"\0" * (3600 + 900)) + payload
    return blob + struct.pack("<I", zlib.crc32(blob) & 0xFFFFFFFF)


def accepted(flags):
    path = Path(tmp) / f"flags-{flags:02x}.uapp"
    path.write_bytes(alias_bytes(flags))
    try:
        verify_alias(path, APPID, TARGET)
    except SystemExit:
        return False
    return True


with tempfile.TemporaryDirectory() as tmp:
    expect(ALIAS_FLAGS_MASK == 0x7B, "alias mask is 0x7B")
    for flags in (0x40, 0x41, 0x48, 0x49, 0x60, 0x7B):
        expect(accepted(flags), f"flags 0x{flags:02X} accepted")
    for flags in (0x00, 0x04, 0x44, 0xC0, 0xFF):
        expect(not accepted(flags), f"flags 0x{flags:02X} rejected")

if failures:
    print(f"{len(failures)} failed", file=sys.stderr)
    sys.exit(1)
print("all alias flag checks passed")
