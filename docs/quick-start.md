# Una-Watch SDK Quick Start

Welcome to Una-Watch development! This guide will get you running your first app in under 10 minutes. Una-Watch apps are unique - they run as pure machine code directly in the watch's MCU memory, using position-independent execution and shared libc libraries.

## What Makes Una-Watch Special

Unlike traditional smartwatch platforms, Una-Watch apps are:
- **Pure Machine Code**: Compiled ELF binaries executing directly in MCU memory
- **Position-Independent**: Apps abstracted from kernel using PIC (Position-Independent Code)
- **Shared libc**: Memory-efficient library sharing across all apps
- **Dual-Process**: Service (background) + GUI (interface) process architecture

## Prerequisites

- Linux/macOS/Windows development machine
- STM32CubeIDE (recommended) or command-line tools
- USB cable for device flashing

## Relation to Watch SDK Current Structure

The current watch SDK structure has several issues:

1. **Monolithic CMakeLists.txt**: Hardcoded paths, not modular
2. **TouchGFX dependency**: GUI framework tightly coupled, generated code mixed with custom code
3. **No external SDK reference**: Everything bundled, hard to update SDK independently
4. **Manual process**: Copy entire apps, modify manually - not sustainable

## Existing CubeIDE Workflow (Maintained)

The current development process using CubeIDE and TouchGFX remains fully supported:

### CubeIDE + TouchGFX Process

1. **Copy App Template**: Copy entire app directory structure from `SDK/examples/Apps/<app_name>` to your workspace
2. **Modify TouchGFX GUI**: Use TouchGFX Designer to modify GUI screens
3. **Modify CubeIDE Projects**: Update C++ code in separate GUI and Service projects
4. **Generate Binaries**: Use app_packer.py to create .gui and .srv files
5. **Merge App**: Use app_merging.py to combine into final .uapp package

### Advantages of CubeIDE Workflow

- **Visual GUI Design**: TouchGFX Designer for WYSIWYG interface design
- **IDE Integration**: Full debugging and development environment
- **Existing Investment**: All current projects continue to work
- **Familiar Tools**: Developers already know the workflow

#### Adapting CMake for Watch Apps (TouchGFX-Compatible)

The team goal is to achieve ESP-IDF-like confidence while maintaining compatibility with existing TouchGFX and CubeIDE workflows, we need to restructure the watch SDK build system.

## New CMake Workflow

The UNA SDK now supports a modern CMake-based build system that provides ESP-IDF-like confidence while maintaining compatibility with existing TouchGFX and CubeIDE workflows.

### Location Independence Features

The new CMake workflow provides complete location independence through:

1. **Environment Variable Reference**: Apps reference the SDK via the `UNA_SDK` environment variable, allowing apps to be placed anywhere on the filesystem
2. **Relative Path Configuration**: All paths are configured relative to the app's CMake directory using a `.env` file
3. **Flexible Project Placement**: Apps can be developed outside the SDK repository while still referencing SDK components

### una.py Commands

The `una.py` tool provides a unified interface for watch app development:

#### `una.py export`
Prints the export command for the UNA_SDK environment variable:
```bash
una.py export
# Output: export UNA_SDK=/path/to/una-watch-sdk
```

#### `una.py init`
Interactive setup of the `.env` configuration file:
```bash
cd MyApp/Software/Apps/MyApp-CMake
una.py init
```

#### `una.py create <name> <type>`
Creates a new app from template with the specified type (`service-only` or `service-gui`):
```bash
una.py create MyServiceApp service-only
una.py create MyGuiApp service-gui
```

### .env Configuration

The `.env` file defines relative paths and app configuration:

#### Required Variables
- `LIBS_PATH`: Relative path to the shared libraries directory
- `OUTPUT_PATH`: Relative path to the output directory
- `RESOURCES_PATH`: Relative path to the resources directory
- `APP_PATH`: Relative path to the app's root directory (usually `.`)
- `APP_TYPE`: Type of app (`service-only` or `service+gui`)

