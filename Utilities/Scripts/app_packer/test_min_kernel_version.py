#!/usr/bin/env python3
"""Self-contained tests for min_kernel_version.py (run: python test_min_kernel_version.py).

Exercises the floor semantics, --stamp raise-to-floor, non-ASCII preservation, and the
clean error on invalid JSON. No pytest dependency (CI's prepare container has plain
python3). Points --sdk-root at a throwaway SDK with a chosen ABI, and uses the real
abi_kernel_map.json (ABI 3 -> 1.4.0)."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SCRIPT = HERE / "min_kernel_version.py"
FLOOR = "1.4.0"   # abi_kernel_map.json: ABI 3
FAKE_SDK = None
failures = []


def run(*args):
    return subprocess.run([sys.executable, str(SCRIPT), "--sdk-root", str(FAKE_SDK), *args],
                          capture_output=True, text=True)


def expect(cond, msg):
    print(("ok:   " if cond else "FAIL: ") + msg)
    if not cond:
        failures.append(msg)


def write_cfg(d, **fields):
    p = Path(d) / "config.json"
    p.write_text(json.dumps({"minKernelVersion": None, **fields}, ensure_ascii=False), encoding="utf-8")
    return p


with tempfile.TemporaryDirectory() as tmp:
    FAKE_SDK = Path(tmp) / "sdk"
    (FAKE_SDK / "Libs/Header/SDK/Interfaces").mkdir(parents=True)
    (FAKE_SDK / "Libs/Header/SDK/Interfaces/IKernel.hpp").write_text(
        "#define KERNEL_INTERFACE_VERSION    (3)\n", encoding="utf-8")

    r = run("--print")
    expect(r.returncode == 0 and r.stdout.strip() == FLOOR, f"--print == {FLOOR}")

    for val, ok in [("1.4.0", True), ("1.4.2", True), ("2.0.0", True), ("1.3.0", False), ("0.9.9", False)]:
        r = run("--check", str(write_cfg(tmp, minKernelVersion=val)))
        expect((r.returncode == 0) == ok, f"--check {val} -> {'accept' if ok else 'reject'}")
    r = run("--check", str(write_cfg(tmp)))   # minKernelVersion = null
    expect(r.returncode != 0, "--check missing -> reject")

    cfg = write_cfg(tmp, minKernelVersion="1.3.0")
    r = run("--stamp", str(cfg))
    expect(r.returncode == 0 and json.loads(cfg.read_text())["minKernelVersion"] == FLOOR, "--stamp raises 1.3.0 -> floor")
    cfg = write_cfg(tmp, minKernelVersion="1.5.0")
    r = run("--stamp", str(cfg))
    expect(r.returncode == 0 and json.loads(cfg.read_text())["minKernelVersion"] == "1.5.0", "--stamp keeps higher 1.5.0")

    cfg = write_cfg(tmp, minKernelVersion="1.3.0", description="Біг — застосунок")
    r = run("--stamp", str(cfg))
    expect(r.returncode == 0 and "Біг" in cfg.read_text(encoding="utf-8"), "--stamp preserves non-ASCII (ensure_ascii=False)")

    bad = Path(tmp) / "bad.json"
    bad.write_text('{\n  "minKernelVersion": "1.4.0", // comment\n}', encoding="utf-8")
    r = run("--check", str(bad))
    out = r.stdout + r.stderr
    expect(r.returncode != 0 and "invalid JSON" in out and "Traceback" not in out,
           "invalid JSON -> clean error (no traceback)")

    arr = Path(tmp) / "arr.json"
    arr.write_text("[]", encoding="utf-8")
    r = run("--check", str(arr))
    expect(r.returncode != 0 and "Traceback" not in (r.stdout + r.stderr),
           "non-object config -> clean error (no traceback)")

print()
if failures:
    print(f"{len(failures)} failure(s)")
    sys.exit(1)
print("all passed")
