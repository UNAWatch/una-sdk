---
name: una-new-app
description: Start a new UNA Watch app (activity, utility, glance or clockface) by copying the closest SDK example and renaming it. Covers choosing between a TouchGFX and an LVGL GUI, a fresh APP_ID, the paths to fix when the app lives outside the SDK, regenerating TouchGFX code, and converting LVGL fonts and images. Use when creating a UNA Watch app or watch face, when deciding which GUI toolkit to use, or when regenerating TouchGFX code or adding fonts or images to an app.
---

# Start a new UNA Watch app

New apps start as a copy of an SDK example. There is no generator. Copy the example closest to
what the user wants, rename it and fix its paths, then build it unchanged before changing any
behaviour.

Needs a working setup (skill `una-setup`): `UNA_SDK` set and `arm-none-eabi-gcc` on `PATH`.

## 1. Decide the app type

| Type | What it is | `APP_TYPE` |
|---|---|---|
| Activity | Full-screen app that records or tracks (run, ride, workout) | `Activity` |
| Utility | Full-screen tool (timer, alarm, navigation) | `Utility` |
| Glance | A 240 × 60 card in the glance list, drawn by the service: no GUI process | `Glance` |
| Clockface | A watch face | `Clockface` |

## 2. Decide the GUI toolkit

Glances have no GUI process. For the other types, ask the user which toolkit they want, with
these facts. Don't decide for them:

| | TouchGFX | LVGL |
|---|---|---|
| How screens are made | TouchGFX Designer (visual editor), which generates C++, plus hand-written code in `gui/` | All in code: C/C++ against LVGL |
| Host for editing screens | Windows only (Designer, and its `tgfx` command-line generator) | Any |
| ST software needed | TouchGFX Designer, free with an ST account | None with the Arm GNU Toolchain |
| Simulator | Visual Studio project (Windows) or a Makefile (Linux x86-64) | Plain CMake + SDL2: Windows, Linux, macOS |
| Examples | Every app in `Examples/Apps` except `RunLVGL` and the glances, and every tutorial with a GUI | `RunLVGL` and the `LVGL-GUI` variant of most tutorials |

Both run on the same kernel. The kernel only receives finished frames, so the choice doesn't
affect what the app can do with sensors, files or the service. Docs: the `RunLVGL` tutorial at
`Docs/Tutorials/RunLVGL/ARCHITECTURE.md`, and `Docs/TouchGFX-Port-Architecture.md`.

## 3. Pick the example to copy

| The user wants | Copy | Toolkits |
|---|---|---|
| Minimal app with one screen | `Docs/Tutorials/HelloWorld` | both |
| Buttons and navigation | `Docs/Tutorials/Buttons`, `Docs/Tutorials/ScrollMenu` | both |
| Sensors (HR, IMU, GPS…) | `Docs/Tutorials/Sensors` | both |
| Reading or writing files | `Docs/Tutorials/Files` | both |
| Images | `Docs/Tutorials/Images` | both |
| GPS navigation utility | `Docs/Tutorials/Waypoint` | both |
| Recording FIT activity files | `Docs/Tutorials/FitFiles` (service only), then `Examples/Apps/Running` | — / TouchGFX |
| A GPS activity (run, ride, hike) | `Examples/Apps/RunLVGL` (LVGL) or `Examples/Apps/Running` (TouchGFX) | either |
| Timer, stopwatch, alarm | `Examples/Apps/Timer`, `Stopwatch`, `Alarm` | TouchGFX |
| Watch face | `Examples/Apps/ClockfaceConsole` (digital), `ClockfaceAnalogue`; read `Docs/writing-a-clockface.md` first | TouchGFX |
| Glance | `Examples/Apps/GlanceSteps` (it has a simulator) | — |

Read the copied example's `ARCHITECTURE.md` (tutorials) before changing code.

## 4. Copy and rename

Copy the whole example folder to where the user wants the app, usually its own git repository.
For a TouchGFX app, put it beside the SDK clone, with the clone's folder named `una-sdk` (see
step 3):

