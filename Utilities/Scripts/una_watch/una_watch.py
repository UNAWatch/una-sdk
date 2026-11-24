#!/usr/bin/env python3
"""Drive a UNA Watch from a computer over USB.

Turn on Developer on the watch first (Settings > Device > Developer). The tool
talks to the watch's USB serial console: it presses buttons, takes screenshots,
installs and launches apps, injects sensor values, reads the log, and runs the
same script files as the SDK's simulators (Docs/Simulator.md, "Scripted and
headless runs"). See Docs/watch-automation.md.

    una_watch.py info
    una_watch.py install Output/MyApp_1.2.0.uapp
    una_watch.py launch MyApp
    una_watch.py screenshot shot.png
    una_watch.py button R1 click
    una_watch.py log --seconds 30
    una_watch.py run script.txt --out run-out --launch MyApp

Needs Python 3.8+ and pyserial (pip install pyserial).
"""

import argparse
import codecs
import math
import os
import posixpath
import platform
import re
import string
import struct
import subprocess
import sys
import time
import zlib
from pathlib import Path

VID, PID = 0x0483, 0x52A4
ANSI = re.compile(rb"\x1b\[[0-9;?]*[A-Za-z]")
LOG_START, LOG_END = 0x1E, 0x1F
PROTOCOL = 2
MAX_TIME_MS = 4294967295     # a script's latest time, as in the simulators

# Exit codes: as the simulators use them (1 also for a command that failed), plus 5 for no
# usable watch.
EXIT_OK, EXIT_STEP_FAILED, EXIT_BAD_SCRIPT, EXIT_NO_WATCH = 0, 1, 2, 5
EXIT_FAILED = EXIT_STEP_FAILED

# SDK::Sensor::Type values the script steps inject (SDK/SensorLayer/SensorTypes.hpp).
HEART_RATE, HEART_RATE_EX = 0x41, 0x43
WRIST_MOTION = 0xA0
PRESSURE = 0x80
GPS_LOCATION, GPS_SPEED = 0x110, 0x111
BATTERY_LEVEL = 0x120
BUTTONS = ("L1", "L2", "R1", "R2")


class WatchError(Exception):
    pass


def _serial():
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        raise WatchError("una_watch.py needs pyserial: pip install pyserial")
    return serial, list_ports


