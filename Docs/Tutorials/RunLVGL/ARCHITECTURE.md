(tutorials/runlvgl/architecture)=

# RunLVGL - Building a Watch App GUI with LVGL

RunLVGL is the Run activity app with its GUI process rewritten on [LVGL](https://lvgl.io)
instead of TouchGFX. It has the same screens, fonts, icons, menus and workout flow as
Run, records the same FIT activities, and runs on the same kernel with no kernel
changes. It exists so that developers who prefer LVGL can see a complete, shipping-grade
app built with it, and use its port and simulator for their own apps.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Examples/Apps/RunLVGL)

## What You'll Learn

- What the kernel gives a GUI process, and why the toolkit is the app's choice
- How the SDK's LVGL port turns LVGL's rendering into kernel frames, ticks and buttons
- How the RunLVGL GUI is organised: model, screens, widgets, theme and assets
- How to build for the watch on Windows or Linux, and run the PC simulator
- Where the memory goes in an LVGL GUI process, and how to keep it small

This tutorial assumes you have read the earlier tutorials, in particular
[HelloWorld](../HelloWorld/ARCHITECTURE.md) for the service/GUI split and the
[Sensors](../Sensors/ARCHITECTURE.md) tutorial for the sensor layer the service uses.
The service half of RunLVGL is a copy of Run's and is not covered again here; see
[Running - Fitness Tracking](../../Examples/Running-Architecture.md).

## Getting Started

### Prerequisites

Everything in the [toolchain setup](toolchain-setup), plus the LVGL submodule. LVGL is a
git submodule of the SDK, pinned to a release tag (v9.5.0), so check it out once:

```bash
cd $UNA_SDK
git submodule update --init ThirdParty/lvgl
```

