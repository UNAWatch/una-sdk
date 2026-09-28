(tutorials/helloworld/architecture)=
# HelloWorld - Introduction to UNA App Architecture

Welcome to the UNA SDK tutorial series! HelloWorld is your first step in learning to build applications for the UNA Watch platform. This tutorial focuses on the fundamental app architecture - how service and GUI components communicate - without the complexity of sensors or data logging.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/HelloWorld)

HelloWorld comes with two GUIs that show the same screen: one built with **TouchGFX**, one with **LVGL**. The service, the message contract and the build system are the same for both; only the GUI process differs. Pick the toolkit you prefer and follow that column through the series. The two are shown side by side so you can also compare them.

## What You'll Learn

- How to set up your development environment for UNA apps
- The basic structure of a UNA Watch application
- How service and GUI layers communicate
- How to build and run apps on simulator and hardware
- How the same GUI is expressed in TouchGFX and in LVGL
- Understanding the UNA app framework fundamentals

## Getting Started

### Prerequisites
Before building HelloWorld, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

- UNA SDK cloned (`git clone https://github.com/UNAWatch/una-sdk.git`)
- ST ARM GCC Toolchain (from STM32CubeIDE/CubeCLT, not system GCC)
- CMake 3.21+ and make
- Python 3 with pip packages installed

**Minimum requirements for HelloWorld:**
- `UNA_SDK` environment variable pointing to SDK root
- ARM GCC toolchain in PATH
- CMake and build tools

**For the TouchGFX GUI:**
- TouchGFX Designer installed (see [toolchain setup](toolchain-setup)) if you want to modify the design or run the TouchGFX simulator

