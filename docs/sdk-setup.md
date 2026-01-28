# SDK Setup and Build Overview

This guide covers SDK installation, manual environment setup, and primary workflows using CMake or STM32CubeIDE. The focus is on manual processes for better understanding and control, with the `una.py` tool limited to essential setup tasks (exporting `UNA_SDK` and basic environment configuration). For platform architecture, see [Platform Overview](platform-overview.md). For tutorials, see [Build Your First App](build-your-first-app.md).

## Prerequisites

- Linux/macOS/Windows development machine
- CMake 3.21+ (for manual builds)
- STM32CubeIDE (recommended for IDE-based development) or ARM GCC toolchain
- USB cable for device flashing
- Git for cloning the SDK

### Step 1: Clone the SDK
```bash
git clone --recursive https://github.com/UNAWatch/una-sdk.git
cd una-sdk
```

### Step 2: Choose Build Environment
- **CubeIDE**: Copy from `SDK/Examples/<app-name>` to `SDK/Examples/<your-app-name>` to avoid path dependencies issue out of the box.
- **CMake**:
  - Copy from `SDK/Examples/<app-name>` to whatever you want and adjust `CMakeLists.txt` accoringly.Make shure UNA_SDK environment variable is defined.
  - Set UNA_SDK for CMake:
      Manually add to your shell profile (e.g., `~/.bashrc` or `~/.zshrc`):
      ```bash
      export UNA_SDK=/path/to/una-sdk
      ```
      Reload your shell or run `source ~/.bashrc`.
  - CMake require the `UNA_SDK` environment variable to point to the SDK root directory. This enables location-independent app development.

## CMake-Based Workflow Development

The CMake workflow is recommended for command-line control and is fully standalone. Apps reference the SDK via `UNA_SDK` and use relative paths for flexibility.

### `Examples/` Project Structure Overview
```
MyApp/  # App root (can be anywhere)
├── Software/
│   ├── Apps/
│ |   │   ├── MyApp-Service-CubeIDE/ # CubeIDE project with Service
│ |   │   ├── MyApp-GUI-CubeIDE/ # CubeIDE project with GUI
│   │   └── MyApp-CMake/  # CMake project dir
│   │       ├── CMakeLists.txt
│   │       ├── MyAppService.ld  # Linker script
│   │       └── syscalls.cpp
│   │   └── TouchGFX-GUI/  # For GUI apps
│   └── Libs/
│       ├── Header/  # Headers
│       └── Sources/ # Sources
├── Resources/
│   ├── icon_30x30.png
│   └── icon_60x60.png
└── Output/  # Built .uapp files
```

### Creating New Apps

1. **Copy CMake Template**:
   ```bash
   # Create app dir
   mkdir -p MyApp
   cp -r SDK/Examples/Apps/<app-name>/Software/<app-name>-CMake/* MyApp/

   # Copy libs
   mkdir -p MyApp/Libs
   cp -r SDK/Examples/Apps/<app-name>/Software/Libs/* MyApp/Libs/

   # For GUI: Copy TouchGFX
   mkdir -p MyApp/TouchGFX-GUI
   cp -r SDK/Examples/Apps/<app-name>/Software/Apps/TouchGFX-GUI/* MyApp/TouchGFX-GUI/
   ```

2. **Customize CMakeLists.txt**:
   - Update `set(APP_NAME "<app-name>")` to `set(APP_NAME "MyApp")`
   - Set unique `APP_ID` (e.g., generate via `python -c 'import hashlib; print(hashlib.md5(b"MyApp").hexdigest().upper()[:8])'`)
   - Verify paths
     - `LIBS_PATH`: Path to shared libraries (example defaults: `../../Libs`)
     - `OUTPUT_PATH`: Path to output directory (example defaults: `../../../Output`)
     - `RESOURCES_PATH`: Path to resources (example defaults: `../../../Resources`)
   - Optional Variables
      - `TOUCHGFX_PATH`: Path to TouchGFX GUI directory (e.g., `../TouchGFX-GUI`) – for GUI apps only
      - `UNA_APP_SERVICE_STACK_SIZE`: Service stack size (e.g., `10*1024`)
      - `UNA_APP_SERVICE_RAM_LENGTH`: Service RAM length (e.g., `500K`)
      - `UNA_APP_GUI_STACK_SIZE`: GUI stack size (e.g., `10*1024`)
      - `UNA_APP_GUI_RAM_LENGTH`: GUI RAM length (e.g., `600K`)

3. **Implement Logic**:
   - Edit `Libs/Sources/` for service code.
   - For GUI, configure TouchGFX in `TouchGFX-GUI/` (e.g., update `application.config`, generate code via TouchGFX Designer).

4. **Add Resources**:
   - Place icons in `Resources/` (30x30 and 60x60 PNGs required).

5. **Set Environment** (as in Step 3 above).