```text
projects/
├── una-sdk/   the SDK clone
└── MyApp/     the new app
```

The app's layout (`MyApp` is the new name):

```text
MyApp/
├── Resources/             icon_30x30.png, icon_60x60.png (launcher icons; not used by glances)
├── Output/                where the build puts the .uapp
└── Software/
    ├── Libs/              the service: Header/, Sources/ (Source/ in glances and FitFiles), libs.cmake
    └── Apps/              (glances and FitFiles: App/)
        ├── <Name>-CMake/  CMakeLists.txt: builds service + GUI + .uapp
        ├── TouchGFX-GUI/  or LVGL-GUI/ (each has its own simulator/)
        └── ...
```

A tutorial with both GUIs has two CMake projects: `<Name>-CMake` for TouchGFX and
`<Name>LVGL-CMake` for LVGL. Keep the chosen one with its GUI folder, and delete the other GUI,
its CMake folder and its output folder.

The tutorials' LVGL `assets/assets.json` takes its fonts and images from the TouchGFX GUI
(`"file": "../../TouchGFX-GUI/assets/..."`). Before deleting `TouchGFX-GUI`, move each file the
manifest names into `LVGL-GUI/assets/` and change its `"file"` path to match.

Then:

1. **`CMakeLists.txt` in the `-CMake` folder.** Rename the folder to `MyApp-CMake`, then set:
   - `APP_NAME "MyApp"`: names the ELFs and targets. No other file has to match it.
   - `APP_ID`: new, 16 uppercase hex digits. Never reuse the example's ID: the store and the phone
     app identify an app by it. For a store app, use the ID the store issues (skill
     `una-store-submit`). Otherwise:
     `python3 -c "import secrets; print(secrets.token_hex(8).upper())"`.
   - `APP_USER_NAME`: the name shown on the watch, at most 15 bytes (longer names are cut). It
     defaults to `APP_NAME`.
   - `DEV_ID`: the developer's own short name. The examples use `"UNA"`.
   - Leave the `RESOURCES_PATH`, `OUTPUT_PATH`, `LIBS_PATH` and `TOUCHGFX_PATH`/`GUI_PATH`
     values as they are, unless you moved folders. They are relative to `CMakeLists.txt`.
2. **Simulator identity.** Rename the simulator's target and executable:
   - **LVGL:** `LVGL-GUI/simulator/CMakeLists.txt` names its project and executable, and repeats
     `APP_ID` in `target_compile_definitions`. Update all three. It also repeats settings from the
     `-CMake` project's GUI build, such as `LV_MEM_SIZE`: change both together.
   - **Glance:** `Software/simulator/CMakeLists.txt` passes `APP_ID` to
     `una_add_glance_simulator`.
3. **TouchGFX only: the framework path.** Four files reach the TouchGFX framework in the SDK by a
   path relative to the example's place in the SDK. The simulator and code generation use these
   paths; the `.uapp` build doesn't, because it finds TouchGFX through `UNA_SDK`. With the app
   beside `una-sdk` (above), set them to:

   | File in `TouchGFX-GUI/` | Key | Value |
   |---|---|---|
   | `<Name>GUI.touchgfx` | `"TouchGfxPath"` | `../../../../una-sdk/ThirdParty/touchgfx` |
   | `target.config` | `"touchgfx_path"` | `../../../../una-sdk/ThirdParty/touchgfx` |
   | `config/gcc/app.mk` | `touchgfx_path :=` | `../../../../una-sdk/ThirdParty/touchgfx` |
   | `config/msvs/Application.props` | `<TouchGFXReleasePath>` | `..\..\..\..\..\..\una-sdk\ThirdParty\touchgfx` |

   The first three are relative to `TouchGFX-GUI/`. `Application.props` is relative to
   `simulator/msvs/`, two folders deeper. These paths hold nothing personal, so commit them; any
   developer who clones the app beside `una-sdk` can build it.

   If the SDK is somewhere else, use the absolute path of `$UNA_SDK/ThirdParty/touchgfx` in all
   four instead. Tell the user that this path belongs to their machine, may include their user
   name, and goes into the repository when the files are committed. The `.touchgfx` file is the
   whole screen design, so it must be committed either way.

   On Windows, Visual Studio joins the project folder and the relative path before shortening
   it, so in a deeply nested folder the simulator build can pass the 260-character path limit and
   fail with "Cannot open source file". Move the projects folder somewhere shorter, or use the
   absolute path.

   The app can also be renamed in the `.touchgfx` file (`"Name"`), and the file renamed to match.