**For the LVGL GUI:**
- The LVGL submodule checked out once: `git submodule update --init ThirdParty/lvgl` (from the SDK root)
- Node.js and Python, only if you regenerate the converted fonts (see [Assets](#lvgl-assets))

### Building and Running HelloWorld

1. **Verify your environment setup** (see [toolchain setup](toolchain-setup) for details):
   ```bash
   echo $UNA_SDK                   # Should point to SDK root. 
                                   # Note for backward compatibility with linux path notation it uses '/'
   
   which arm-none-eabi-gcc         # Should find ST toolchain
   which cmake                     # Should find CMake
   ```

2. **Navigate to the HelloWorld directory:**
   ```bash
   cd $UNA_SDK/Docs/Tutorials/HelloWorld
   ```

3. **Build the application** with the GUI of your choice. Each is its own CMake project and produces its own `.uapp`:
   ```bash
   # TouchGFX GUI
   mkdir build && cd build
   cmake -G "Unix Makefiles" ../Software/Apps/HelloWorld-CMake
   make

   # LVGL GUI (from the tutorial directory again)
   cd .. && mkdir build-lvgl && cd build-lvgl
   cmake -G "Unix Makefiles" ../Software/Apps/HelloWorldLVGL-CMake
   make
   ```

The app will start and show a basic GUI demonstrating the UNA app framework. This HelloWorld focuses on the core architecture - the service-GUI communication pattern that all UNA apps use. The two builds install side by side on the watch (`D:\Apps\HelloWorld\` and `D:\Apps\HelloWorldLVGL\`) and appear in the launcher as **HelloWorld** and **HelloWorldLVGL**.

### Running on Simulator

**TouchGFX** (Windows only):

1. Open `HelloWorld.touchgfx` in TouchGFX Designer and click **Generate Code (F4)** (do this once).
2. Navigate to `HelloWorld\Software\Apps\TouchGFX-GUI\simulator\msvs`
3. Open `Application.vcxproj` in Visual Studio
4. Press **F5** to start debugging and run the simulator

**LVGL** (Windows and Linux): the simulator is a plain CMake project in `Software/Apps/LVGL-GUI/simulator`. It runs the real service and GUI processes against the SDK's mock kernel, with the display in an SDL2 window. It needs a host C++ compiler, CMake, a generator (Ninja, or Visual Studio on Windows) and SDL2, and no IDE: on Linux or WSL, GCC, Ninja and the SDL2 development package; on Windows, MSVC, which the free *Build Tools for Visual Studio* provide as well as the Visual Studio IDE (the "Desktop development with C++" workload's "C++ CMake tools for Windows" component brings CMake and Ninja). The build's architecture must match the SDL2 it links; it is chosen by the developer prompt with the Ninja generator (x86 or x64) and by `-A` with a Visual Studio generator (`-A Win32` or `-A x64`; Ninja does not take `-A`). Without an installed SDL2 the Windows build reuses the 32-bit copy TouchGFX Designer ships and must therefore be 32-bit, as in the commands below; with an installed 64-bit SDL2 pick the 64-bit option instead.

```bat
:: Windows, in an "x86 Native Tools Command Prompt for VS" (a cmd window with
:: the 32-bit MSVC environment loaded; any shell after vcvarsall.bat x86 is the same)
cd /d %UNA_SDK%\Docs\Tutorials\HelloWorld\Software\Apps\LVGL-GUI\simulator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
build\bin\HelloWorldLVGLSimulator.exe
```

```powershell
# Windows, any shell: the Visual Studio generator finds MSVC itself and also
# writes a solution ("Visual Studio 18 2026" needs CMake 4.2 or newer,
# "Visual Studio 17 2022" works with the 3.21 minimum)
cd $env:UNA_SDK\Docs\Tutorials\HelloWorld\Software\Apps\LVGL-GUI\simulator
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Debug
.\build\bin\HelloWorldLVGLSimulator.exe
```

```bash
# Linux, or WSL (Debian/Ubuntu: sudo apt-get install build-essential cmake ninja-build libsdl2-dev)
cd $UNA_SDK/Docs/Tutorials/HelloWorld/Software/Apps/LVGL-GUI/simulator
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
(cd build/bin && ./HelloWorldLVGLSimulator)
```

In either simulator the keyboard keys **1**, **2**, **3** and **4** are the watch buttons L1, L2, R1 and R2. HelloWorld only reacts to R2, which exits the app. Since HelloWorld has minimal interactive elements, it primarily demonstrates the app startup and basic framework.

For detailed simulator setup and features, see [Simulator](../../Simulator.md).

### Working with the TouchGFX GUI (Optional)

If you want to explore or modify the GUI design:

1. **Install TouchGFX Designer** (see [toolchain setup](../../sdk-setup.md) for installation)

2. **Open the TouchGFX project:**
   ```
   HelloWorld.touchgfx
   ```

3. **Make design changes** in TouchGFX Designer (add/modify screens, widgets, interactions)

4. **Generate code** after making changes:
   - Click "Generate Code" button in TouchGFX Designer, OR

5. **Rebuild the app** to include your GUI changes:
   ```bash
   cmake -G "Unix Makefiles" /path/to/HelloWorld-CMake # If artifacts has been changed
   make
   ```

### Working with the LVGL GUI (Optional)

There is no designer: the screen is code. `gui/src/screens/MainScreen.cpp` creates the label and the button hint with the SDK's drawing helpers (`SDK/GUI/LVGL/Draw.hpp`), which take the same coordinates a TouchGFX Designer shows, so a layout can be copied across. Edit it, rebuild, run. Adding a font or an icon means adding it to `assets/assets.json` and regenerating (see [Assets](#lvgl-assets)).

## HelloWorld App Overview

HelloWorld demonstrates the essential UNA app architecture:

### The Service Layer (Backend)
- Runs as the main application thread
- Handles sensor connections and data processing
- Manages app lifecycle (start/stop)
- Communicates with the GUI through messages

The service is one copy of the code, `Software/Libs`, shared by both GUI builds: `HelloWorld-CMake` and `HelloWorldLVGL-CMake` each link it with their GUI. Compare the two `CMakeLists.txt`: the service half is identical, and the GUI half differs in three lines (`TOUCHGFX_PATH` becomes `GUI_PATH`, and the `UNA_SDK_*_GUI` variables become `UNA_SDK_*_GUI_LVGL`).

### The GUI Layer (Frontend)
- Built with TouchGFX or LVGL. For the TouchGFX port, see [TouchGFX Port Architecture](../../TouchGFX-Port-Architecture.md); for the LVGL port, see the [RunLVGL tutorial](../RunLVGL/ARCHITECTURE.md)
- Displays information to the user
- Receives updates from the service
- Handles user interactions

### Communication Between Layers
- Uses the UNA kernel messaging system
- Service sends data to GUI via custom messages
- GUI can send commands back to service

The kernel does not know which toolkit a GUI process uses. It hands the process a 240 x 240 frame buffer, button codes and a 10 Hz tick, and each toolkit's port in the SDK turns those into its own rendering and events.

## Project Layout

```
HelloWorld/
  ARCHITECTURE.md                       this page
  Resources/                            launcher icons (shared)
  Output/, OutputLVGL/                  where each build puts its .uapp (and, for apps that
                                        have one, its app-manifest.json)
  Software/
    Libs/                               the service (shared)
      Header/Service.hpp
      Sources/Service.cpp
      libs.cmake
    Apps/
      HelloWorld-CMake/CMakeLists.txt       .uapp with the TouchGFX GUI
      HelloWorldLVGL-CMake/CMakeLists.txt   .uapp with the LVGL GUI
      TouchGFX-GUI/                         TouchGFX GUI process
        HelloWorld.touchgfx                 the Designer project
        gui/include/gui/, gui/src/          model, view, presenter, containers
        generated/                          Designer output (do not edit)
        simulator/msvs, simulator/gcc       Visual Studio and gcc simulator projects
      LVGL-GUI/                             LVGL GUI process
        lvgl-gui.cmake                      sources and include dirs
        assets/assets.json                  fonts and images to convert
        assets/fonts/*.c                    converted font (committed)
        gui/include/gui/, gui/src/          model, screen, entry point
        simulator/CMakeLists.txt, main.cpp  CMake simulator project
```

## The Same Screen in Two Toolkits

The screen is a "Hello World" label with a hint arc beside the R2 button, and R2 exits. Here is how each toolkit expresses the pieces.

| Piece | TouchGFX (`TouchGFX-GUI/gui`) | LVGL (`LVGL-GUI/gui`) |
|---|---|---|
| Owner of the model and screen | `FrontendHeap` (generated singleton) | `GuiApp.cpp`: `una_lvgl_app_init()` constructs `Model` and `MainScreen`, then `lv_screen_load()` |
| Model | `Model` registers with `TouchGFXCommandProcessor` and implements `IGuiLifeCycleCallback`; also a `UIEventListener` | `Model` registers with `SDK::LVGL::Port` and implements `IGuiLifeCycleCallback` |
| Screen | `MainView` (widgets, key handling) + `MainPresenter` (talks to the model) | `MainScreen`: one class owning an LVGL screen object |
| The label | `TextArea` placed in Designer, text from `texts.xml`, font from the typography table | `Draw::label(root, &poppins_regular_18, "Hello World", 19, 98, 203)` |
| The button hint | `ButtonsSet` custom container with twelve bitmaps, `setR2(WHITE)` | `SDK::LVGL::Buttons`, `set(NONE, NONE, NONE, WHITE)`: arcs drawn by LVGL, no bitmaps |
| Button input | `MainView::handleKeyEvent(uint8_t key)` | `LV_EVENT_KEY` on the screen object; `MainScreen::onKey(uint8_t code)` |
| Exit | `presenter->exit()` -> `model->exitApp()` -> `mKernel.sys.exit()` | `mModel.exitApp()` -> `mKernel.sys.exit()` |
| Fonts | Designer converts the TTF at generate time | `assets/assets.json` + `lvgl_assets.py` convert it to a C file, committed |

What is the same: the `Model` class and its lifecycle callbacks, the button codes (`SDK::GUI::Button`), the colours (`SDK::GUI::Color`), the service, and the kernel messages.

### Entry point

TouchGFX: `FrontendHeap::getInstance()` is created by the SDK's TouchGFX entry point; its constructor builds the model and shows the start screen chosen in Designer.

LVGL: the SDK's entry point (`Libs/Source/AppSystem/EntryPoint/LVGL/main.cpp`) initialises the port and calls the app's `una_lvgl_app_init()`. HelloWorld's version, in `GuiApp.cpp`, builds the shared styles, the model and the screen, and loads the screen. It also defines `una_lvgl_default_font()`, which names the font LVGL falls back to, so LVGL's built-in font stays out of the binary.

### Buttons

The kernel delivers each button as a click code (`'1'`..`'4'`) plus a press code when it goes down and a release code when it comes up (`SDK/GUI/Button.hpp`). TouchGFX passes them to the view's `handleKeyEvent()`. The LVGL port posts them as `LV_EVENT_KEY` events to the active screen; `MainScreen` registers one event callback on its screen object and switches on the code. HelloWorld acts on the R2 click only.

### The LVGL screen in full

```cpp
MainScreen::MainScreen(Model& model) : mModel(model)
{
    mRoot = lv_obj_create(nullptr);                 // a screen object
    Draw::applyScreen(mRoot);                       // black, unscrollable
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    Draw::label(mRoot, &poppins_regular_18, "Hello World", 19, 98, 203,
                LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::WHITE);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::WHITE);
}
```

Every object is created on `mRoot`, and LVGL deletes them with it. The `Draw` helpers and `Buttons` are part of the SDK (`Libs/Header/SDK/GUI/LVGL/`) and are what the activity apps use, so a tutorial screen looks like a shipped one.

### LVGL assets

LVGL needs fonts as C arrays. `LVGL-GUI/assets/assets.json` lists them, here one entry: Poppins Regular at 18 px, printable ASCII, 2 bits per pixel, taken from the TTF the TouchGFX GUI already ships in `TouchGFX-GUI/assets/fonts`. The converted file is committed, so building needs no converter. To add a font or an icon, add an entry and run:

```bash
python $UNA_SDK/Utilities/Scripts/lvgl_assets/lvgl_assets.py Software/Apps/LVGL-GUI/assets/assets.json
```

Fonts go through `lv_font_conv` (run with `npx`, so Node.js is needed); images through LVGL's own `LVGLImage.py` (needs the `pypng` and `lz4` Python packages). Declare each generated symbol in `gui/include/gui/Assets.hpp`.

### Size on the watch

| Build | `.uapp` | GUI code (text) | GUI RAM (bss) |
|---|---|---|---|
| HelloWorld (TouchGFX) | 228 KB | 214 KB | 71 KB |
| HelloWorldLVGL | 167 KB | 158 KB | 139 KB |

LVGL is linked from source, so the GUI carries only the parts it uses; its RAM is dominated by the frame buffer and the 40 KB object pool set in the SDK's `lv_conf.h`. The TouchGFX library is prebuilt, and its RAM is the frame buffer plus the Designer-generated heap.

## Understanding the Commented Code

HelloWorld includes commented-out implementations of common UNA app features. These serve as reference examples for future tutorials:

- **Heart Rate Sensor Integration**: Complete sensor connection, data parsing, and real-time GUI updates
- **FIT File Logging**: Activity data recording with session summaries
- **Custom Messaging**: Service-to-GUI communication patterns

**Note**: The next tutorial will walk through enabling heart rate monitoring step-by-step. For now, focus on understanding the basic app structure and messaging framework.

## Understanding UNA App Communication

HelloWorld demonstrates the two main ways UNA apps communicate between service and GUI:

### SDK Custom Messages (Service → GUI)
Used for real-time data updates. The service creates messages using `SDK::make_msg<>()` and sends them via the kernel. The GUI receives them in `Model::customMessageHandler()`.

### AppTypes Events (GUI → Service)
Used for commands and configuration. The GUI sends events through the `IGuiBackend` interface, which the service implements to receive commands.

These patterns form the foundation of all UNA app communication, and they are the same whichever toolkit draws the screen. Future tutorials will show how to implement specific features using these systems.

## Common Patterns and Best Practices

### Sensor Integration
- Always check `matchesDriver(handle)` before processing sensor data
- Validate data with `isDataValid()` before using
- Handle sensor timeouts and disconnections gracefully

### Message Design
- Use unique message type IDs (increment from 0x00000001)
- Keep messages small and focused on single purposes
- Use descriptive names for message types and fields

### GUI Updates
- Only update GUI when necessary to avoid performance issues
- Use appropriate data types (float for measurements, int for counts)
- Handle invalid/missing data gracefully

### File Operations
- Use the kernel's filesystem interface (`mKernel.fs`)
- Handle file I/O errors appropriately
- FIT files are Garmin's standard format for activity data

## Next Steps

1. **Get HelloWorld running** - Follow the build steps above and confirm the app launches
2. **Explore the code structure** - Look at Service.cpp and Model.cpp to understand the messaging flow
3. **Check the commented examples** - Review the commented HR and FIT code to see what's available
4. **Continue to the next tutorial** - Learn how to enable heart rate monitoring and data logging
5. **Study other example apps** - Look at Alarm or Cycling apps for different patterns, or [RunLVGL](../RunLVGL/ARCHITECTURE.md) for a complete activity app with an LVGL GUI

## Troubleshooting

### Build Issues
- Ensure all SDK paths are correctly configured
- Check that TouchGFX is properly installed
- For the LVGL build, check that `ThirdParty/lvgl` is populated (`git submodule update --init ThirdParty/lvgl`)
- Verify CMake finds all required dependencies

### Runtime Issues
- Check log output for error messages
- Verify sensor connections on real hardware
- Use the simulator for initial testing

### Common Mistakes
- Forgetting to uncomment all related code sections
- Using duplicate message type IDs
- Not handling message memory management properly

Remember: Every complex app started as a simple HelloWorld. Take it step by step, and you'll be building amazing UNA Watch applications in no time!
