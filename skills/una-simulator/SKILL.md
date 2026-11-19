---
name: una-simulator
description: Build and run a UNA Watch app in the desktop simulator, and drive it with a script, headless, with no window, keyboard or display. Scripts press buttons, set heart rate, GPS, battery and pressure, replay GPS tracks and save PNG screenshots, and the exit code says pass or fail. Use to see or check a UNA Watch app's screens or behaviour without a watch, to take screenshots, or to test a flow end to end.
---

# Run a UNA Watch app in the simulator

The simulator compiles the app's real service and GUI code for the PC and runs it against a
mock kernel, with simulated sensors and a mock file system. Drive it with a **script** and run it
**headless**. Don't send keystrokes to a window, and don't take the user's keyboard focus.

A run produces:
- PNG screenshots, which you can open and look at;
- `sim.log`, holding everything the app and the simulator logged;
- an exit code.

## 1. Build the simulator

Each GUI toolkit has its own simulator project inside the app:

| App | Project | Hosts |
|---|---|---|
| LVGL GUI | `Software/Apps/LVGL-GUI/simulator/CMakeLists.txt` | Windows, Linux, macOS |
| Glance | `Software/simulator/CMakeLists.txt` | Windows, Linux, macOS |
| TouchGFX GUI | `Software/Apps/TouchGFX-GUI/simulator/msvs` (Visual Studio), `simulator/gcc/Makefile` (Linux) | Windows, Linux x86-64 |

Simulators don't need the ARM toolchain. They need a host C++ compiler and SDL2. The commands
below assume `UNA_SDK` is set.

### LVGL and glance (CMake)

```bash
# Linux / WSL: apt install build-essential cmake ninja-build libsdl2-dev
# macOS: brew install cmake ninja sdl2
cmake -S <app>/Software/Apps/LVGL-GUI/simulator -B <app>/Software/Apps/LVGL-GUI/simulator/build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build <app>/Software/Apps/LVGL-GUI/simulator/build
# executable: simulator/build/bin/<Name>Simulator
```

```powershell
# Windows, any shell. Without an installed SDL2 the build reuses the 32-bit SDL2 that
# TouchGFX Designer ships, so it must be 32-bit (-A Win32). The "Visual Studio 18 2026"
# generator needs Visual Studio 2026 and CMake 4.2 or newer; with Visual Studio 2022 or an
# older CMake, use -G "Visual Studio 17 2022".
cmake -S <app>\Software\Apps\LVGL-GUI\simulator -B <app>\Software\Apps\LVGL-GUI\simulator\build -G "Visual Studio 18 2026" -A Win32
cmake --build <app>\Software\Apps\LVGL-GUI\simulator\build --config Debug
# executable: simulator\build\bin\<Name>Simulator.exe
```

For a glance, use `Software/simulator` in place of `Software/Apps/LVGL-GUI/simulator`. A glance
simulator built with a Visual Studio generator puts its executable in `build/bin/Debug`. The
LVGL simulators set a flat `build/bin` for every configuration.

**macOS:** untested by the SDK maintainers. On Apple Silicon, Homebrew's SDL2 is arm64, so the
simulator must be built arm64. A CMake running under Rosetta needs
`-DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_PREFIX_PATH=/opt/homebrew`.

### TouchGFX

The TouchGFX simulator needs the app's `generated/` code, which the examples ship. Regenerate
only after changing the design (skill `una-new-app`).

```powershell
# Windows: needs TouchGFX 4.26.1 installed (for its asset converters) and MSVC v143
# ("Desktop development with C++"). Application.props expects TouchGFX in C:\TouchGFX\4.26.1;
# for another location, set the TouchGFXEnvPath environment variable to its env folder.
# Find MSBuild with vswhere:
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
& $msbuild <app>\Software\Apps\TouchGFX-GUI\simulator\msvs\Application.sln /p:Configuration=Debug /p:Platform=x86 /m /nologo /verbosity:minimal
# executable: TouchGFX-GUI\build\Debug\bin\Application.exe
```

```bash
# Linux x86-64: apt install build-essential libsdl2-dev libsdl2-image-dev libjpeg-dev ruby ruby-nokogiri
cd <app>/Software/Apps/TouchGFX-GUI
make -f simulator/gcc/Makefile -j"$(nproc)"     # UNA_SDK must be an absolute path
# executable: build/bin/simulator.out
```

A source file added to a TouchGFX app must go into both `simulator/msvs/Application.vcxproj`
(`ClCompile`/`ClInclude`) and `simulator/gcc/Makefile` (`ADDITIONAL_SOURCES_UNA` /
`ADDITIONAL_INCLUDE_PATHS`). The `.uapp` build picks up sources from `libs.cmake`, but the
simulator projects list files one by one.

## 2. Write a script

One step per line: a time in ms from the GUI's start (`+N` = N ms after the previous step, a
trailing `s` = seconds), then the step. `#` starts a comment.