class Watch:
    """The watch's USB serial console and its developer-mode commands."""

    def __init__(self, port=None, log_sink=None):
        self.port_name = port
        self.serial_number = None     # the watch's USB serial number, once connected
        self.protocol = 0             # of its 'dev' command, once checked
        self.firmware = None          # its firmware version, from protocol 3
        self.ser = None
        self.raw = bytearray()
        self.buf = bytearray()
        self.log_sink = log_sink
        self._log_open = False

    # --- connection ---------------------------------------------------------

    @staticmethod
    def watch_ports():
        _, list_ports = _serial()
        return [p for p in list_ports.comports() if p.vid == VID and p.pid == PID]

    @staticmethod
    def count_ports():
        return len(Watch.watch_ports())

    def find_port(self):
        """The port of this watch. Once connected, the watch is known by its USB serial number,
        so a reconnect after re-enumeration (where the port's name may change) reaches the same
        watch and no other."""
        ports = self.watch_ports()
        if self.serial_number:
            for p in ports:
                if p.serial_number == self.serial_number:
                    return p.device
            return None
        if self.port_name:
            return self.port_name if any(p.device == self.port_name for p in ports) else None
        if len(ports) > 1:
            raise WatchError("more than one watch is connected: pass --port to choose one")
        return ports[0].device if ports else None

    def open(self, timeout=30.0, _depth=0):
        serial, _ = _serial()
        deadline = time.monotonic() + timeout
        while True:
            name = self.find_port()
            if name:
                try:
                    self.ser = serial.Serial(name, 115200, timeout=0.05, write_timeout=5)
                    if self.serial_number is None:
                        self.serial_number = next((p.serial_number for p in self.watch_ports()
                                                   if p.device == name), None)
                    self.ser.dtr = True
                    self.ser.rts = True
                    self.raw.clear()
                    self.buf.clear()
                    self._log_open = False
                    time.sleep(0.2)
                    self.pump(0.2)
                    break
                except (serial.SerialException, OSError, WatchError):
                    self.close()
            if time.monotonic() > deadline:
                raise WatchError("the watch did not come back on USB" if self.serial_number else
                                 "no watch found on USB (VID 0483, PID 52A4)")
            time.sleep(0.25)
        self._leave_test_mode(timeout, _depth)

    def _leave_test_mode(self, timeout, depth):
        """In its first seconds after a start with USB connected, the watch puts itself in test
        mode on any input, and there only factory commands work. If that is where this landed,
        'boot' restarts the watch without that window, so the port can be used at once."""
        self._send("dev")
        try:
            self.pump(1.0)
        except WatchError:
            pass
        text = ANSI.sub(b"", bytes(self.buf)).decode("utf-8", "replace")
        self.buf.clear()
        if "una-test" not in text:
            return
        if depth >= 2:
            raise WatchError("the watch keeps starting in test mode")
        if "booting normally" not in self.plain("boot", 1.0):
            self.close()    # closing the port leaves test mode with a restart
            raise WatchError("this watch's firmware has no developer mode (no 'boot' in test mode)")
        self.reopen(gone_timeout=10, back_timeout=max(timeout, 60), _depth=depth + 1)

    def close(self):
        if self.ser:
            try:
                self.ser.close()
            except Exception:
                pass
        self.ser = None

    def reopen(self, gone_timeout=5.0, back_timeout=60.0, _depth=0):
        """After the watch re-enumerates (USB drive on or off, restart): wait for the port to
        go, then reconnect."""
        self.close()
        end = time.monotonic() + gone_timeout
        while time.monotonic() < end and self.find_port():
            time.sleep(0.1)
        if self.serial_number is None:
            self.port_name = None     # the name may change; without a serial number, rescan
        time.sleep(0.5)
        self.open(back_timeout, _depth)

    def restart(self, expect_path=None):
        """Restart the watch and reconnect once it is up and has found its apps: the app at
        expect_path (a .uapp path on the watch) if given, else any."""
        self.plain("reset", 0.5)
        self.reopen(gone_timeout=10, back_timeout=90)
        # The app list is built by a scan after boot; until it ends, 'dev apps' is empty.
        want = expect_path.lstrip("/").lower() if expect_path else None
        end = time.monotonic() + 30
        while True:
            apps = self.apps()
            if want is None and apps:
                return
            if want is not None and any(a["path"].replace("\\", "/").lower().endswith(want)
                                        for a in apps):
                return
            if time.monotonic() > end:
                raise WatchError(f"{expect_path} is not in the watch's app list after restarting: "
                                 "the watch did not accept it" if want else
                                 "the watch lists no apps after restarting")
            self.pump(0.5)

    # --- input --------------------------------------------------------------

    def _read(self, n):
        try:
            return self.ser.read(n)
        except Exception as e:      # serial.SerialException: the port went away
            raise WatchError(f"lost the watch: {e}")

    # What arrives is kept unread in self.raw until it is parsed into console text (self.buf)
    # and log frames (0x1E ... 0x1F, to the sink). Parsing stops at the end of a line when a
    # reply is being read, because binary data may follow it: a screenshot's pixels must not be
    # searched for log frames.

    def pump(self, seconds=0.0):
        """Read what the watch sent, for up to `seconds`, and hand any log text to the sink."""
        end = time.monotonic() + seconds
        while True:
            self.raw += self._read(4096)
            self._parse()
            if time.monotonic() >= end:
                return

    def _parse(self, one_line=False):
        """Move bytes from raw into text and log frames; with one_line, stop after a newline."""
        while self.raw:
            if self._log_open:
                j = self.raw.find(bytes([LOG_END]))
                if j < 0:
                    self._emit_log(self.raw)
                    self.raw.clear()
                    return
                self._emit_log(self.raw[:j])
                del self.raw[:j + 1]
                self._log_open = False
            elif self.raw[0] == LOG_START:
                self._log_open = True
                del self.raw[:1]
            else:
                j = self.raw.find(bytes([LOG_START]))
                stop = len(self.raw) if j < 0 else j
                if one_line:
                    nl = self.raw.find(b"\n", 0, stop)
                    if nl >= 0:
                        self.buf += self.raw[:nl + 1]
                        del self.raw[:nl + 1]
                        return
                self.buf += self.raw[:stop]
                del self.raw[:stop]

    def _emit_log(self, data):
        if data and self.log_sink:
            self.log_sink(bytes(data))

    def _line(self, deadline):
        while True:
            if b"\n" not in self.buf:
                self._parse(one_line=True)
            n = self.buf.find(b"\n")
            if n >= 0:
                text = bytes(self.buf[:n])
                del self.buf[:n + 1]
                return ANSI.sub(b"", text).replace(b"\r", b"").decode("utf-8", "replace")
            if time.monotonic() > deadline:
                raise WatchError("the watch did not answer")
            self.raw += self._read(4096)

    def _raw(self, n, deadline):
        """Exactly n bytes of binary data, straight from what arrived after the reply line."""
        while len(self.raw) < n:
            if time.monotonic() > deadline:
                raise WatchError("the watch stopped sending")
            self.raw += self._read(max(4096, n - len(self.raw)))
        out = bytes(self.raw[:n])
        del self.raw[:n]
        return out

    # --- commands -----------------------------------------------------------

    def _send(self, line):
        self.pump(0.0)
        self.buf.clear()
        try:
            self.ser.write(line.encode() + b"\n")
        except Exception as e:      # serial.SerialException, including the write timeout
            raise WatchError(f"lost the watch: {e}")

    def cmd(self, line, timeout=15.0):
        """Run a 'dev' command; return (text after #OK, the lines before it). #ERR raises."""
        self._send(line)
        deadline = time.monotonic() + timeout
        lines = []
        while True:
            text = self._line(deadline).strip()
            if text.startswith("#OK"):
                return text[3:].strip(), lines
            if text.startswith("#ERR"):
                raise WatchError(text[4:].strip())
            # Skip the echo of the command and the prompt; keep everything else.
            if text and text != line and "una>" not in text:
                lines.append(text)

    def plain(self, line, wait=0.5):
        """A console command with no #OK reply (usb, reset, boot): send it and give it time.

        Returns what it printed. The port may drop meanwhile: these commands re-enumerate."""
        self._send(line)
        try:
            self.pump(wait)
        except WatchError:
            pass
        out = ANSI.sub(b"", bytes(self.buf)).decode("utf-8", "replace")
        self.buf.clear()
        return out

    def require_developer_mode(self):
        try:
            payload, _ = self.cmd("dev")
        except WatchError:
            raise WatchError("this watch's firmware has no developer mode (no 'dev' command)")
        parts = payload.split()
        if len(parts) < 2 or parts[1] != "on":
            raise WatchError("Developer is off: on the watch, open Settings > Device and switch "
                             "Developer on")
        self.protocol = int(parts[0])
        self.firmware = parts[2] if len(parts) > 2 else None    # from protocol 3
        if self.protocol < PROTOCOL:
            raise WatchError(f"the watch's 'dev' protocol is {parts[0]}; this tool needs {PROTOCOL}")

    def drive_open(self):
        return re.search(r"drive:\s+open", self.plain("usb", 0.5)) is not None

    def close_drive(self):
        """Close the USB drive if it is open. While it is, apps are stopped and the charging
        screen can't be dismissed. The computer lets go of it first (see let_go_of_drive), so
        nothing it has cached for the drive is lost. The port reconnects."""
        if not self.drive_open():
            return False
        if self.count_ports() > 1:
            # A drive found by its label could be another watch's: letting go of that one and
            # closing this one's would leave this one's under the computer's cached writes.
            raise WatchError("the USB drive is open, and with several watches connected the "
                             "tool can't tell which drive is this one's: eject it on the "
                             "computer, then send 'usb drive off' or unplug")
        close_drive(self, find_drive())
        return True

    def show_home(self):
        """From what the watch shows on USB to the home screen: close the USB drive and dismiss
        the charging screen with R2, which the home screen ignores."""
        self.close_drive()
        self.button("R2", "click")
        self.pump(0.5)

    def screen(self, attempts=2):
        """The display: (width, height, one ABGR2222 byte per pixel).

        The watch holds the frame for up to 3 s while it is sent; a transfer slower than that
        can catch the next frame part-way, which the CRC shows. Then it is taken again."""
        for attempt in range(attempts):
            try:
                return self._screen_once()
            except WatchError as e:
                if "CRC mismatch" not in str(e) or attempt == attempts - 1:
                    raise
        raise WatchError("no screenshot")

    def _screen_once(self):
        self._send("dev screen")
        deadline = time.monotonic() + 20
        while True:
            text = self._line(deadline).strip()
            if text.startswith("#ERR"):
                raise WatchError(text[4:].strip())
            if text.startswith("#DATA"):
                break
        _, size, w, h, _fmt, crc = text.split()
        px = self._raw(int(size), deadline)
        if zlib.crc32(px) != int(crc, 16):
            raise WatchError("screenshot damaged in transfer (CRC mismatch)")
        while not self._line(deadline).strip().startswith("#OK"):
            pass
        return int(w), int(h), px

    def button(self, name, action, ms=None):
        line = f"dev button {name} {action}" + (f" {ms}" if ms is not None else "")
        self.cmd(line, timeout=(ms or 0) / 1000 + 10)

    def sensor(self, type_id, values):
        """Publish values in place of a sensor's: floats, or ("u", n) for an unsigned field.
        None goes back to the real sensor."""
        if values is None:
            return self.cmd(f"dev sensor {type_id:X} off")
        fields = [f"{int(v[1])}{v[0]}" if isinstance(v, tuple) else repr(float(v)) for v in values]
        return self.cmd(f"dev sensor {type_id:X} " + " ".join(fields))

    def apps(self):
        """Installed apps: dicts of pid (service), type, id, path, name."""
        _, lines = self.cmd("dev apps")
        out = []
        for text in lines:
            parts = text.split("\t")
            if len(parts) == 5:
                out.append(dict(zip(("pid", "type", "id", "path", "name"), parts)))
        return out

    def find_app(self, name_or_pid):
        for a in self.apps():
            if a["name"] == name_or_pid or a["pid"].lower() == name_or_pid.lower():
                return a
        raise WatchError(f"no app called {name_or_pid} on the watch")

    def launch(self, name_or_pid):
        pid = self.find_app(name_or_pid)["pid"]
        self.cmd(f"dev launch {pid}")
        return pid

    def stop(self, name_or_pid):
        pid = self.find_app(name_or_pid)["pid"]
        self.cmd(f"dev stop {pid}")
        return pid

    def file_matches(self, watch_path, data):
        """(whether the file on the watch holds exactly `data`, its size, the CRC compared).

        A .uapp ends with the CRC-32 of everything before it, so the CRC-32 of any whole intact
        .uapp is the same constant: the CRC of all but those last 4 bytes tells builds apart,
        and the whole-file CRC then covers the last 4 bytes too."""
        n = max(len(data) - 4, 0)
        payload, _ = self.cmd(f"dev crc {quoted(watch_path)} {n}", timeout=60)
        size, crc = (int(payload.split()[0]), int(payload.split()[1], 16))
        if size != len(data) or crc != zlib.crc32(data[:n]):
            return False, size, crc
        payload, _ = self.cmd(f"dev crc {quoted(watch_path)}", timeout=60)
        whole = int(payload.split()[1], 16)
        return whole == zlib.crc32(data), size, whole


