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
- **CubeIDE**: Copy from `SDK/examples/<app-name>` to `SDK/examples/<your-app-name>` to avoid path dependencies issue out of the box.
- **CMake**:
  - Copy from `SDK/examples/<app-name>` to whatever you want and adjust `CMakeLists.txt` accoringly.Make shure UNA_SDK environment variable is defined.
  - Set UNA_SDK for CMake:
      Manually add to your shell profile (e.g., `~/.bashrc` or `~/.zshrc`):
      ```bash
      export UNA_SDK=/path/to/una-sdk
      ```
      Reload your shell or run `source ~/.bashrc`.
  - CMake require the `UNA_SDK` environment variable to point to the SDK root directory. This enables location-independent app development.

## CMake-Based Workflow Development

The CMake workflow is recommended for command-line control and is fully standalone. Apps reference the SDK via `UNA_SDK` and use relative paths for flexibility.

### `examples/` Project Structure Overview
```
MyApp/  # App root (can be anywhere)
├── Software/
│   ├── Apps/
|   │   ├── MyApp-Service-CubeIDE/ # CubeIDE project with Service
|   │   ├── MyApp-GUI-CubeIDE/ # CubeIDE project with GUI
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
   cp -r SDK/examples/Apps/<app-name>/Software/<app-name>-CMake/* MyApp/

   # Copy libs
   mkdir -p MyApp/Libs
   cp -r SDK/examples/Apps/<app-name>/Software/Libs/* MyApp/Libs/

   # For GUI: Copy TouchGFX
   mkdir -p MyApp/TouchGFX-GUI
   cp -r SDK/examples/Apps/<app-name>/Software/Apps/TouchGFX-GUI/* MyApp/TouchGFX-GUI/
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

For IDE users, copy CubeIDE projects from examples.

### Creating New Apps in CubeIDE
1. **Copy Projects**:
   - Copy from `SDK/examples/<app-name>` to `SDK/examples/<your-app-name>`

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


## **Troubleshooting**

- **Missing Dependencies**: Ensure all required libraries are linked.
- **Path Errors**: Verify `UNA_SDK` and other paths are correctly set.
- **Toolchain Issues**: Confirm ARM GCC is properly installed and in PATH.
- **Build Conflicts**: Clean build dirs if errors persist after changes.