#### CMake Template Examples
Use these as starting points in `CMakeLists.txt`.

**Service-Only**:
```cmake
cmake_minimum_required(VERSION 3.21)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")

# App config
set(APP_NAME "MyApp")
set(APP_ID "YOUR_UNIQUE_ID")
set(DEV_ID "UNA")
set(LIBS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Libs") # Adjust for your sources tree
set(OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Output") # Adjust for your sources tree
set(RESOURCES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Resources") # Adjust for your sources tree

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
include(${LIBS_PATH}/libs.cmake)

project(${APP_NAME})

una_app_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

set(SERVICE_SOURCES ${LIBS_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp" ${UNA_SDK_SOURCES_COMMON} ${UNA_SDK_SOURCES_SERVICE})
set(SERVICE_INCLUDE_DIRS ${LIBS_INCLUDE_DIRS} ${UNA_SDK_INCLUDE_DIRS_COMMON} ${UNA_SDK_INCLUDE_DIRS_SERVICE})

una_app_build_service(${APP_NAME}Service.elf)

una_app_build_app()
```

**Service+GUI** (add after service build):
```cmake
set(TOUCHGFX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../TouchGFX-GUI")
include(${TOUCHGFX_PATH}/touchgfx.cmake)

set(GUI_SOURCES ${TOUCHGFX_SOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/PaintImpl.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/syscalls.cpp" ${UNA_SDK_SOURCES_COMMON} ${UNA_SDK_SOURCES_GUI})
set(GUI_INCLUDE_DIRS ${LIBS_INCLUDE_DIRS} ${UNA_SDK_INCLUDE_DIRS_COMMON} ${UNA_SDK_INCLUDE_DIRS_GUI} ${TOUCHGFX_INCLUDE_DIRS})

una_app_build_gui(${APP_NAME}GUI.elf)
```

### Building Apps Manually
Navigate to the app's CMake directory:
```bash
cd MyApp/Software/Apps/MyApp-CMake
# Ensure UNA_SDK and other vars are exported

# Configure (creates build dir)
cmake -S . -B build

# Build
cmake --build build

# Output: .uapp in Output/
```

For clean rebuild: `rm -rf build && cmake -S . -B build && cmake --build build`.

## Alternative Workflow: STM32CubeIDE

For IDE users, copy CubeIDE projects from Examples.

### Creating New Apps in CubeIDE
1. **Copy Projects**:
   - Copy from `SDK/Examples/<app-name>` to `SDK/Examples/<your-app-name>`

2. **Customize**:
   - Edit `.project` and `.cproject`: Replace names/IDs.
   - Update `Linker.ld`: App name, paths.
   - Modify `Core/Src/` and `Core/Inc/` for logic.

3. **Import to CubeIDE**:
   - File > Import > Existing Projects into Workspace > Select copied dirs.
   - Fix paths in Project Properties > C/C++ Build > Settings.

4. **Build**:
   - Build service/GUI separately.
   - Manual merge: Use SDK scripts (e.g., `Utilities/Scripts/app_merging/app_merging.py`) with icons/resources.

This workflow integrates with TouchGFX Designer for GUI.

## TouchGFX

TouchGFX is a high-performance graphics framework designed for STM32 microcontrollers, enabling rich graphical user interfaces on embedded devices with limited resources.

### Installation