# --- screenshots ---------------------------------------------------------------

def png_from_abgr2222(w, h, px, round_mask=True):
    """A PNG of the frame, each 2-bit channel widened to 8 bits; black outside the round glass."""
    lut = (0, 85, 170, 255)
    rows = bytearray()
    cx, cy, r2 = (w - 1) / 2, (h - 1) / 2, (min(w, h) / 2) ** 2
    for y in range(h):
        rows.append(0)
        for x in range(w):
            v = px[y * w + x]
            if round_mask and (x - cx) ** 2 + (y - cy) ** 2 > r2:
                rows += b"\0\0\0"
            else:
                rows += bytes((lut[v & 3], lut[(v >> 2) & 3], lut[(v >> 4) & 3]))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


# --- the USB drive -------------------------------------------------------------

def find_drive(label="UNA WATCH"):
    """The mounted watch drive, by its volume label. A watch whose drive has lost its label is
    taken only when it is the one removable drive with an Apps folder; otherwise pass --drive."""
    if platform.system() == "Windows":
        import ctypes
        k32 = ctypes.windll.kernel32
        unlabelled = []
        for letter in string.ascii_uppercase:
            root = f"{letter}:\\"
            if k32.GetDriveTypeW(root) != 2:      # DRIVE_REMOVABLE
                continue
            name = ctypes.create_unicode_buffer(261)
            if k32.GetVolumeInformationW(root, name, 261, None, None, None, None, 0):
                if name.value == label:
                    return Path(root)
                if (Path(root) / "Apps").is_dir():
                    unlabelled.append(Path(root))
        return unlabelled[0] if len(unlabelled) == 1 else None
    if platform.system() == "Linux":
        mounted = linux_mount_of_label(label)
        if mounted is not None:
            return mounted
    user = os.environ.get("USER", "")
    for base in (f"/Volumes/{label}", f"/media/{user}/{label}", f"/run/media/{user}/{label}"):
        if Path(base).is_dir():
            return Path(base)
    return None


def linux_mount_of_label(label, by_label="/dev/disk/by-label", mounts="/proc/mounts"):
    """Where Linux has the volume with this label mounted, wherever that is (a mount point of
    the user's own included), or None. The device comes from /dev/disk/by-label, where a space
    is written \\x20; /proc/mounts writes one in a device or mount point as \\040."""
    def unescape(field):
        return re.sub(r"\\([0-7]{3})", lambda m: chr(int(m.group(1), 8)), field)

    link = Path(by_label) / label.replace(" ", "\\x20")
    try:
        if not link.exists():
            return None
        device = os.path.realpath(link)
        with open(mounts, encoding="utf-8", errors="replace") as f:
            for line in f:
                fields = line.split()
                if len(fields) >= 2 and os.path.realpath(unescape(fields[0])) == device:
                    return Path(unescape(fields[1]))
    except OSError:
        return None
    return None


class _WindowsVolume:
    """The watch's volume, locked and dismounted until release(): every write the computer
    has cached reaches the watch, and nothing on the computer can write to the drive while
    the watch takes it back."""

    GENERIC_READ, GENERIC_WRITE = 0x80000000, 0x40000000
    FILE_SHARE_READ, FILE_SHARE_WRITE, OPEN_EXISTING = 1, 2, 3
    FSCTL_LOCK_VOLUME, FSCTL_DISMOUNT_VOLUME = 0x00090018, 0x00090020

    def __init__(self, drive):
        import ctypes
        from ctypes import wintypes
        self.k32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self.k32.CreateFileW.restype = wintypes.HANDLE
        self.k32.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                         wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD,
                                         wintypes.HANDLE]
        self.k32.DeviceIoControl.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPVOID,
                                             wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD,
                                             ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
        self.k32.FlushFileBuffers.argtypes = [wintypes.HANDLE]
        self.k32.CloseHandle.argtypes = [wintypes.HANDLE]
        self.ctypes, self.wintypes = ctypes, wintypes
        letter = str(drive)[0]
        self.h = self.k32.CreateFileW(f"\\\\.\\{letter}:", self.GENERIC_READ | self.GENERIC_WRITE,
                                      self.FILE_SHARE_READ | self.FILE_SHARE_WRITE, None,
                                      self.OPEN_EXISTING, 0, None)
        if self.h in (None, wintypes.HANDLE(-1).value):
            raise WatchError(f"can't open the watch's drive {letter}: to close it "
                             f"(error {ctypes.get_last_error()})")
        self.k32.FlushFileBuffers(self.h)
        # The lock is refused while another program has a file on the drive open.
        end = time.monotonic() + 10
        while not self._ioctl(self.FSCTL_LOCK_VOLUME):
            if time.monotonic() > end:
                err = ctypes.get_last_error()
                self.release()
                raise WatchError(f"a program has files open on the watch's drive {letter}: (error "
                                 f"{err}); close them, then run again. The drive is still open.")
            time.sleep(0.5)
        if not self._ioctl(self.FSCTL_DISMOUNT_VOLUME):
            err = ctypes.get_last_error()
            self.release()
            raise WatchError(f"can't dismount the watch's drive {letter}: (error {err}); "
                             "the drive is still open")

    def _ioctl(self, code):
        n = self.wintypes.DWORD(0)
        return bool(self.k32.DeviceIoControl(self.h, code, None, 0, None, 0,
                                             self.ctypes.byref(n), None))

    def release(self):
        if self.h is not None:
            self.k32.CloseHandle(self.h)
            self.h = None