#### Optional Variables (Service+GUI apps)
- `TOUCHGFX_GUI_PATH`: Relative path to the TouchGFX GUI project directory

#### Example .env for service+gui app
```
LIBS_PATH=../../Libs
TOUCHGFX_GUI_PATH=../TouchGFX-GUI
OUTPUT_PATH=../../../Output
RESOURCES_PATH=../../../Resources
APP_PATH=.
APP_TYPE=service+gui
```

#### Example .env for service-only app
```
LIBS_PATH=../../Libs
OUTPUT_PATH=../../../Output
RESOURCES_PATH=../../../Resources
APP_PATH=.
APP_TYPE=service-only
```

### CMake Templates

Two CMake templates support different app types:

#### Service-Only App Template

For apps with only background service functionality:

```cmake
# Watch App CMakeLists.txt - Service-Only Template
cmake_minimum_required(VERSION 3.21)
include($ENV{UNA_SDK}/cmake/watch_project.cmake)

project(MyApp)

# Load configuration from .env file
file(READ .env ENV_CONTENT)
string(REPLACE "\n" ";" ENV_LINES ${ENV_CONTENT})
foreach(line ${ENV_LINES})
    string(STRIP "${line}" line)
    if(line MATCHES "^([^=]+)=(.*)$")
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()

# App configuration
set(APP_ID "your_app_id_here")  # Replace with actual app ID
set(APP_NAME "MyApp")           # Replace with actual app name
set(DEV_ID "UNA")               # Developer ID

# Set up build version
watch_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Build service executable
watch_build_service(${APP_NAME}Service.elf ${LIBS_PATH} "${TOUCHGFX_GUI_PATH}")
```

#### Service+GUI App Template

For apps with both service and TouchGFX-based GUI components:

```cmake
# Watch App CMakeLists.txt - Service+GUI Template
cmake_minimum_required(VERSION 3.21)
include($ENV{UNA_SDK}/cmake/watch_project.cmake)

project(MyApp)

# Load configuration from .env file
file(READ .env ENV_CONTENT)
string(REPLACE "\n" ";" ENV_LINES ${ENV_CONTENT})
foreach(line ${ENV_LINES})
    string(STRIP "${line}" line)
    if(line MATCHES "^([^=]+)=(.*)$")
        set(${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
    endif()
endforeach()

# App configuration
set(APP_ID "your_app_id_here")  # Replace with actual app ID
set(APP_NAME "MyApp")           # Replace with actual app name
set(DEV_ID "UNA")               # Developer ID

# Set up build version
watch_setup_version(BUILD_VERSION ${CMAKE_CURRENT_SOURCE_DIR})

# Build service and GUI executables
watch_build_service(${APP_NAME}Service.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})
watch_build_gui(${APP_NAME}GUI.elf ${LIBS_PATH} ${TOUCHGFX_GUI_PATH})

# Scripts path
set(SCRIPTS_PATH "${UNA_SDK}/Utilities/Scripts")

# Final app merging
add_custom_target(${APP_NAME}App ALL
    DEPENDS ${APP_NAME}Service.elf ${APP_NAME}GUI.elf
    COMMAND python3 ${SCRIPTS_PATH}/app_merging/app_merging.py
        -normal_icon ${RESOURCES_PATH}/icon_60x60.png
        -small_icon ${RESOURCES_PATH}/icon_30x30.png
        -name ${APP_NAME}
        -type Activity
        -glance_capable
        -out ${CMAKE_CURRENT_BINARY_DIR}
        -appid ${APP_ID}
        -appver ${BUILD_VERSION}
        -scripts ${SCRIPTS_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/Tmp ${OUTPUT_PATH}/Tmp
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}_${BUILD_VERSION}.uapp ${OUTPUT_PATH}/
    COMMENT "Merging ${APP_NAME} application"
)
```

### Building Examples

The easiest way to get started:

