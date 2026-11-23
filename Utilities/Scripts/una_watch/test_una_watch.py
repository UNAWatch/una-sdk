#!/usr/bin/env python3
"""Self-contained tests for una_watch.py (run: python test_una_watch.py).

No watch and no pyserial needed: the console is a fake that answers like the watch. Covers
the script parser, replay interpolation, PNG output, log frames, command replies, screenshots,
the .uapp check and finding an app's folder."""
import struct
import sys
import tempfile
import zlib
from pathlib import Path

sys.dont_write_bytecode = True   # leave no __pycache__ beside the script
sys.path.insert(0, str(Path(__file__).resolve().parent))
import una_watch as uw  # noqa: E402

failures = []


def expect(cond, msg):
    print(("ok:   " if cond else "FAIL: ") + msg)
    if not cond:
        failures.append(msg)


def raises(fn, exc):
    try:
        fn()
    except exc:
        return True
    return False


class FakePort:
    """Answers each command line with canned bytes, as the watch's console would."""

    def __init__(self, replies, clock=None):
        self.replies = replies
        self.pending = bytearray()
        self.sent = []
        self.clock = clock
        self.timed = []         # (clock time, line) of each write, with a clock

    def write(self, data):
        line = data.decode().strip()
        self.sent.append(line)
        if self.clock:
            self.timed.append((self.clock.t, line))
        self.pending += b"una> " + data
        for prefix, reply in self.replies:
            if line.startswith(prefix):
                self.pending += reply(line) if callable(reply) else reply
                break

    def read(self, n):
        if self.clock:
            self.clock.t += 0.005       # each read takes 5 ms of fake time
        out = bytes(self.pending[:n])
        del self.pending[:n]
        return out


class FakeClock:
    """Stands in for the time module in una_watch: time passes only as the fake port is read."""

    def __init__(self):
        self.t = 1000.0

    def monotonic(self):
        return self.t

    def sleep(self, s):
        self.t += s

    def close(self):
        pass


def crc_of_file(on_watch):
    """A 'dev crc' reply for a file on the watch holding these bytes."""
    def reply(line):
        parts = line.split()
        n = int(parts[3]) if len(parts) == 4 else len(on_watch)
        return b"#OK %d %08X\n" % (len(on_watch), zlib.crc32(on_watch[:n]))
    return reply


def watch_with(replies, sink=None):
    w = uw.Watch(log_sink=sink)
    w.ser = FakePort(replies)
    return w


