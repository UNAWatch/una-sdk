# Simulator script reference

## Lines and times

- One step per line: a time, then the step. `#` starts a comment, and blank lines are ignored.
- A time is in ms from the start of the GUI. `+N` means N ms after the previous step.
- A trailing `s` means seconds: `5s`, `+1.5s`.
- Times may not go backwards. The latest is 4294967295 ms.
- Steps at the same time run in file order.
- In a TouchGFX simulator with `UNA_SIM_SLOW` set, script times stretch with the frames. LVGL
  and glance simulators don't stretch them.

The whole script is checked before the simulator starts. An unknown step, a bad value, a missing
replay file or a time that goes backwards stops the run with exit code 2, naming the file and
line.

## Steps

| Step | Effect |
|---|---|
| `click B` | Press B and release it 100 ms later. B is `L1`, `L2`, `R1` or `R2`. |
| `hold B MS` | Press B and release it MS ms later (a whole number). Use for hold-to-confirm controls, for example `hold R1 3500` for a 3 s hold. |
| `press B` / `release B` | Press or release B on its own. |
| `screenshot NAME` | Save the display as NAME in the `--out` folder, as PNG. `.png` is added when NAME has no extension. NAME may name a folder inside `--out`, such as `shots/boot`; missing folders are made. NAME may not be an absolute path, contain `..` or `:`, or be `sim.log`. |
| `hr BPM` / `hr auto` | A fixed heart rate (whole number, 0-255, always trusted), or back to the simulated one. |
| `battery PCT` / `battery auto` | A fixed battery level, 0-100. |
| `pressure HPA` / `pressure auto` | A fixed barometric pressure, a positive number. |
| `gps fix on` / `off` / `auto` | Force the GPS fix. `on` skips the simulated search delay. `off` reports no fix and zero speed. `auto` has a fix once the simulated search has ended. |
| `gps replay FILE` | Replay a recorded track (format below). FILE is relative to the script. |
| `gps auto` | Stop the replay and the forced fix. |
| `wrist` | A wrist-raise motion. It fails (exit code 1) in a simulator built without the sensor simulation. |
| `log TEXT` | Write TEXT to the console and `sim.log`, to mark a point in the log. |
| `exit` | End the run. |

### Buttons

Buttons reach the app the way the watch's kernel reports them:
1. a press when the button goes down;
2. on release, a click, if the button was down for less than 500 ms;
3. then the release.

So `click` gives press, click, release, and `hold B 3000` gives press, then release with no
click.

A `click`, `hold` or `press` of a button that is already down does nothing. Glances have no
buttons: a button step in a glance script is a script error (exit code 2).

Button positions:
- L1 top left (up);
- L2 bottom left (down);
- R1 top right (select/start);
- R2 bottom right (back).

### GPS

- Without a fix, the simulated track stands still and the step count stops.
- A replay keeps going without a fix, so when the fix returns, the position jumps to where the
  replay has got to.
- Starting or stopping a replay moves the position straight to the other track, which may be far
  away. An app that adds up distance counts that jump. So start a replay at time 0, before the
  app has a fix:

```text
0      gps replay track.csv
0      gps fix on
```

## Replay files

CSV, one fix per line, with these columns:
- seconds from the start of the replay;
- latitude and longitude in degrees;
- altitude in metres (-1000 to 20000);
- speed in m/s (0 to 300).

A first line that isn't numbers is a header.

```text
t_s,lat,lon,alt_m,speed_mps
0,51.500000,-0.100000,20.0,3.0
1,51.500027,-0.100000,20.1,3.0
2,51.500054,-0.100000,20.1,3.0
```

- **Before the first line's time:** the fix is the first line's.
- **Between lines:** position, altitude and speed are interpolated, and a longitude that crosses
  ±180° goes the short way.
- **After the last line:** the position stays where it ended, with speed 0.

To make a track, generate points at a steady speed. At latitude φ, 1 m north is 1/111320
degrees, and 1 m east is 1/(111320 · cos φ) degrees.

## Screenshots

- **TouchGFX and LVGL:** a 240 × 240 PNG of the display as the watch shows it: round, black
  outside the glass, with no housing.
  - LVGL screenshots are the exact frame the watch would receive, expanded to 8 bits per channel.
  - In a TouchGFX simulator with the panel look on (`UNA_SIM_LOOK=watch`), the screenshot is
    toned like the panel.
- **Glance:** the screenshot is 240 × 60.

A screenshot taken before the first frame has been drawn fails (exit code 1), so take the first
one at 1000 ms or later.

## Logging

During a scripted or headless run, everything the app and the simulator log goes to
`<out>/sim.log` as well as the console. Each step is logged as it runs, with its time and script
line, as a line starting `[sim]`.

## Examples

Hold-to-confirm save at the end of an activity:

```text
0       gps fix on
2000    click R1          # start
+10000  click R1          # pause
+1000   screenshot paused
+0      hold R1 3500      # hold to save: released 3500 ms later
+5000   screenshot saved  # after the release, once the saved screen is drawn
+500    exit
```

A glance with a changing heart rate:

```text
0      hr 60
1500   screenshot rest
+0     hr 150
+2000  screenshot active
+0     exit
```