```text
# Start a run with a fixed heart rate and check the screens
0       gps fix on
2000    screenshot 01-start
+0      hr 145
+500    click R1
+5000   screenshot 02-running
+0      click R1
+1500   screenshot 03-paused
+500    exit
```

The steps:
- **Buttons:** `click B`, `hold B MS`, `press B`, `release B`, where B is `L1`, `L2`, `R1` or
  `R2`.
- **Screenshots:** `screenshot NAME`.
- **Sensors:** `hr BPM|auto`, `battery PCT|auto`, `pressure HPA|auto`, `gps fix on|off|auto`,
  `gps replay FILE.csv`, `gps auto`, `wrist`.
- **Other:** `log TEXT` and `exit`.

`references/script-reference.md` has the full rules: values, button timing and replay files.
Read it before writing anything beyond clicks and screenshots.

To work out which button does what, read the app's GUI code. TouchGFX handles them in each
view's `handleKeyEvent`; LVGL apps in their screens. A button's position is: L1 top left, L2
bottom left, R1 top right, R2 bottom right. In the examples, R1 is usually start/select and R2
back.

## 3. Run it

Run from the executable's folder. Always pass `--headless`, `--out` and `--fs-root`, so the run
needs no display, writes where you expect and starts from known data:

```bash
cd <folder with the executable>
export SDL_AUDIODRIVER=dummy   # no sound device in containers and CI
./RunLVGLSimulator --headless --script /path/run.txt --out /path/run-out --fs-root /path/run-fs
echo "exit $?"
```

```powershell
# TouchGFX on Windows is a GUI-subsystem program: use Start-Process to wait for it and get its exit code
$p = Start-Process -FilePath .\Application.exe -ArgumentList '--headless','--script','C:\work\run.txt','--out','C:\work\run-out','--fs-root','C:\work\run-fs' -Wait -PassThru -NoNewWindow
$p.ExitCode
```

| Option | Meaning |
|---|---|
| `--script FILE` | Run the steps in FILE. |
| `--headless` | No window. Needs `--duration-ms` or an `exit` step. |
| `--duration-ms N` | End the run after N ms. |
| `--out DIR` | Where screenshots and `sim.log` go (default `sim-out`). |
| `--fs-root DIR` | Root of the mock file system: what the app reads and writes. A fresh folder means no saved settings or activities. Copy a `settings.json` into it to start from known settings. |

| Exit code | Meaning |
|---|---|
| 0 | The run ended normally. |
| 1 | A step failed: a screenshot or replay file, or a lost button event. A glance simulator also exits 1 when the glance fails to start or draws nothing. |
| 2 | Bad options or script. Nothing ran; the message names the line. |
| 3 | The simulator didn't shut down within 15 s of the end of the run. |
| 4 | Out of memory: an LVGL app exhausted its pool. |

Options can also come from the `UNA_SIM_ARGS` environment variable, for example when the user
starts the simulator from an IDE.

## 4. Check the result

- **Look at the screenshots.** Open each PNG and describe what is on screen. Compare it with what
  the step before it should have caused.
- **Read `<out>/sim.log`.** Each step is logged as `[sim]` with its time and script line, between
  the app's own log lines. Look for errors and warnings the app logged.
- **Check the files the app wrote.** For example `settings.json` or activity files under
  `--fs-root`.
- **Report the exit code,** and for a failure the log lines around it.

## What the simulator doesn't show

Tell the user to check these on a watch (skill `una-test-on-watch`):

- **Messages over 256 bytes.** The watch drops them, but the simulator delivers them.
- **Memory:**
  - an LVGL app's pool in a 64-bit build is twice the watch's. A 32-bit build (`-A Win32`, or
    `-m32` on Linux) has exactly the watch's, so size the pool there;
  - the PC has no RAM limit for the service or the GUI.
- **Timing:** the service runs on the PC's clock, and frames on the PC can be faster than on the
  watch.
- **Hardware:** real sensor behaviour, GPS accuracy and the grade sensor (never valid in the
  simulator).
- **Display:**
  - glance fonts differ (Montserrat in place of Poppins);
  - the panel shows 2 bits per channel. TouchGFX simulators preview its tones with
    `UNA_SIM_LOOK=watch`.

## Troubleshooting

- **Exit code 2:** the message gives the script file and line. Fix the step.
- **Screenshot fails right after start (exit 1):** no frame has been drawn yet. Take the first
  screenshot after 1000 ms or more.
- **Distance jumps by hundreds of km:** a `gps replay` started after the app had a fix. Start
  replays at time 0, before `gps fix on`.
- **The app shows old data:** `--fs-root` points at a folder from an earlier run. Use a fresh one.
- **LVGL out of memory (exit 4):** raise `LV_MEM_SIZE`, or use less LVGL memory. It is set
  twice: in the `-CMake` project's GUI compile definitions and in
  `LVGL-GUI/simulator/CMakeLists.txt`. Change both. The message gives the pool size and use.