1. **Download and Install TouchGFX Designer**:
   - Create a free account on the [STMicroelectronics website](https://www.st.com).
   - Download the latest version of TouchGFX from the ST Developer Zone.
   - Install the TouchGFX Designer tool on your development machine (Windows, Linux, or macOS supported).

2. **Framework Integration**:
   - In the UNA SDK, TouchGFX is pre-integrated in example GUI projects (e.g., `SDK/Examples/Apps/<app>/Software/Apps/TouchGFX-GUI/`).
   - For new projects, copy the TouchGFX-GUI directory from an example and ensure the framework libraries are available via the SDK paths.
   - No separate installation of the framework is needed if using the SDK's bundled version; otherwise, download the TouchGFX framework package from ST.

### Using TouchGFX Designer for Visual Editing

- **Launch Designer**: Open TouchGFX Designer and create a new project or open an existing `.touchgfx` configuration file (e.g., `MyAppGUI.touchgfx`).

- **Visual Design**:
  - Define the display resolution and color depth in the project settings (e.g., 240x240 for typical watch displays).
  - Use the drag-and-drop interface to add widgets such as buttons, sliders, text fields, images, graphs, and animations.
  - Organize screens (e.g., main menu, settings, data views) and define transitions between them.
  - Import assets: Add fonts, images, and videos to the `assets/` directory.
  - Configure interactions: Link button presses to actions, animations to timers, and data bindings to model updates.

- **Preview and Simulate**: Use the built-in simulator to test the UI on your development machine before generating code.

### Code Generation

- After completing the design, click **Generate Code** in TouchGFX Designer.
- This automatically creates C++ source files in the `generated/` directory:
  - `gui_generated/`: Base classes for screens (Views), presenters, and the frontend application.
  - `images/`: Bitmap and image handling code.
  - `fonts/`: Font rendering and caching.
  - `texts/`: Multilingual text resources.
  - Simulator files for desktop testing.
- The generated code is optimized for the target hardware and includes hardware abstraction for touch input and display drivers.

### Integration with CMake

In UNA SDK projects, TouchGFX integrates seamlessly with CMake for building GUI applications:

1. **Set Paths**:
   - In `CMakeLists.txt`, define `TOUCHGFX_PATH` pointing to your TouchGFX project directory (e.g., `set(TOUCHGFX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../TouchGFX-GUI")`).
   - Optionally, set the environment variable `TOUCHGFX_GUI_PATH` to the same path for global reference across builds.

2. **Include TouchGFX Build**:
   ```cmake
   include(${TOUCHGFX_PATH}/touchgfx.cmake)
   
   set(GUI_SOURCES ${TOUCHGFX_SOURCES} "PaintImpl.cpp" "syscalls.cpp" ${UNA_SDK_SOURCES_COMMON} ${UNA_SDK_SOURCES_GUI})
   set(GUI_INCLUDE_DIRS ${TOUCHGFX_INCLUDE_DIRS} ${UNA_SDK_INCLUDE_DIRS_GUI})
   
   una_app_build_gui(${APP_NAME}GUI.elf)
   ```

3. **For Service+GUI Apps**:
   - Build the service executable first (as in the service-only template).
   - Then build the GUI executable, which links against TouchGFX.
   - Use SDK tools to merge the service, GUI, and resources into a single `.uapp` file for deployment.

4. **Dependencies**:
   - Ensure `UNA_SDK` is set, as it provides STM32 HAL and CMSIS integrations.
   - For custom hardware, adjust display and touch drivers in `touchgfx/hal/`.

### Modifying Generated Code Safely

- **Avoid Editing Generated Files**: Files in `generated/` are automatically overwritten during code generation. Any manual changes will be lost.

- **Custom Code Locations**:
  - Place custom implementations in `gui/src/`:
    - `gui/src/<screen>_view.cpp`: Override view logic, handle user interactions, and update UI elements.
    - `gui/src/<screen>_presenter.cpp`: Manage data flow between view and model.
    - `gui/src/model/model.cpp`: Handle backend data and service communication.
    - `gui/src/common/frontend_application.cpp`: App-wide initialization, screen transitions, and tick handling.
  - For reusable components, create custom containers in `gui/src/containers/`.

- **Regeneration Workflow**:
  - Design in Designer → Generate Code → Implement custom logic in `gui/src/` → Rebuild.

- **Version Control Tip**: Commit the `.touchgfx` file, assets, and custom `gui/src/` code, but exclude or .gitignore the `generated/` directory.

### Best Practices for Custom GUI Development in Service+GUI Apps

- **Separation of Concerns**: Keep the service layer focused on business logic (e.g., sensor data processing, file I/O) and the GUI layer on presentation. Use inter-process communication (e.g., via queues, events, or shared memory) to pass data between service and GUI threads.

- **Leverage MVP Pattern**: TouchGFX's Model-View-Presenter architecture promotes clean code: Model for data, View for UI, Presenter for logic.

- **Performance Optimization**:
  - Use vector fonts and compressed images to minimize flash usage.
  - Limit frame rates (e.g., 30 FPS) and use partial frame buffer updates for power efficiency.
  - Profile memory usage with TouchGFX's built-in tools.

- **Touch and Input Handling**: Calibrate touch sensitivity and handle multi-touch if supported. Integrate with the SDK's input HAL.

- **Testing**:
  - Use the TouchGFX Simulator for rapid iteration.
  - Test on hardware early, especially for touch responsiveness and power draw.
  - Unit test custom presenters and models.

- **UNA-Specific Tips**:
  - Ensure app icons (30x30 and 60x60 PNGs) are in `Resources/` for launcher integration.
  - Set unique `APP_ID` in CMake to avoid conflicts.
  - For watch apps, design for small screens and battery life; use dark themes and efficient animations.
  - When merging service+GUI, verify stack sizes (`UNA_APP_GUI_STACK_SIZE`) are adequate for your UI complexity.

With these practices, you can develop responsive, professional GUIs that integrate smoothly with the UNA platform's service architecture.

## **Troubleshooting**

- **Missing Dependencies**: Ensure all required libraries are linked.
- **Path Errors**: Verify `UNA_SDK` and other paths are correctly set.
- **Toolchain Issues**: Confirm ARM GCC is properly installed and in PATH.
- **Build Conflicts**: Clean build dirs if errors persist after changes.