The converted fonts and images are committed, so building the app needs neither Node
nor Python. Regenerating them does; see [Assets](#assets).

### Building for the Watch

The same recipe as every other app:

```bash
cd $UNA_SDK/Examples/Apps/RunLVGL/Software/Apps/RunLVGL-CMake
mkdir build && cd build
cmake ..
make
```

On Windows without Docker, use Ninja and the ST toolchain from STM32CubeIDE on `PATH`;
the CMake generator is the only difference (`cmake -G Ninja ..`). The final copy step
into `Software/Output` uses a glob that fails under CMake's `-E copy`; the `.uapp` is
still produced in the build directory and can be copied by hand.

Install `RunLVGL_<version>.uapp` into `D:\Apps\RunLVGL\` on the watch with
`Utilities/Scripts/Update-Watch-Apps.ps1`, or by copying it there. The app appears in
the launcher as **RunLVGL**.

### Running on the Simulator

RunLVGL has a PC simulator that runs the real service and GUI processes against the
SDK's mock kernel, with the display in an SDL2 window. Unlike the TouchGFX simulators,
it is a plain CMake project and builds on Windows and Linux.

Windows, with Visual Studio (32-bit, because it reuses the SDL2 that TouchGFX ships; use
the generator name for your Visual Studio version, e.g. `"Visual Studio 17 2022"`):

```powershell
$env:UNA_SDK = "C:/path/to/una-sdk"
cd $env:UNA_SDK\Examples\Apps\RunLVGL\Software\Apps\LVGL-GUI\simulator
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Debug
build\bin\RunLVGLSimulator.exe
```

Linux (Debian/Ubuntu):

```bash
sudo apt-get install build-essential cmake ninja-build libsdl2-dev
export UNA_SDK=/path/to/una-sdk
cd $UNA_SDK/Examples/Apps/RunLVGL/Software/Apps/LVGL-GUI/simulator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build/bin && ./RunLVGLSimulator)
```

Keys are the watch buttons: **1** = L1 (up), **2** = L2 (down), **3** = R1 (select),
**4** = R2 (back). Holding a key is a press, releasing it a release, and a hold shorter
than 500 ms is also a click, the same events the kernel emits. **5** raises a simulated
wrist-motion event and **Esc** closes the app. Run the executable from `build/bin`: the
mock file system is `../../../../../Output` relative to the working directory, which
resolves to `Software/Output`, so recorded activities land next to the `.uapp` output.
The simulated GPS gets a fix after four seconds and the heart rate starts a few seconds
later; both are configured in `simulator/ConfigurationSimulator.hpp`.

`simulator/linux-check.sh` reproduces the Linux build, a headless smoke run
(`SDL_VIDEODRIVER=dummy`) and the asset regeneration in an Ubuntu container, and is
what the Linux checks for this app are based on.

## Why LVGL Works on a TouchGFX Kernel

The kernel does not know which toolkit draws an app's screens. A GUI process is an ELF
loaded into SRAM that talks to the kernel through messages, and the contract is small:

| The kernel provides | Form |
|---|---|
| A frame tick | `EVENT_GUI_TICK`, 10 per second |
| Button input | `EVENT_BUTTON` with press, click and release for each of the four buttons |
| Lifecycle | `COMMAND_APP_GUI_RESUME`, `COMMAND_APP_GUI_SUSPEND`, `COMMAND_APP_STOP` |
| Service messages | Any app-defined message type, forwarded from the service process |
| **The GUI provides** | |
| Frames | `REQUEST_DISPLAY_UPDATE` with a pointer to a 240 x 240 buffer of one ABGR2222 byte per pixel, row-major |

The kernel copies the buffer into its own surface and composites it. Nothing in that
contract mentions TouchGFX. `SDK::GuiCommandProcessor` (called `TouchGFXCommandProcessor`
before this app existed; the old name remains as an alias) is the message pump that
implements the GUI's side of it, and both ports use it unchanged.

Two constraints are worth knowing before choosing a toolkit. The GUI process runs in
a fixed RAM region reserved when it is loaded (`GUI_RAM_LENGTH` in the app's CMake), so
a toolkit's working memory is best kept on a static pool of known size inside it. And
the app's C library is the kernel's export table rather than a full libc; RunLVGL does
not rely on `float` formatting from it and formats every number as integers
(`gui/include/gui/Format.hpp`).

## The LVGL Port

The port lives in the SDK so any app can use it:

| File | Role |
|---|---|
| `Libs/Header/SDK/Port/LVGL/lv_conf.h` | LVGL configuration for the watch |
| `Libs/Header/SDK/Port/LVGL/LvglPort.hpp`, `Libs/Source/Port/LVGL/LvglPort.cpp` | `SDK::LVGL::Port`: display, tick, buttons, lifecycle, frame loop |
| `Libs/Source/AppSystem/EntryPoint/LVGL/main.cpp` | The GUI process entry point |
| `Libs/Source/Port/GuiCommandProcessor.cpp` | The kernel message pump shared with the TouchGFX port |
| `cmake/una-sdk.cmake` | `UNA_SDK_SOURCES_GUI_LVGL`, `UNA_SDK_INCLUDE_DIRS_GUI_LVGL`, `UNA_SDK_DEFINES_GUI_LVGL` |

An app selects LVGL in its CMake by using those three variables in place of the
TouchGFX ones and pointing `GUI_PATH` at its GUI source tree; compare
`RunLVGL-CMake/CMakeLists.txt` with `Running-CMake/CMakeLists.txt`.

### Display

LVGL renders in RGB565 into a 30-row stripe buffer (14 KB) in `PARTIAL` render mode.
Each flushed stripe is packed into a persistent 57.6 KB ABGR2222 frame, keeping the top
two bits of each channel, which is the same quantisation the TouchGFX port's
`LCD8bpp_ABGR2222` applies, so both toolkits produce identical colours from identical
input. When LVGL reports the last stripe of a frame (`lv_display_flush_is_last`), the
frame is sent with `REQUEST_DISPLAY_UPDATE`. Because the frame persists, LVGL's partial
redraws compose correctly onto the previous content, and only the invalidated areas are
re-rendered each frame.

### Tick and Frame Loop

`lv_tick_set_cb` reads the kernel's millisecond clock, so animations are time-based and
correct even when ticks arrive unevenly. The frame loop in `Port::run()` blocks on the
message pump until the next `EVENT_GUI_TICK`, delivers queued service messages, posts
button codes, then calls `lv_timer_handler()` once. LVGL therefore renders at most one
frame per kernel tick, which is the watch's 10 Hz.

One configuration choice matters here. LVGL's refresh and animation timers only run
when at least their period has elapsed, and the default period is 100 ms. With the
kernel tick also at 100 ms, any tick that lands a millisecond early was skipped and the
frame waited for the next one, halving the frame rate during animations. `lv_conf.h`
sets `LV_DEF_REFR_PERIOD` to 1 so that every tick renders whatever is invalid; the tick
still paces the loop, so this adds no work.

### Buttons

Each `SDK::GUI::Button` code the kernel delivers (click `'1'`..`'4'`, press
`'q'`..`'r'`, release `'a'`..`'f'`) is posted to the active LVGL screen as an
`LV_EVENT_KEY` event. No LVGL input device or group is involved: the focus-navigation
model of an LVGL keypad group does not fit four buttons whose meaning each screen
defines, so screens read the code with `lv_event_get_key()` and decide themselves.

### Lifecycle

The port registers itself as the pump's `IGuiLifeCycleCallback` and forwards
start/stop/resume/suspend/frame to the app's `Model`. On resume it invalidates the
active screen so the kernel receives a full frame, since another GUI may have owned the
display meanwhile. On stop it calls `lv_deinit()`.

### Memory

LVGL uses its built-in allocator on a static pool, `LV_MEM_SIZE`, sized by measurement:
`ScreenManager` logs the pool's use and peak on every screen switch, and RunLVGL's peak
over a full session was about 25 KB, so the pool is 40 KB. No stock theme is compiled;
the app styles its widgets itself, which is smaller and closer to the design.

## The RunLVGL GUI

```
Software/Apps/LVGL-GUI/
  lvgl-gui.cmake              sources and include dirs (globbed)
  assets/gen_assets.py        font and image converter (see Assets)
  assets/fonts/*.c            15 Poppins faces, 2 bpp
  assets/images/*.c           15 icons
  gui/include/gui/
    Assets.hpp                LV_FONT_DECLARE / LV_IMAGE_DECLARE for the above
    Format.hpp                integer-only number formatting (pace, distance, time)
    Strings.hpp               text constants
    model/                    Model, ModelListener, menu navigation state
    screens/                  Screen base, ScreenManager, one class per screen
    theme/Theme.hpp           palette, fonts, drawing helpers
    widgets/                  Buttons, Title, Battery, HeartRateZone, WheelMenu, ...
  gui/src/
    GuiApp.cpp                una_lvgl_app_init(): model + first screen
    ...                       implementations of the above
  simulator/                  PC simulator project (CMake) and its sensor config
```

### Entry Point

The SDK's LVGL `main.cpp` binds the kernel, initialises the port, then calls one hook
the app defines:

```cpp
extern "C" void una_lvgl_app_init(void)
{
    Theme::init();
    sModel = new (sModelStorage) Model();
    ScreenManager::instance().start(*sModel, ScreenId::Main);
}
```

Objects are constructed here rather than as globals so nothing touches the kernel
before `main()` has bound it. The same file defines `una_lvgl_default_font()`, which
`lv_conf.h` routes `LV_FONT_DEFAULT` through; returning one of the app's own faces keeps
LVGL's built-in Montserrat fonts out of the link.

### Model and the Service Contract

`Model` is the GUI's single point of contact with the service and the kernel. It
implements `IGuiLifeCycleCallback` (the port's lifecycle events) and
`ICustomMessageHandler` (the service's messages), keeps the last known state (time,
battery, GPS fix, settings, track data, activity summary) and exposes commands
(`trackStart`, `trackPause`, `saveLap`, `saveSettings`, `exitApp`, ...) that send the
matching messages to the service.

The message types in `Software/Libs/Header/Commands.hpp` are unchanged from Run:
`SettingsUpd`, `Time`, `Battery`, `GpsFix`, `TrackStateUpd`, `TrackDataUpd`,
`LapEnded`, `Summary`, `IntervalsPhaseAlert`, `IntervalsWorkoutCompleted` and
`AccessoryStatusUpd` flow from service to GUI; `SettingsSave`, `TrackStart`,
`TrackStop`, `TrackPause`, `TrackResume`, `ManualLap` and `IntervalsNextPhase` flow
back. This is the point of the exercise: the service does not know or care which toolkit
renders its state, and the whole GUI could be swapped again without touching it.

Exactly one screen is bound to the model at a time (`Model::bind`). The model
forwards each incoming message to the bound screen through `ModelListener`, a set of
virtual callbacks with empty defaults (`onGpsFix`, `onBatteryLevel`, `onTrackData`,
`onLapChanged`, `onIntervalsPhaseAlert`, ...), so a screen overrides only what it shows.
State updates are always applied to the model first, whether or not a screen is bound,
so a screen created later reads current values.

### Screens and Navigation

`Screen` owns one LVGL screen object. `create()` makes the object, styles it as the
black background and calls the subclass's `build()`, which populates it with widgets;
`destroy()` deletes the object and everything on it. Key events reach `onKey(code)`;
`onShow()` and `onHide()` bracket the time a screen is active and are where it restores
and stores navigation state (which menu item was selected, which face was showing).

`ScreenManager::goTo(ScreenId)` switches screens. The switch is deferred with
`lv_async_call` so it can be requested from inside an event or a message handler, and
runs as: hide and unbind the old screen, create and load the new one, destroy the old
one, bind and show the new one. Screens are therefore built on entry and destroyed on
exit, which is the same policy as the TouchGFX MVP application and keeps only one
screen's widgets in memory. The `ScreenId` enum in `ScreenManager.hpp` lists every
screen, and `ScreenManager::create()` is the one place that maps ids to classes.

`MainScreen` is the smallest complete example. Its `build()` creates a `WheelMenu` with
the three items (Start, Intervals, Settings), the button hints, the title and the sensor
status row; `onShow()` restores the remembered menu position and asks the model for the
current GPS and accessory state; `onKey()` maps L1/L2 to the wheel, R1 to `confirm()`
and R2 to `exitApp()`; and `onGpsFix()` recolours the lens and shows or hides the R1
hint, because Start is greyed out without a fix. Compare it with Run's `MainView` and
`MainPresenter` to see the same behaviour expressed in the two toolkits.

### Theme

`Theme` holds what TouchGFX Designer would have generated: the palette (the SDK's
64-colour `SDK::GUI::Color` values, which the two-bits-per-channel display renders
exactly), the fifteen Poppins faces by weight and size, and small drawing helpers:
`label`, `hline`, `vline`, `image`, `imageTinted`, `dot`, `arc`, `container`. The
helpers take TouchGFX coordinates and angles (arcs measured from 12 o'clock, clockwise;
`arcAngle()` converts to LVGL's 3 o'clock origin), so screens can be laid out straight
from the Run app's Designer values and the two apps match to the pixel.

### Widgets

`Widgets.hpp` collects the composite widgets Run had as TouchGFX custom containers:
`Buttons`, `Title`, `Battery`, `ScrollIndicator`, `SensorStatusRow`, `HeartRateZone`,
`PauseIndicator`, `InfoCarousel`, `TimerRing`, `Map`, `Toggle`, `IntervalsTimer` and
`TwoTonePicker`. Each is a small class that creates LVGL objects on a parent and keeps
the handles it needs to update. Two are worth reading for technique:

- **`WheelMenu`** reproduces the scroll wheel's 400 ms slide the way TouchGFX's
  `ScrollWheelWithSelectionStyle` does: two strips of three slots, one in the large
  selected style clipped to the selection window and one in the small style clipped to
  the area below, moved together by one item pitch with `lv_anim`. It reports its slide
  midpoint so the screen can recolour the lens when the incoming item takes the centre,
  as Run does.
- **`HeartRateZone`** draws the five-segment zone bar and the active-zone marker with
  `lv_arc` objects and one triangle drawn in an `LV_EVENT_DRAW_MAIN` handler, with
  geometry fitted from Run's bitmaps. It replaced about 50 KB of images with about 100
  lines of code and no bitmaps, which is the general lesson for LVGL on this platform:
  shapes are cheap, pixels are not.

### Assets

`assets/gen_assets.py` converts Run's Poppins TTFs and PNG icons into LVGL C arrays, and
the outputs are committed:

- Fonts through `lv_font_conv` (run via `npx`, so Node is needed) at 2 bits per pixel,
  uncompressed. Faces used only for numbers carry digits and punctuation only, which
  keeps all fifteen faces to about 80 KB; the one exception is documented in the script
  (the selected Start item uses a face that must stay full ASCII).
- Images through LVGL's own `LVGLImage.py` (needs the `pypng` and `lz4` Python packages).
  Multi-colour icons are `RGB565A8`; single-colour icons (ticks, crosses, pause, sensor
  glyphs) are `A8` alpha masks tinted at draw time with `Theme::imageTinted()`, so one
  bitmap serves every colour variant. Do not use indexed formats to save space: LVGL v9
  decodes them to 32-bit ARGB in the pool at draw time.

The generator runs the converters with SDK-relative paths and normalises the output to
LF, so a regeneration on Windows or Linux reproduces the committed files byte for byte.
Declarations in `Assets.hpp` sit inside `extern "C"`, which MSVC needs to link C-defined
symbols from C++.

## Memory and Performance

The GUI process is loaded whole into SRAM, so its size is the number that matters:

| | RunLVGL | Run (TouchGFX) |
|---|---|---|
| `.uapp` file | ~408 KB | ~529 KB |
| GUI process RAM as loaded | ~452 KB | ~493 KB |
| of which code and assets | ~308 KB | |
| of which static data | ~139 KB: 40 KB LVGL pool, 58 KB frame, 14 KB stripe, the rest LVGL and app state | |

The kernel needs one contiguous block for the whole process, so watch the headroom in
the kernel's `System Heap` log lines when other apps are resident. `GUI_RAM_LENGTH` and
`GUI_STACK_SIZE` in the app's CMake set the region and the GUI task's stack; LVGL's
software renderer recurses deeper than TouchGFX and needs the 24 KB stack RunLVGL gives
it.

Rendering a full-screen animation frame takes about 40 ms of the 100 ms period on the
watch's Cortex-M33. Configuring the app with `-DRUNLVGL_FRAME_STATS=ON` logs one line per
100 kernel ticks with tick spacing, frames sent, render time and hand-off time, which is
how the numbers in this section were measured.

Pitfalls met while writing this app, so you do not have to:

- `LV_IMAGE_DECLARE()` has no trailing semicolon in its expansion while
  `LV_FONT_DECLARE()` has; a missing `;` produces a cascade of unrelated errors.
- `lv_obj_remove_flag(obj, A | B)` needs `static_cast<lv_obj_flag_t>` in C++.
- `lv_point_precise_t` is integer unless `LV_USE_FLOAT` is on; round before assigning.
- `LV_FONT_DEFAULT` must be a font you ship; the `una_lvgl_default_font()` hook exists so
  Montserrat can be left out of the link.
- Weak symbols do not exist on MSVC; the port uses `/alternatename` for the same effect
  in the simulator build.

## What Differs from the TouchGFX Run App

| | Run (TouchGFX) | RunLVGL |
|---|---|---|
| Layout | TouchGFX Designer, generated base classes | Code, using the same coordinates |
| Screens | View + Presenter per screen, generated `ViewBase` | One `Screen` subclass per screen |
| Text and fonts | Designer text database, generated typed texts | Plain strings, fonts converted once |
| Animations | TouchGFX animated widgets and `handleTickEvent` | `lv_anim` and time-based LVGL timers |
| Simulator | Visual Studio project generated by Designer, Windows only for the IDE flow | One CMake project, Windows and Linux |
| Service | Identical | Identical |

The service, the FIT files, the settings file and the message contract are the same, so
an activity recorded with RunLVGL is indistinguishable from one recorded with Run.

## Where to Go Next

To start your own LVGL app, copy `RunLVGL-CMake/CMakeLists.txt` and the `GuiApp.cpp`,
`Screen`, `ScreenManager`, `Theme` and `Model` skeletons, keep the port as it is, and
write your screens. The simulator project needs only the app's source lists and its
`ConfigurationSimulator.hpp`. For the kernel-side details of what a GUI process may do,
see the [TouchGFX Port Architecture](../../TouchGFX-Port-Architecture.md), most of which
describes the shared message pump and applies to LVGL unchanged, and
[Simulator](../../Simulator.md) for the mock kernel the simulator runs on.