def let_go_of_drive(drive):
    """Make the computer finish with the watch's drive before the watch takes it back: every
    cached write written out, the volume dismounted. Returns something to release() once the
    watch has the drive. Handing the drive back without this can leave writes the watch never
    gets, and damage its filesystem."""
    if platform.system() == "Windows":
        return _WindowsVolume(drive)
    subprocess.run(["sync"])
    if platform.system() == "Darwin":
        cmds = [["diskutil", "unmount", str(drive)]]
    else:
        # A desktop auto-mount is unmounted through gio as the user; umount works for a mount
        # the user may unmount (fstab 'user'/'users').
        cmds = [["gio", "mount", "-u", str(drive)], ["umount", str(drive)]]
    errors = []
    for cmd in cmds:
        try:
            r = subprocess.run(cmd, capture_output=True, text=True)
        except FileNotFoundError:
            continue
        if r.returncode == 0:
            return None
        errors.append(r.stderr.strip())
    raise WatchError(f"can't unmount the watch's drive {drive}: {'; '.join(errors) or 'no unmount tool'}"
                     ". Close anything using it, or unmount it yourself, and run again. The drive "
                     "is still open.")


def quoted(path):
    """A path as one argument of a watch command: the console splits a command at spaces
    outside quotes. FAT allows no double quote in a name, so none needs escaping."""
    if '"' in path:
        raise WatchError(f"no double quote is allowed in a path on the watch: {path}")
    return f'"{path}"'


def folder_name_ok(name):
    """A single folder name under Apps/."""
    return (bool(name) and name not in (".", "..") and not any(c in name for c in "/\\:")
            and name.strip() == name)


def app_folder(apps, data):
    """The folder under Apps/ holding the app in this .uapp (its first 8 bytes are the app ID),
    or None for a new app."""
    app_id = f"{struct.unpack_from('<Q', data)[0]:016X}"
    for a in apps:
        parts = a["path"].replace("\\", "/").split("/")
        if a["id"].upper() == app_id and len(parts) >= 3:
            return parts[-2]
    return None


def install(watch, uapp, folder=None, drive_arg=None, attempts=3):
    """Copy a .uapp over the USB drive, check it on the watch, restart the watch.

    An update goes into the folder that already holds the app, whatever its files are called;
    a new app gets Apps/<name>/."""
    uapp = Path(uapp)
    if uapp.suffix.lower() != ".uapp":
        # The watch loads only .uapp files, and a file of another kind's name in the folder
        # would be replaced without a backup.
        raise WatchError(f"{uapp} is not a .uapp file")
    data = uapp.read_bytes()
    if len(data) < 12:
        raise WatchError(f"{uapp} is too short to be a .uapp")
    if drive_arg is None and Watch.count_ports() > 1:
        raise WatchError("more than one watch is connected: pass --drive to say which drive is "
                         "this one's")
    if folder is None:
        folder = app_folder(watch.apps(), data)
        if folder:
            print(f"updating the app in Apps/{folder}")
        else:
            m = re.match(r"(.+?)_\d", uapp.name)
            folder = m.group(1) if m else uapp.stem
            print(f"new app: Apps/{folder}")
    if not folder_name_ok(folder):
        raise WatchError(f"'{folder}' is not a folder name: one name, no '/', '\\', ':' or '..'")
    watch_path = f"/Apps/{folder}/{uapp.name}"

    # The version being replaced is kept as <file>.uapp.bak, which the watch does not load,
    # until the watch has accepted the new one; on any failure before that, it is put back.
    # Backups are recorded by name within the app folder, as (backup, original), and found
    # again on the drive as it is mounted then: the mount point can change between openings.
    backups = []
    state = {"written": False}      # whether this install has written its copy
    try:
        _copy_and_check(watch, uapp, data, folder, watch_path, drive_arg, attempts, backups,
                        state)
        print("restarting the watch so it finds the app")
        state["restarting"] = True
        watch.restart(watch_path)
    except (WatchError, OSError) as e:
        if backups or state["written"]:
            _restore(watch, drive_arg, folder, uapp.name, backups, state["written"], e)
        raise
    except KeyboardInterrupt:
        # Interrupted while the drive was being changed: put it back as for a failure. Not once
        # the watch is restarting: it may already have taken the new version.
        if (backups or state["written"]) and not state.get("restarting"):
            _restore(watch, drive_arg, folder, uapp.name, backups, state["written"],
                     WatchError("interrupted"))
        raise
    if backups:
        _remove_backups(watch, drive_arg, folder, backups)
    return watch_path


def _remove_backups(watch, drive_arg, folder, backups):
    """Delete the backups once the watch has taken the new version. For a few seconds after
    the restart the watch refuses writes to its drive (Windows reports it as write-protected),
    so this tries again for a while. The install has succeeded either way: a backup left
    behind is only a warning, since the watch never loads a .bak."""
    left = [name for name, _ in backups]
    try:
        drive = open_drive(watch, drive_arg)
        try:
            app_dir = drive / "Apps" / folder
            end = time.monotonic() + 20
            while left:
                for name in list(left):
                    try:
                        if (app_dir / name).exists():
                            (app_dir / name).unlink()
                        left.remove(name)
                    except OSError:
                        pass
                if not left or time.monotonic() > end:
                    break
                time.sleep(1)
        finally:
            close_drive(watch, drive)
    except (WatchError, OSError) as e:
        print(f"warning: {e}", file=sys.stderr)
    if left:
        print(f"warning: the previous version's backup is still in Apps/{folder}: "
              + ", ".join(left) + ". The watch does not load it; delete it when convenient.",
              file=sys.stderr)


def uapp_files(folder):
    """The .uapp files in a folder, whatever the case of the extension: the watch's volume
    ignores case, where a glob on Linux or macOS would miss Demo.UAPP."""
    if not folder.is_dir():
        return []
    return [p for p in folder.iterdir() if p.is_file() and p.suffix.lower() == ".uapp"]


def backup_name(path):
    """An unused name for a backup of path: <file>.bak, or <file>.bak2 and on if taken. A
    .bak already there may be someone's; it is left alone."""
    n = 1
    while True:
        bak = path.with_name(path.name + (".bak" if n == 1 else f".bak{n}"))
        if not bak.exists():
            return bak
        n += 1