4. **Check that nothing still points at the old app.** Search the copy for the example's name and
   old `APP_ID`, outside `generated/` and binary assets. Rename what belongs to the app: a
   service class or namespace named after the example can stay.
5. **Ignore build output.** The examples' own `.gitignore` files cover the CMake project's
   `build/`, the GUI's `build/` and `Output/`. Add a `.gitignore` at the app root for the
   folders the other skills create there:

   ```text
   /build/
   /build-tests/
   sim-out/
   ```
6. **Commit the starting point.** If the app is a git repository, commit before changing
   behaviour, so the user can see their changes against the unchanged example.

## 5. Build it unchanged

Build the `.uapp` (skill `una-build-uapp`) and run the simulator (skill `una-simulator`) before
editing code. A failure now is a copy or path problem, not the user's code.

## Working on the app

- **Service:** `Software/Libs/Sources/Service.cpp`. It connects sensors, reads and writes files,
  and sends messages to the GUI. Message types are app-defined structs in `Software/Libs/Header`.
  Guard each with `static_assert(sizeof(T) <= 256)`: the watch drops larger messages silently,
  and the simulator doesn't. The message header is 32 bytes on the watch and 40 in a 64-bit
  simulator or test build, so keep the payload within 216 bytes for the assert to hold in both.
- **TouchGFX GUI:**
  - edit screens in TouchGFX Designer, then regenerate (see below);
  - write code in `gui/src` and `gui/include` (views, presenters, model);
  - never edit `generated/`.
- **LVGL GUI:**
  - screens are code in `LVGL-GUI/gui/src/screens`;
  - in the tutorials, fonts and images are listed in `LVGL-GUI/assets/assets.json`, converted
    with `python $UNA_SDK/Utilities/Scripts/lvgl_assets/lvgl_assets.py <path>/assets.json`.
    `RunLVGL` converts its assets with its own `LVGL-GUI/assets/gen_assets.py` instead;
  - fonts need Node.js, and images need the `pypng` and `lz4` Python packages
    (`pip install pypng lz4`);
  - on Windows, `lvgl_assets.py` fails when the app and the SDK are on different drives;
  - declare each generated symbol in `gui/include/gui/Assets.hpp`.
- **Logic worth testing:** put it in plain classes in `Software/Libs` that don't depend on the
  GUI, so the `una-unit-test` skill can test it on the PC.

### Regenerating TouchGFX code (Windows)

After a change in Designer, or to the `.touchgfx` file, regenerate. Designer's **Generate Code**
(F4) does it, or headlessly from the `TouchGFX-GUI` folder:

```powershell
& "<touchgfx-install>\designer\tgfx.exe" generate -p MyAppGUI.touchgfx
```

`tgfx` is not on `PATH` after installation. Generation also rewrites `config/gcc/app.mk` and
`config/msvs/Application.props` with paths from this machine. Before committing, put back the
framework paths from step 3 in those two files, for example with
`git checkout -- config/gcc/app.mk config/msvs/Application.props` when they hold no other edits.

## Further reading in the SDK

- **Topic guides** in `Docs/`:
  - `service-lifecycle.md`: start, resume, suspend and stop;
  - `SensorsLayer.md`: sensors;
  - `shared-data.md`: data shared between apps;
  - `app-config-fields.md`: settings the phone collects;
  - `writing-a-clockface.md`: watch faces.
- **Interfaces:** `Libs/Header/SDK/Interfaces`.