```bash
# Clone the SDK
git clone --recursive https://github.com/UNAWatch/una-sdk.git
cd una-sdk

# Build the Running example
./tools/una.py build-example Running

# The built .uapp file will be in:
# examples/Apps/Running/Software/Apps/Running-CMake/build/
```

### Creating New Apps

Use the `una.py create` command to start new projects:

```bash
# Create a service-only app
una.py create MyServiceApp service-only

# Create a service+GUI app
una.py create MyGuiApp service-gui
```

This creates the complete directory structure, CMakeLists.txt, .env file, and copies necessary build files.

### Building Apps

For external apps (outside the SDK repository):

```bash
# Set SDK path
export UNA_SDK=/path/to/una-watch-sdk

# Navigate to app CMake directory
cd MyApp/Software/Apps/MyApp-CMake

# Configure and build
una.py build
```

### Dual Workflow Benefits

1. **Backward Compatibility**: Existing CubeIDE + TouchGFX projects remain fully functional
2. **Choice of Tools**: Developers can use familiar CubeIDE or new CMake workflow
3. **ESP-IDF-like Experience**: CMake workflow provides the same confidence as ESP-IDF
4. **Location Independence**: Apps can be placed anywhere via UNA_SDK environment variable
5. **Relative Path Configuration**: .env files enable flexible project structures
6. **Unified Tooling**: una.py provides consistent interface across all operations
7. **Shared SDK**: Common build logic centralized in SDK, reducing duplication
8. **Maintainable**: Clear separation between SDK and app-specific code

### Implementation Details

#### Project Structure for CMake Workflow

```
# SDK Repository
una-watch-sdk/
├── cmake/            # Common CMake modules
│   ├── toolchain-arm-none-eabi.cmake
│   ├── watch_project.cmake  # Common project setup
│   └── watch_components.cmake  # Component definitions
├── tools/
│   └── una.py        # Build tool
├── examples/         # Example apps
│   └── Apps/         # App examples
│       ├── Running/
│       ├── Alarm/
│       └── ...
├── Libs/            # SDK core libraries
├── docs/            # SDK documentation
└── .git/

# External App Development
my-apps-workspace/
├── MyServiceApp/
│   ├── Software/
│   │   ├── Apps/
│   │   │   └── MyServiceApp-CMake/
│   │   │       ├── CMakeLists.txt
│   │   │       ├── .env
│   │   │       ├── MyServiceAppService.ld
│   │   │       └── syscalls.cpp
│   │   └── Libs/
│   │       ├── Header/
│   │       └── Sources/
│   ├── Resources/
│   │   ├── icon_30x30.png
│   │   └── icon_60x60.png
│   └── Output/
└── MyGuiApp/
    ├── Software/
    │   ├── Apps/
    │   │   ├── MyGuiApp-CMake/
    │   │   │   ├── CMakeLists.txt
    │   │   │   ├── .env
    │   │   │   ├── MyGuiAppService.ld
    │   │   │   ├── MyGuiAppGUI.ld
    │   │   │   ├── syscalls.cpp
    │   │   │   └── PaintImpl.cpp
    │   │   └── TouchGFX-GUI/
    │   │       └── ... (TouchGFX project)
    │   └── Libs/
    │       ├── Header/
    │       └── Sources/
    ├── Resources/
    │   ├── icon_30x30.png
    │   └── icon_60x60.png
    └── Output/
```

#### Required Files for CMake Apps

- **CMakeLists.txt**: Project configuration and build logic
- **.env**: Relative path configuration
- **Linker scripts**: `AppNameService.ld` and `AppNameGUI.ld` (for GUI apps)
- **System files**: `syscalls.cpp` and `PaintImpl.cpp` (for GUI apps)
- **App logic**: Service implementation in `Libs/Header/` and `Libs/Sources/`
- **Resources**: Icons in `Resources/` directory
- **TouchGFX project**: GUI implementation (for service+GUI apps)