def _restore(watch, drive_arg, folder, name, backups, written, error):
    """Undo a failed install, or say how to: delete the copy it wrote, and put back the
    version it replaced, if any. The copy is deleted only if this install wrote it: before
    that, a file of that name is the original."""
    print("putting the previous version back" if backups else "removing the copy")
    copy_removed = not written
    left = list(backups)            # (backup, original) not yet put back
    try:
        if watch.ser is None:
            # Interrupted, or timed out, while waiting for the port to come back.
            watch.open()
        drive = open_drive(watch, drive_arg)
        try:
            app_dir = drive / "Apps" / folder
            # The new copy goes first: a backup may be going back under the same name.
            if written and (app_dir / name).exists():
                (app_dir / name).unlink()
            copy_removed = True
            for bak, original in backups:
                # A backup not found on the drive as now mounted stays on the list to do.
                if (app_dir / bak).exists():
                    (app_dir / bak).rename(app_dir / original)
                    left.remove((bak, original))
        finally:
            close_drive(watch, drive)
        if left:
            raise WatchError("backup not found: " + ", ".join(bak for bak, _ in left))
        watch.restart()
    except (WatchError, OSError) as e:
        todo = []
        if not copy_removed:
            todo.append(f"delete {name}")
        todo += [f"rename {bak} to {original}" for bak, original in left]
        what = "putting the previous version back" if backups else "removing the copy"
        how = (": on the watch's drive, in Apps/" + folder + ", " + "; then ".join(todo)
               if todo else "; the files are back as they were, and the watch needs a restart")
        raise WatchError(f"{error}; and {what} failed ({e}){how}") from e


def _copy_and_check(watch, uapp, data, folder, watch_path, drive_arg, attempts, backups, state):
    for attempt in range(1, attempts + 1):
        print(f"opening the watch's USB drive (attempt {attempt})")
        drive = open_drive(watch, drive_arg)
        try:
            dest_dir = drive / "Apps" / folder
            dest = dest_dir / uapp.name
            # Never touch another app's files: every .uapp already in the folder must carry
            # this app's ID (its first 8 bytes). On a retry, the copy this install wrote is
            # left out: damage can reach its first bytes too.
            others = [p.name for p in uapp_files(dest_dir)
                      if not (attempt > 1 and p.name == dest.name) and p.read_bytes()[:8] != data[:8]]
            if others:
                raise WatchError(f"Apps/{folder} holds another app ({', '.join(others)}): "
                                 "pass --folder to choose a different folder")
            dest_dir.mkdir(parents=True, exist_ok=True)
            if attempt == 1:
                for old in uapp_files(dest_dir):
                    bak = backup_name(old)
                    old.rename(bak)
                    backups.append((bak.name, old.name))
            if dest.exists():
                dest.unlink()       # a damaged copy from the attempt before
            state["written"] = True
            with open(dest, "wb") as f:
                f.write(data)
                f.flush()
                os.fsync(f.fileno())
        finally:
            print("closing the drive")
            close_drive(watch, drive)

        same, size, crc = watch.file_matches(watch_path, data)
        if same:
            print(f"{watch_path}: {size} bytes, matches")
            return
        print(f"copy damaged on the watch ({size} bytes, CRC {crc:08X}); copying again")
    raise WatchError(f"{watch_path} could not be copied intact in {attempts} attempts")


def update_kernel(watch, ota, drive_arg=None):
    """Install a kernel .ota as the phone app does: copy it to /Update/ over the USB drive,
    check it, and have the watch hand it to the bootloader, which installs it on the restart
    that follows. Returns the firmware version the watch comes back with."""
    ota = Path(ota)
    if ota.suffix.lower() != ".ota":
        raise WatchError(f"{ota} is not an .ota file")
    if not folder_name_ok(ota.name):
        raise WatchError(f"'{ota.name}' can't be a file name on the watch")
    if watch.protocol < 3:
        raise WatchError("this watch's firmware has no 'dev update' (protocol 3): install a kernel "
                         "with it by the bootloader or the phone app first")
    if drive_arg is None and Watch.count_ports() > 1:
        raise WatchError("more than one watch is connected: pass --drive to say which drive is "
                         "this one's")
    data = ota.read_bytes()
    before = watch.firmware
    watch_path = f"/Update/{ota.name}"

    # A copy to the watch's drive is now and then damaged on the way, so each is checked on
    # the watch and made again if it differs. Only an intact copy goes to the bootloader.
    attempts = 3
    for attempt in range(1, attempts + 1):
        print(f"opening the watch's USB drive (attempt {attempt})")
        drive = open_drive(watch, drive_arg)
        try:
            update_dir = drive / "Update"
            update_dir.mkdir(exist_ok=True)
            dest = update_dir / ota.name
            if dest.exists():
                dest.unlink()
            with open(dest, "wb") as f:
                f.write(data)
                f.flush()
                os.fsync(f.fileno())
        finally:
            print("closing the drive")
            close_drive(watch, drive)

        same, size, crc = watch.file_matches(watch_path, data)
        if same:
            print(f"{watch_path}: {size} bytes, matches")
            break
        print(f"copy damaged on the watch ({size} bytes, CRC {crc:08X}); copying again")
    else:
        raise WatchError(f"{watch_path} could not be copied intact in {attempts} attempts")

    # The watch answers once the kernel is in place for its bootloader (copying it takes a
    # few seconds), or with #ERR and the reason; then it restarts.
    print("the watch is copying the kernel to its bootloader")
    watch.cmd(f"dev update {quoted(watch_path)}", timeout=120)
    end = time.monotonic() + 30
    try:
        while time.monotonic() < end:
            watch.pump(0.5)
    except WatchError as e:
        if not str(e).startswith("lost the watch"):
            raise
    else:
        raise WatchError("the watch did not restart within 30 s of putting the kernel in place")

    print("installing; waiting for the watch to come back")
    watch.reopen(gone_timeout=5, back_timeout=600)
    watch.require_developer_mode()
    print(f"firmware: {before} -> {watch.firmware}")
    return watch.firmware


def open_drive(watch, drive_arg=None):
    """Open the watch's USB drive and return where it is mounted."""
    watch.plain("usb drive on", 0.5)
    watch.reopen()
    end = time.monotonic() + 30
    if drive_arg:
        # The port can come back before the volume is mounted, and until it is, writes to the
        # mount point would land on the computer's own disk.
        drive = Path(drive_arg)
        while not (os.path.ismount(drive) and drive.is_dir()):
            if time.monotonic() >= end:
                # The drive may have been mounted somewhere else: found there, it is let go of
                # before the watch takes it back. Not looked for among several watches, where
                # the drive found could be another's.
                close_drive(watch, find_drive() if Watch.count_ports() == 1 else None)
                raise WatchError(f"nothing was mounted at {drive}")
            time.sleep(0.5)
        return drive
    drive = None
    while drive is None and time.monotonic() < end:
        drive = find_drive()
        if drive is None:
            time.sleep(0.5)
    if drive is None:
        close_drive(watch, None)
        raise WatchError("the watch's drive did not appear; pass --drive")
    return drive


