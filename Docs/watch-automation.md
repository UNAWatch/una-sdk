# Driving a watch from a computer {#watch-automation}

With **Developer** switched on, a watch connected by USB can be driven from a computer with no
one touching it: buttons pressed, screenshots taken, apps installed and launched, sensor values
injected and the log read as it is written. `Utilities/Scripts/una_watch/una_watch.py` does all
of this, and runs the same script files as the [simulators](Simulator.md#scripted-runs), so a
script written against the simulator also runs on a watch.

## Turn on Developer

On the watch, open **Settings > Device**, scroll to **Developer** and press the top-right button
(R1) to switch it **On**. It stays on across restarts. While it is off, the watch refuses every
command below except the check of whether it is on.

Anything plugged into the watch's USB port can drive it while Developer is on, including reading
the screen. Switch it off when you are done.

## Set up the computer

Python 3.8 or newer and pyserial:

```bash
pip install pyserial
```

The tool finds the watch by its USB ID (VID 0483, PID 52A4), so no port needs naming. Pass
`--port` to choose one when several watches are connected.

## Commands

From the SDK root:

```bash
python3 Utilities/Scripts/una_watch/una_watch.py info
```

| Command | What it does |
|---|---|
| `info`, `apps` | Check the connection, and list the installed apps: service PID, type, folder, name. |
| `install FILE.uapp` | Copy the app to the watch over its USB drive, check the copy on the watch, and restart the watch so it finds the app. An update goes into the folder that already holds the app (matched by the app ID in the `.uapp`); a new app gets `Apps/<name>/`. `--folder` picks the folder, `--drive` the mount point. |
| `update-kernel FILE.ota` | Install watch firmware, as the phone app does: copy the `.ota` to `Update/` over the USB drive, check the copy, and hand it to the bootloader, which installs it on the restart that follows. The battery must be at least 10%. Prints the firmware version before and after. |
| `home` | Close the USB drive and dismiss the charging screen, leaving the home screen. |
| `launch APP` | `home`, then start an app by name or PID. |
| `stop APP` | Stop an app. |
| `screenshot FILE.png` | Save the display. |
| `button B ACTION [MS]` | `B` is `L1`, `L2`, `R1` or `R2`; `ACTION` is `click`, `press`, `release` or `hold MS`. |
| `log [--seconds N]` | Print the watch's log as it is written. |
| `run SCRIPT` | Run a simulator script on the watch (below). |

The exit code is 0 on success, 1 when the command failed (the message says why), and 5 when there
is no usable watch: none connected, firmware without Developer, or Developer off.

`install` never replaces another app's files: if the folder it would use holds a `.uapp` with a
different app ID, it stops and asks for `--folder`. With more than one watch connected, it needs
`--drive`.

## Running a simulator script on the watch

```bash
python3 Utilities/Scripts/una_watch/una_watch.py run run.txt --launch MyApp --out run-out
```

| Option | Meaning |
|---|---|
| `--launch APP` | Close the USB drive, dismiss the charging screen and start the app. The script's time 0 is 2 s after the start. |
| `--out DIR` | Where screenshots and `watch.log` go. Default: `watch-out`. |
| `--duration-ms N` | End the run after N ms. |

The script format, its steps and the replay file format are those of the simulators: see
[Script format](Simulator.md#scripted-runs). On the watch:

- Times run on the computer's clock from the start of the run.
- `click` holds the button 150 ms. `hold B MS` presses it and releases it MS ms later, while
  later steps run at their own times.
- `screenshot` saves what the watch shows: the kernel's whole screen, as PNG, black outside the
  round glass.
- `hr BPM` sets both heart-rate sensors (`HEART_RATE` and `HEART_RATE_EX`), always trusted.
- `gps fix on` without a replay reports a fixed position. `gps replay` sends one fix a second.
- `wrist` sends one wrist-motion event.
- `watch.log` holds the watch's log for the run, with each step marked `[run]`.

At the end of a run every injected sensor goes back to the real one.

Exit codes are as for the simulators: 0 the run ended, 1 a step failed, 2 bad options or
script. Code 5 means no usable watch.

## Sensor values

A script's sensor steps replace what a sensor publishes. Every listener gets the injected values
at the sensor's own rate: the app, and the watch's own consumers too, such as the daily
heart-rate record. Nothing is published while nothing is listening. The kernel's own battery and
GPS status icons read the hardware directly, so they keep showing the real state.

Injection ends when the serial port closes, so it lasts as long as the run that started it, and
when Developer is switched off.

Other tools inject with `dev sensor` (below). `TYPE` is the `SDK::Sensor::Type` value in hex
(`SDK/SensorLayer/SensorTypes.hpp`), and the values are the type's fields in the order of its
parser (`SDK/SensorLayer/DataParsers/`): a plain number is a float, `3u` an unsigned field and
`-3i` a signed one. For example, a GPS fix at 51.5 N, 0.1 W, 20 m, accurate to 3 m:

```text
dev sensor 110 3 1u 51.5 -0.1 20
```

## What happens on the watch

- **The USB drive.** Plugging the watch in opens its USB drive, and apps are stopped while it is
  open. The tool closes it (`usb drive off`) before starting an app, and opens it only to copy
  files. Whenever it closes the drive, the tool first makes the computer let go of it: on
  Windows it locks and dismounts the volume, which writes out everything cached; on macOS it
  unmounts it with `diskutil`, and on Linux with `gio mount -u` or `umount`. A drive taken back
  while the computer still has writes cached can lose them and damage the watch's filesystem.
  If a program has files open on the drive, or the user may not unmount it (on Linux, a mount
  without the `user` option that the desktop didn't make), the tool stops and leaves the drive
  open.
- **Installing.** The app list is built when the watch starts, so `install` restarts the watch.
  A `.uapp` ends with the CRC-32 of the rest of the file; the copy is checked against everything
  before those last 4 bytes.
- **Test mode.** For a few seconds after the watch starts with USB connected, any input on its
  serial port puts it in test mode, where only factory commands work. When the tool lands there
  it sends `boot`, which restarts the watch without that window, and carries on.

## The serial commands

The tool uses the `dev` command of the watch's USB serial console (115200 8N1; send lines ending
in LF). Other tools can use it directly. Each command ends with a line `#OK ...` or
`#ERR <reason>`.

| Command | Reply |
|---|---|
| `dev` | `#OK <protocol> on\|off <firmware>`: the protocol version (3), Developer, and the firmware version. |
| `dev button L1\|L2\|R1\|R2 click\|press\|release\|hold <ms>` | `#OK` once done. |
| `dev screen` | `#DATA <bytes> <width> <height> abgr2222 <crc32>`, the pixels, then `#OK`. One byte per pixel: bits 5-4 blue, 3-2 green, 1-0 red. |
| `dev apps` | One line per app, tab-separated: service PID, type, app ID, `.uapp` path, name. Then `#OK`. |
| `dev launch <pid>`, `dev stop <pid>` | `#OK`. Either of an app's PIDs is accepted. |
| `dev crc <path> [bytes]` | `#OK <size> <crc32>`: of the file, or of its first `bytes`. Paths are on the apps volume, as `/Apps/...`. |
| `dev sensor <type> <values...>` | `#OK active\|idle`: whether anything is listening. `dev sensor <type> off`, or `dev sensor off` for all. |
| `dev log on\|off` | `#OK`. While on, log text arrives between commands, between the bytes 0x1E and 0x1F. |
| `dev update /Update/<file>.ota` | `#OK`, then the watch copies the file to its bootloader and restarts into it, which installs it. A refusal after the `#OK` arrives as `#ERR`. Needs the USB drive closed and the battery at 10% or more. |

Injected values and the log stream stop when the port closes or Developer goes off.

In test mode (prompt `una-test>`), `boot` leaves test mode as described above.