def main():
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)

        # --- script parsing --------------------------------------------------
        (tmp / "track.csv").write_text("t_s,lat,lon,alt_m,speed_mps\n0,51.5,-0.1,20,3\n10,51.5003,-0.1,21,3\n")
        good = tmp / "good.txt"
        good.write_text("# comment\n0 gps replay track.csv\n0 gps fix off\n1500 screenshot shots/01\n"
                        "+500 gps fix on\n+0 hr 152\n+1.5s click R1\n+0 hold R1 3000\n5s battery 40\n"
                        "+0 pressure 1013.2\n+0 wrist\n+0 log marker text\n+0 gps auto\n+500 exit\n")
        steps = uw.parse_script(good)
        expect([s[2] for s in steps] == ["gps", "gps", "screenshot", "gps", "hr", "click", "hold",
                                          "battery", "pressure", "wrist", "log", "gps", "exit"],
               "a script parses into its steps")
        expect([s[0] for s in steps][:7] == [0, 0, 1500, 2000, 2000, 3500, 3500],
               "absolute, +N and seconds times")
        expect(steps[-3][3] == ["marker text"], "log keeps its whole text")
        expect(len(steps[0][3][1]) == 2, "a replay file is read relative to the script")

        bad_cases = {
            "unknown step": "0 jump L1\n",
            "bad button": "0 click L3\n",
            "hold without ms": "0 hold R1\n",
            "time going back": "100 click L1\n50 click L1\n",
            "bad time": "abc click L1\n",
            "screenshot outside": "0 screenshot ../x\n",
            "screenshot rooted (Windows)": "0 screenshot \\\\server\\x\n",
            "screenshot on a drive": "0 screenshot C:x\n",
            "screenshot as the log": "0 screenshot ./watch.log\n",
            "infinite time": "inf click L1\n",
            "time not a number": "nan click L1\n",
            "time past the limit": "4294967296 click L1\n",
            "missing replay": "0 gps replay nope.csv\n",
            "bad hr": "0 hr fast\n",
        }
        for name, text in bad_cases.items():
            p = tmp / "bad.txt"
            p.write_text(text)
            expect(raises(lambda: uw.parse_script(p), uw.ScriptError), f"rejected: {name}")
        expect(uw.main(["run", str(tmp / "bad.txt")]) == uw.EXIT_BAD_SCRIPT, "a bad script exits 2")
        expect(uw.screenshot_name_ok("shots/boot") and uw.screenshot_name_ok("a.png"),
               "relative screenshot names are accepted")

        # --- replay ----------------------------------------------------------
        rows = uw.load_replay(tmp / "track.csv")
        lat, lon, alt, speed = uw.replay_at(rows, 5)
        expect(abs(lat - 51.50015) < 1e-9 and abs(alt - 20.5) < 1e-9, "a replay interpolates")
        expect(uw.replay_at(rows, -1)[0] == 51.5, "before the first fix: the first fix")
        expect(uw.replay_at(rows, 99)[3] == 0.0, "after the last fix: speed 0")

        # --- PNG -------------------------------------------------------------
        px = bytes([0xC0 | 0x03] * 4)            # 2x2, red channel 3 (full)
        png = uw.png_from_abgr2222(2, 2, px, round_mask=False)
        idat = png[png.index(b"IDAT") + 4:png.index(b"IEND") - 8]
        raw = zlib.decompress(idat)
        expect(png.startswith(b"\x89PNG") and struct.unpack(">II", png[16:24]) == (2, 2), "PNG header")
        expect(raw[:7] == b"\0\xff\0\0\xff\0\0", "ABGR2222 red widens to 255,0,0")
        big = uw.png_from_abgr2222(240, 240, bytes([0xFF] * 240 * 240))
        raw = zlib.decompress(big[big.index(b"IDAT") + 4:big.index(b"IEND") - 8])
        expect(raw[1:4] == b"\0\0\0" and raw[120 * 721 + 1 + 360:120 * 721 + 1 + 363] == b"\xff\xff\xff",
               "black outside the round glass, the frame inside")

        # --- console protocol ------------------------------------------------
        logs = []
        w = watch_with([("dev crc", b"\x1e    1 App -I- hello\x1f#OK 10 0000ABCD\n")], logs.append)
        payload, lines = w.cmd("dev crc /x")
        expect(payload == "10 0000ABCD", "#OK payload")
        expect(b"".join(logs) == b"    1 App -I- hello", "log frames are taken out of replies")

        w = watch_with([("dev button", b"#ERR developer mode is off\n")])
        expect(raises(lambda: w.button("R1", "click"), uw.WatchError), "#ERR raises")

        w = watch_with([("dev", b"\x1b[36mdeveloper mode: on\n#OK 2 on\n")])
        w.require_developer_mode()
        expect(True, "Developer on with protocol 2 is accepted")
        w = watch_with([("dev", b"developer mode: off\n#OK 2 off\n")])
        expect(raises(w.require_developer_mode, uw.WatchError), "Developer off is refused")
        w = watch_with([("dev", b"developer mode: on\n#OK 3 on 1.8.0\n")])
        w.require_developer_mode()
        expect((w.protocol, w.firmware) == (3, "1.8.0"), "protocol 3 reports the firmware version")

        w = watch_with([("dev", b"developer mode: on\n#OK 2 on\n")])
        w.require_developer_mode()
        (tmp / "k.ota").write_bytes(b"kernel")
        expect(raises(lambda: uw.update_kernel(w, tmp / "k.ota"), uw.WatchError) and
               not any(s.startswith("usb") for s in w.ser.sent),
               "update-kernel refuses firmware without 'dev update', touching nothing")
        w = watch_with([("dev", b"developer mode: on\n#OK 3 on 1.8.0\n")])
        w.require_developer_mode()
        try:
            uw.update_kernel(w, tmp / "track.csv")
            err = ""
        except uw.WatchError as e:
            err = str(e)
        expect("not an .ota file" in err and not any(s.startswith("usb") for s in w.ser.sent),
               "update-kernel takes only an .ota, even on a watch that has 'dev update'")

        # The watch refuses the kernel: its reason is reported, and nothing waits for a restart.
        drive = tmp / "drive"
        drive.mkdir()
        saved = uw.open_drive, uw.close_drive, uw.Watch.count_ports
        uw.open_drive = lambda watch, drive_arg=None: drive
        uw.close_drive = lambda watch, d: None
        uw.Watch.count_ports = staticmethod(lambda: 1)
        try:
            w = watch_with([("dev crc", crc_of_file(b"kernel")),
                            ("dev update", b"#ERR not a kernel for this watch\n"),
                            ("dev", b"developer mode: on\n#OK 3 on 1.8.0\n")])
            w.require_developer_mode()
            w.reopen = lambda *a, **k: failures.append("update-kernel waited for a restart")
            try:
                uw.update_kernel(w, tmp / "k.ota")
                err = ""
            except uw.WatchError as e:
                err = str(e)
            expect("not a kernel for this watch" in err and (drive / "Update" / "k.ota").is_file(),
                   "a kernel the watch refuses ends update-kernel with the watch's reason")
        finally:
            uw.open_drive, uw.close_drive, uw.Watch.count_ports = saved

        # Giving the drive back fails: the computer's hold on the volume is still let go.
        class Held:
            released = False

            def release(self):
                Held.released = True
        saved = uw.let_go_of_drive
        uw.let_go_of_drive = lambda d: Held()
        try:
            w = watch_with([("usb", b"#OK\n")])

            def lost(*a, **k):
                raise uw.WatchError("lost the watch")
            w.reopen = lost
            expect(raises(lambda: uw.close_drive(w, drive), uw.WatchError) and Held.released,
                   "the volume is released even when the watch does not come back")
        finally:
            uw.let_go_of_drive = saved

        frame = bytes(range(256)) * 225          # 57600 bytes, includes 0x1E and 0x1F
        w = watch_with([("dev screen", b"#DATA 57600 240 240 abgr2222 %08X\n" % zlib.crc32(frame)
                         + frame + b"\n#OK\n")], logs.append)
        logs.clear()
        width, height, got = w.screen()
        expect((width, height, got) == (240, 240, frame) and not logs,
               "a screenshot arrives whole, log markers in pixels untouched")
        w = watch_with([("dev screen", b"#DATA 4 2 2 abgr2222 00000000\n\1\2\3\4\n#OK\n")])
        expect(raises(w.screen, uw.WatchError), "a damaged screenshot is refused")

        apps_reply = (b"F04FE720\tutility\t1111222233334444\t2:/Apps/Stopwatch/Stopwatch_1.0.0.uapp\tStopwatch\n"
                      b"3397BEA0\tclockface\tED66C90184FF55A0\t2:/Apps/ED66C90184FF55A0/ED66C90184FF55A0_1.0.4.uapp"
                      b"\tVoxel Meteor\n#OK\n")
        w = watch_with([("dev apps", apps_reply), ("dev launch", b"#OK\n")])
        apps = w.apps()
        expect([a["name"] for a in apps] == ["Stopwatch", "Voxel Meteor"], "app names may hold spaces")
        expect(w.launch("Voxel Meteor") == "3397BEA0" and w.ser.sent[-1] == "dev launch 3397BEA0",
               "launch by name sends the app's pid")
        expect(raises(lambda: w.launch("Nope"), uw.WatchError), "launching a missing app fails")

        # --- installing --------------------------------------------------------
        body = struct.pack("<Q", 0xED66C90184FF55A0) + b"rest of the app"
        uapp = body + struct.pack("<I", zlib.crc32(body))
        expect(zlib.crc32(uapp) == 0x2144DF1C, "a whole intact .uapp always has CRC 2144DF1C")
        expect(uw.app_folder(apps, uapp) == "ED66C90184FF55A0", "an update finds its folder by app ID")
        expect(uw.app_folder(apps, struct.pack("<Q", 1) + b"x" * 8) is None, "a new app has no folder")

        crc_of = crc_of_file
        w = watch_with([("dev crc", crc_of(uapp))])
        expect(w.file_matches("/Apps/x/x.uapp", uapp)[0], "the copy on the watch matches")
        other = body[:-1] + b"X" + struct.pack("<I", zlib.crc32(body[:-1] + b"X"))
        expect(not w.file_matches("/Apps/x/x.uapp", other)[0],
               "another intact build of the same size does not")
        w = watch_with([("dev crc", crc_of(uapp[:-1] + bytes([uapp[-1] ^ 0xFF])))])
        expect(not w.file_matches("/Apps/x/x.uapp", uapp)[0],
               "a copy damaged only in its last 4 bytes does not")

        # --- a hold doesn't hold up the timeline (on a fake clock) ---------------
        tiny = bytes([0xC0] * 4)
        screen_reply = b"#DATA 4 2 2 abgr2222 %08X\n" % zlib.crc32(tiny) + tiny + b"\n#OK\n"

        def timed_run(text, replies):
            clock = FakeClock()
            real_time, uw.time = uw.time, clock
            try:
                w = uw.Watch()
                w.ser = FakePort(replies, clock)
                script = tmp / "timed.txt"
                script.write_text(text)
                with open(tmp / "watch.log", "wb") as log:
                    runner = uw.Runner(w, tmp / "out", log)
                    start = clock.t
                    runner.run(uw.parse_script(script))
                return runner, [(round(t - start, 2), s) for t, s in w.ser.timed
                                if s.startswith("dev ")]
            finally:
                uw.time = real_time

        def near(t, want):
            return abs(t - want) <= 0.05

        runner, timed = timed_run("0 hold R1 600\n+100 screenshot during\n",
                                  [("dev button", b"#OK\n"), ("dev screen", screen_reply)])
        expect([s for _, s in timed] == ["dev button R1 press", "dev screen", "dev button R1 release"],
               "a screenshot during a hold is taken while the button is down")
        expect(near(timed[1][0], 0.1) and near(timed[2][0], 0.6),
               f"at its own time, 100 ms in, and the release 600 ms in ({timed})")
        expect((tmp / "out" / "during.png").is_file() and not runner.failed, "and saved")

        runner, timed = timed_run("0 press R1\n+100 hold R1 300\n+100 click R1\n+100 release R1\n",
                                  [("dev button", b"#OK\n")])
        expect([s for _, s in timed] == ["dev button R1 press", "dev button R1 release"],
               "a hold or click of a button already down does nothing, as in the simulators")

        runner, timed = timed_run("0 press R1\n+0 hold R2 100\n+200 log done\n",
                                  [("dev button", b"#OK\n")])
        expect([s for _, s in timed] == ["dev button R1 press", "dev button R2 press",
                                         "dev button R2 release", "dev button R1 release"],
               "the end of a run releases a button left pressed, once, after the holds")

        fails = {"n": 0}

        def flaky(line):
            if line.endswith("release") and fails["n"] < 1:
                fails["n"] += 1
                return b"#ERR busy\n"
            return b"#OK\n"
        runner, timed = timed_run("0 hold R1 200\n", [("dev button", flaky)])
        expect([s for _, s in timed] == ["dev button R1 press", "dev button R1 release",
                                         "dev button R1 release"] and near(timed[2][0], 1.2),
               "a release that fails to send is tried again a second later")

        (tmp / "x.uapp.bak").write_bytes(b"someone's")
        expect(uw.backup_name(tmp / "x.uapp").name == "x.uapp.bak2",
               "a backup never takes the name of a .bak already there")

        expect(uw.folder_name_ok("Stopwatch") and uw.folder_name_ok("ED66C90184FF55A0"),
               "app folder names are accepted")
        expect(not any(uw.folder_name_ok(n) for n in ("", "..", "a/b", "a\\b", "C:x", " x")),
               "folder names that are paths are refused")

    if failures:
        print(f"\n{len(failures)} failure(s)")
        return 1
    print("\nall passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