def close_drive(watch, drive):
    """Give the USB drive back to the watch: the computer lets go of it first (all writes out,
    the volume dismounted), then 'usb drive off'. If the computer can't let go, the drive is
    left open rather than taken back from under it. A drive not found may still be mounted
    where the tool doesn't look (a mount point of the user's own): whatever the computer has
    cached for any drive goes out first (POSIX; on Windows the drive is found by its label)."""
    held = let_go_of_drive(drive) if drive is not None else None
    if drive is None and hasattr(os, "sync"):
        os.sync()
    try:
        watch.plain("usb drive off", 0.5)
        try:
            watch.reopen()
        except WatchError as e:
            raise WatchError(f"{e}; the watch's USB drive may still be open: unplug the watch, "
                             "or send 'usb drive off' on its serial console")
    finally:
        if held is not None:
            held.release()


# --- scripts -------------------------------------------------------------------

class ScriptError(Exception):
    pass


def parse_time(tok, prev):
    rel = tok.startswith("+")
    t = tok[1:] if rel else tok
    mult = 1.0
    if t.endswith("s"):
        t, mult = t[:-1], 1000.0
    try:
        v = float(t) * mult
    except ValueError:
        raise ScriptError(f"bad time '{tok}'")
    if not math.isfinite(v) or v < 0:
        raise ScriptError(f"bad time '{tok}'")
    ms = prev + v if rel else v
    if ms < prev:
        raise ScriptError("times may not go backwards")
    if ms > MAX_TIME_MS:
        raise ScriptError(f"time past {MAX_TIME_MS} ms")
    return ms


def load_replay(path):
    """Fixes from a replay CSV: (t_s, lat, lon, alt_m, speed_mps). A first line that is not
    numbers is a header."""
    rows = []
    for n, line in enumerate(Path(path).read_text().splitlines()):
        if not line.strip():
            continue
        parts = [p.strip() for p in line.split(",")]
        try:
            if len(parts) < 5:
                raise ValueError
            rows.append(tuple(float(p) for p in parts[:5]))
        except ValueError:
            if n == 0:
                continue
            raise ScriptError(f"{path}:{n + 1}: expected t_s,lat,lon,alt_m,speed_mps")
    if not rows:
        raise ScriptError(f"{path}: no fixes")
    return rows


def replay_at(rows, t):
    """(lat, lon, alt, speed) at t seconds into a replay, interpolated between fixes; after the
    last fix the position holds with speed 0."""
    if t <= rows[0][0]:
        r = rows[0]
        return r[1], r[2], r[3], r[4]
    for a, b in zip(rows, rows[1:]):
        if a[0] <= t <= b[0]:
            f = (t - a[0]) / (b[0] - a[0]) if b[0] > a[0] else 0
            return tuple(a[i] + (b[i] - a[i]) * f for i in range(1, 5))
    r = rows[-1]
    return r[1], r[2], r[3], 0.0


def screenshot_name_ok(name):
    """A screenshot name stays inside --out: relative on any system, no '..', no drive."""
    norm = name.replace("\\", "/")
    return (norm != "" and not norm.startswith("/") and ":" not in norm
            and ".." not in norm.split("/") and posixpath.normpath(norm) != "watch.log")


def parse_script(path):
    """The steps of a simulator script, checked: (time_ms, line, step, args)."""
    steps, prev = [], 0.0
    base = Path(path).parent
    try:
        text = Path(path).read_text()
    except UnicodeDecodeError as e:
        raise ScriptError(f"{path}: not a text file (byte {e.start}: {e.reason})")
    for n, line in enumerate(text.splitlines(), 1):
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        try:
            t = parse_time(parts[0], prev)
            if len(parts) < 2:
                raise ScriptError("missing step")
            step, args = parts[1], parts[2:]
            if step in ("click", "press", "release"):
                if len(args) != 1 or args[0] not in BUTTONS:
                    raise ScriptError(f"{step} needs a button: L1, L2, R1 or R2")
            elif step == "hold":
                if len(args) != 2 or args[0] not in BUTTONS or not re.fullmatch(r"[0-9]+", args[1]):
                    raise ScriptError("hold needs a button and a time in ms")
            elif step == "screenshot":
                if len(args) != 1 or not screenshot_name_ok(args[0]):
                    raise ScriptError("screenshot needs a name inside the output folder: not "
                                      "rooted, no '..' or ':', not watch.log")
            elif step in ("hr", "battery", "pressure"):
                if len(args) != 1:
                    raise ScriptError(f"{step} needs a value or auto")
                if args[0] != "auto":
                    float(args[0])
            elif step == "gps":
                if not args or args[0] not in ("fix", "replay", "auto"):
                    raise ScriptError("gps fix on|off|auto, gps replay FILE or gps auto")
                if args[0] == "fix" and (len(args) != 2 or args[1] not in ("on", "off", "auto")):
                    raise ScriptError("gps fix on|off|auto")
                if args[0] == "auto" and len(args) != 1:
                    raise ScriptError("gps auto takes nothing more")
                if args[0] == "replay":
                    if len(args) != 2:
                        raise ScriptError("gps replay FILE")
                    args = ["replay", load_replay(base / args[1])]
            elif step == "log":
                args = [" ".join(args)]
            elif step not in ("wrist", "exit"):
                raise ScriptError(f"unknown step '{step}'")
        except (ScriptError, ValueError, OSError) as e:
            raise ScriptError(f"{path}:{n}: {e}")
        steps.append((t, n, step, args))
        prev = t
    return steps


class Runner:
    """Runs a simulator script against the watch."""

    # Where a forced fix with no replay puts the watch.
    DEFAULT_FIX = (51.5, -0.1, 20.0, 0.0)

    def __init__(self, watch, out, log):
        self.w = watch
        self.out = Path(out)
        self.log = log
        self.replay = None
        self.replay_t0 = 0.0
        self.fix = "auto"
        self.last_gps = 0.0
        self.failed = False
        # Buttons a 'hold' pressed, with when to release them: [(monotonic time, button, tries)].
        self.releases = []
        # Buttons the script has down, from press or hold until their release. As in the
        # simulators, a click, hold or press of a button already down does nothing.
        self.down = set()
        self.lost = False           # the port was lost while releasing at the end of the run
        self.t0 = None             # when the run started, for the times in watch.log

    def note(self, text):
        self.log.write(f"[run] {text}\n".encode())
        self.log.flush()
        print(text)

    def send_gps(self, now):
        if self.fix == "off":
            self.w.sensor(GPS_LOCATION, [0.0, ("u", 0), 0.0, 0.0, 0.0])
            self.w.sensor(GPS_SPEED, [0.0, ("u", 0), ("u", 0)])
            return
        if self.replay is not None:
            lat, lon, alt, speed = replay_at(self.replay, now - self.replay_t0)
        elif self.fix == "on":
            lat, lon, alt, speed = self.DEFAULT_FIX
        else:
            return
        self.w.sensor(GPS_LOCATION, [3.0, ("u", 1), lat, lon, alt])
        self.w.sensor(GPS_SPEED, [speed, ("u", 1), ("u", 0)])

    def gps_off(self):
        self.w.sensor(GPS_LOCATION, None)
        self.w.sensor(GPS_SPEED, None)

    RELEASE_TRIES = 3

    def release_due(self, now, everything=False):
        """Release the held buttons whose time has come (all of them, at the end of a run). A
        release that fails to send is tried again a second later, up to RELEASE_TRIES times;
        closing the port releases it on the watch in any case."""
        for entry in sorted(self.releases):
            due, button, tries = entry
            if not (everything or due <= now):
                continue
            self.releases.remove(entry)
            try:
                self.w.button(button, "release")
                self.down.discard(button)
                if self.t0 is not None:
                    self.note(f"{time.monotonic() - self.t0:8.3f}s {button} released (hold)")
            except WatchError as e:
                tries += 1
                self.failed = True
                if everything and str(e).startswith("lost the watch"):
                    # The port is gone, and with it every button the run held down: closing
                    # it releases them on the watch. Each further try would only wait out
                    # the write timeout.
                    self.note(f"releasing {button} failed: {e}")
                    self.releases.clear()
                    self.lost = True
                    return
                if tries < self.RELEASE_TRIES and not everything:
                    self.note(f"releasing {button} failed, trying again: {e}")
                    self.releases.append((time.monotonic() + 1.0, button, tries))
                elif not everything:
                    # The button may still be down, and later steps would go on around it: the
                    # run stops, and its end tries the release once more before the port
                    # closes, which releases it anyway.
                    raise WatchError(f"releasing {button} failed {tries} times: {e}")
                else:
                    self.note(f"releasing {button} failed: {e}")

    def wait_until(self, t_end):
        """Wait, collecting the log, releasing held buttons on time, and sending a GPS fix each
        second while one is due."""
        while True:
            now = time.monotonic()
            if self.releases:
                self.release_due(now)
            if (self.replay is not None or self.fix != "auto") and now - self.last_gps >= 1.0:
                self.last_gps = now
                try:
                    self.send_gps(now)
                except WatchError as e:
                    # Location and speed go together: a half-sent update is withdrawn, and the
                    # GPS goes back to the watch's own. If that fails too, the run ends.
                    self.failed = True
                    self.note(f"GPS update failed, GPS back to the watch's own: {e}")
                    self.replay, self.fix = None, "auto"
                    self.gps_off()
            if now >= t_end:
                return
            step = t_end - now
            if self.releases:
                step = min(step, max(min(r[0] for r in self.releases) - now, 0.0))
            self.w.pump(min(0.05, step))

    def press(self, button, release=None):
        """Press a button, recorded as down first. If the watch refuses the press (#ERR), the
        button is up after all and a hold's release is dropped; if the answer is lost, it may
        be down, so both stay."""
        self.down.add(button)
        try:
            self.w.button(button, "press")
        except WatchError as e:
            if not str(e).startswith(("lost the watch", "the watch did not answer")):
                self.down.discard(button)
                if release in self.releases:
                    self.releases.remove(release)
            raise

    def step(self, step, args):
        w = self.w
        if step in ("click", "hold", "press") and args[0] in self.down:
            self.note(f"{args[0]} is already down: {step} does nothing")
        elif step == "click":
            w.button(args[0], "click")
        elif step == "hold":
            # As in the simulators: pressed now, released MS later, while later steps keep
            # their own times.
            # The release is due before the press is sent: a press whose answer is lost
            # still gets its release.
            entry = (time.monotonic() + int(args[1]) / 1000, args[0], 0)
            self.releases.append(entry)
            self.press(args[0], entry)
        elif step == "press":
            self.press(args[0])
        elif step == "release":
            w.button(args[0], "release")
            # Released by hand: a hold's pending release of it is done, once the release
            # has actually been sent.
            self.down.discard(args[0])
            self.releases = [r for r in self.releases if r[1] != args[0]]
        elif step == "screenshot":
            name = args[0] if Path(args[0]).suffix else args[0] + ".png"
            dest = (self.out / name).resolve()
            try:
                dest.relative_to(self.out.resolve())
            except ValueError:
                raise WatchError(f"{name} leads outside {self.out}")
            log = (self.out / "watch.log").resolve()
            if dest == log or (dest.exists() and log.exists() and os.path.samefile(dest, log)):
                raise WatchError(f"{name} is the run's log")
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(png_from_abgr2222(*w.screen()))
        elif step == "hr":
            if args[0] == "auto":
                w.sensor(HEART_RATE, None)
                w.sensor(HEART_RATE_EX, None)
            else:
                bpm = float(args[0])       # trust 3: a scripted value is always trusted
                w.sensor(HEART_RATE, [bpm, 3.0])
                w.sensor(HEART_RATE_EX, [bpm, 3.0, 1.0, bpm, 3.0, 0.0, 0.0])
        elif step == "battery":
            w.sensor(BATTERY_LEVEL, None if args[0] == "auto" else [float(args[0])])
        elif step == "pressure":
            pa = None if args[0] == "auto" else float(args[0]) * 100.0
            w.sensor(PRESSURE, None if pa is None else [pa, pa])
        elif step == "gps":
            if args[0] == "auto":
                self.replay, self.fix = None, "auto"
                self.gps_off()
            elif args[0] == "fix":
                self.fix = args[1]
                if self.fix == "auto" and self.replay is None:
                    self.gps_off()
                else:
                    self.last_gps = 0.0
            elif args[0] == "replay":
                self.replay = args[1]
                self.replay_t0 = time.monotonic()
                self.last_gps = 0.0
        elif step == "wrist":
            w.sensor(WRIST_MOTION, [("u", 1)])
        elif step == "log":
            self.note(args[0])

    def run(self, steps, duration_ms=None):
        t0 = self.t0 = time.monotonic()
        end = t0 + duration_ms / 1000 if duration_ms is not None else None
        try:
            for t, line, step, args in steps:
                due = t0 + t / 1000
                if end is not None and due > end:
                    break
                self.wait_until(due)
                if end is not None and time.monotonic() > end:
                    break           # a step before ran past the end of the run
                shown = " ".join(str(a) for a in args if not isinstance(a, list))
                ran = time.monotonic() - t0
                late = f" (ran at {ran:.3f}s)" if ran - t / 1000 >= 0.05 else ""
                self.note(f"{t / 1000:8.3f}s line {line}: {step} {shown}".rstrip() + late)
                if step == "exit":
                    return
                try:
                    self.step(step, args)
                except (WatchError, OSError) as e:
                    self.failed = True
                    self.note(f"step failed: {e}")
            if end is not None:
                self.wait_until(end)
            elif self.releases:
                # The script ended during a hold: let the hold run its length.
                while self.releases:
                    self.wait_until(max(r[0] for r in self.releases))
        finally:
            # 'exit' or the end of the run lets go of anything still held: holds first, then
            # buttons a 'press' left down, which have no release due.
            if self.releases:
                self.release_due(time.monotonic(), everything=True)
            for button in sorted(self.down) if not self.lost else ():
                try:
                    self.w.button(button, "release")
                    self.down.discard(button)
                    self.note(f"{time.monotonic() - t0:8.3f}s {button} released (end of run)")
                except WatchError as e:
                    self.failed = True
                    self.note(f"releasing {button} failed: {e}")
                    if str(e).startswith("lost the watch"):
                        break       # as for the holds above


# --- command line ----------------------------------------------------------------

def connect(args, log_sink=None):
    w = Watch(args.port, log_sink)
    w.open(args.wait)
    w.require_developer_mode()
    return w


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", help="serial port (default: found by USB ID)")
    ap.add_argument("--wait", type=float, default=30, help="seconds to wait for the watch (default 30)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("info", help="check the connection and list installed apps")
    sub.add_parser("apps", help="list installed apps: pid, type, folder, name")
    sub.add_parser("home", help="close the USB drive and dismiss the charging screen")
    s = sub.add_parser("update-kernel", help="install a kernel .ota, as the phone app does")
    s.add_argument("ota")
    s.add_argument("--drive", help="where the watch's drive is mounted (default: found by its label)")
    s = sub.add_parser("install", help="copy a .uapp to the watch, check it, restart the watch")
    s.add_argument("uapp")
    s.add_argument("--folder", help="app folder under Apps/ (default: the app's folder, or its name)")
    s.add_argument("--drive", help="where the watch's drive is mounted (default: found by its label)")
    s = sub.add_parser("launch", help="go to the home screen, then start an app by name or pid")
    s.add_argument("app")
    s = sub.add_parser("stop", help="stop an app by name or pid")
    s.add_argument("app")
    s = sub.add_parser("screenshot", help="save the display as PNG")
    s.add_argument("file")
    s.add_argument("--square", action="store_true", help="keep the corners outside the round glass")
    s = sub.add_parser("button", help="press a button")
    s.add_argument("name", choices=BUTTONS)
    s.add_argument("action", choices=("click", "press", "release", "hold"))
    s.add_argument("ms", nargs="?", type=int, help="for hold: how long, in ms")
    s = sub.add_parser("log", help="print the watch's log as it is written")
    s.add_argument("--seconds", type=float, default=0, help="stop after this long (default: Ctrl+C)")
    s = sub.add_parser("run", help="run a simulator script on the watch")
    s.add_argument("script")
    s.add_argument("--out", default="watch-out", help="screenshots and watch.log (default: watch-out)")
    s.add_argument("--duration-ms", type=int, help="end the run after this long")
    s.add_argument("--launch", help="start this app first; the script's time 0 is just after")

    args = ap.parse_args(argv)
    if args.cmd == "button" and (args.action == "hold") != (args.ms is not None):
        ap.error("hold needs a time in ms, and only hold takes one")
    if not math.isfinite(args.wait) or args.wait <= 0:
        ap.error("--wait must be a positive number of seconds")
    if args.cmd == "run" and args.duration_ms is not None and args.duration_ms < 0:
        ap.error("--duration-ms must not be negative")

    if args.cmd == "run":
        try:
            steps = parse_script(args.script)
        except (ScriptError, OSError) as e:
            print(e, file=sys.stderr)
            return EXIT_BAD_SCRIPT
        out = Path(args.out)
        try:
            out.mkdir(parents=True, exist_ok=True)
            log = open(out / "watch.log", "wb")
        except OSError as e:
            print(f"error: cannot write to {out}: {e}", file=sys.stderr)
            return EXIT_FAILED
        with log:
            try:
                w = connect(args, log.write)
            except WatchError as e:
                print(f"error: {e}", file=sys.stderr)
                return EXIT_NO_WATCH
            runner = Runner(w, out, log)
            try:
                # Closing the USB drive reconnects the port, which ends a log stream: go home
                # first, then start the log.
                if args.launch:
                    w.show_home()
                w.cmd("dev log on")
                if args.launch:
                    runner.note(f"launching {args.launch} (pid {w.launch(args.launch)})")
                    w.pump(2.0)
                runner.run(steps, args.duration_ms)
            except WatchError as e:
                runner.failed = True
                runner.note(f"run stopped: {e}")
            finally:
                # Closing the port ends both on the watch anyway; these say so first.
                try:
                    for cleanup in ("dev sensor off", "dev log off"):
                        try:
                            w.cmd(cleanup)
                        except WatchError as e:
                            runner.failed = True
                            runner.note(f"'{cleanup}' failed: {e}")
                finally:
                    w.close()
        return EXIT_STEP_FAILED if runner.failed else EXIT_OK

    try:
        w = connect(args, None)
    except WatchError as e:
        print(f"error: {e}", file=sys.stderr)
        return EXIT_NO_WATCH

    try:
        if args.cmd == "log":
            # One decoder for the whole stream: a character can be split across reads.
            decoder = codecs.getincrementaldecoder("utf-8")("replace")

            def show(b):
                sys.stdout.write(decoder.decode(b))
                sys.stdout.flush()
            w.log_sink = show
            try:
                w.cmd("dev log on")
                end = time.monotonic() + args.seconds if args.seconds else None
                while end is None or time.monotonic() < end:
                    w.pump(0.2)
            except KeyboardInterrupt:
                pass
            except WatchError as e:
                print(f"error: {e}", file=sys.stderr)
                return EXIT_FAILED
            finally:
                # 'dev log off' can bring the last of the log: the decoder ends after it.
                try:
                    w.cmd("dev log off")
                except WatchError:
                    pass
                sys.stdout.write(decoder.decode(b"", final=True))
            return EXIT_OK

        try:
            if args.cmd in ("info", "apps"):
                if args.cmd == "info":
                    print("watch connected, Developer on"
                          + (f", firmware {w.firmware}" if w.firmware else ""))
                for a in w.apps():
                    folder = a["path"].replace("\\", "/").split("/")[-2] if "/" in a["path"] else ""
                    print(f"{a['pid']}  {a['type']:<9}  {folder:<20}  {a['name']}")
            elif args.cmd == "home":
                w.show_home()
            elif args.cmd == "update-kernel":
                update_kernel(w, args.ota, args.drive)
            elif args.cmd == "install":
                install(w, args.uapp, args.folder, args.drive)
            elif args.cmd == "launch":
                w.show_home()
                print(w.launch(args.app))
            elif args.cmd == "stop":
                print(w.stop(args.app))
            elif args.cmd == "screenshot":
                Path(args.file).write_bytes(png_from_abgr2222(*w.screen(), round_mask=not args.square))
                print(args.file)
            elif args.cmd == "button":
                w.button(args.name, args.action, args.ms)
        except (WatchError, OSError) as e:
            print(f"error: {e}", file=sys.stderr)
            return EXIT_FAILED
        return EXIT_OK
    finally:
        w.close()


if __name__ == "__main__":
    sys.exit(main())
