# UNA-Watch SDK

The Una-Watch SDK provides a comprehensive suite of tools, libraries, and interfaces for building high-performance wearable applications. This SDK enables developers to create apps for the UNA watch platform with support for sensor data, GUI interfaces, file systems, and inter-process communication.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Opening SDK in VSCode](#opening-sdk-in-vscode)
4. [Compiling Projects](#compiling-projects)
5. [Compiling Examples](#compiling-examples)

## Overview

### Key Components

- **Core Interfaces**: Type-safe APIs for system services, communication, and hardware access
- **Message System**: High-performance inter-process communication (IPC) framework
- **Sensor APIs**: Simplified access to PPG, IMU, and GNSS sensors
- **File System**: Multi-volume storage management (Flash, USB, External)
- **Build Tools**: Automated scripts for compilation and `.uapp` packaging
- **Simulator**: Desktop-based environment for rapid prototyping and testing

### SDK Project Structure

```
SDK/
├── Libs/                    # SDK Interface Headers and Implementation
│   ├── Header/SDK/         # Core API interfaces
│   └── Source/SDK/         # SDK Implementation
├── Port/                   # Platform-specific integrations
├── ThirdParty/             # External dependencies (coreJSON, FitSDK)
├── Utilities/Scripts/      # Build and packaging tools
├── Examples/              # Sample applications
├── cmake/                 # CMake build system files
└── Simulator/             # Development simulator
```

### High-Level Utilities

The SDK includes several utilities for common tasks:

- **GSModel & GSBridge**: Type-safe GUI-Service communication
- **Glance UI System**: Lightweight UI for 240x60 notification area
- **FitHelper**: Garmin FIT file generation
- **TrackMapBuilder**: GPS track visualization
- **Serialization**: CBOR and JSON stream readers/writers
- **Signal Processing**: Filters, queues, timers

## Prerequisites

### Required Tools

- **Development Machine**: Linux, macOS, or Windows
- **CMake**: Version 3.21+ (for manual builds)
- **Python**: Version 3.6+ (for build scripts)
- **Git**: For cloning the SDK
- **USB Cable**: For device flashing

### Toolchain Setup

**CRITICAL**: Use STMicroelectronics GNU Tools for STM32 (from CubeIDE/CubeCLT). System `gcc-arm-none-eabi` (GCC 13.2+) is incompatible due to missing newlib syscall stubs.

#### Linux (Ubuntu/Debian)

1. Download and install [STM32CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html):
   ```bash
   chmod +x ~/Downloads/STM32CubeCLT_Linux64_v1-*.run
   ./STM32CubeCLT_Linux64_v1-*.run
   ```

2. Add to PATH in your shell profile (`~/.bashrc` or `~/.zshrc`):
   ```bash
   export PATH="$HOME/.local/share/stm32cube/bundles/gnu-tools-for-stm32/*/bin:$PATH"
   ```

3. Reload your shell and verify:
   ```bash
   source ~/.bashrc
   arm-none-eabi-gcc --version  # Should show ~14.3+st, not 13.2
   ```

#### Windows

1. Install [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)

2. Add the following to your PATH environment variable:
   - `C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin`
   - `C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cmake.win32_1.1.0.202409170845\tools\bin`
   - `C:\ST\STM32CubeIDE_*\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_1.1.0.202409170845\tools\bin`

3. Restart your command prompt/IDE and verify with `arm-none-eabi-gcc --version`

### Environment Variables

Set the `UNA_SDK` environment variable to point to the SDK root directory:

**Linux/macOS** (add to `~/.bashrc` or `~/.zshrc`):
```bash
export UNA_SDK=/path/to/una-sdk
```

**Windows** (System Properties > Environment Variables):
```
UNA_SDK=C:\path\to\una-sdk
```

## Opening SDK in VSCode

1. Launch VSCode
2. Open the SDK folder: `File > Open Folder...` and select the `SDK` directory
3. The workspace will automatically load with the correct configuration
4. VSCode extensions for C/C++, CMake, and ARM development are recommended

## Compiling Projects

The SDK supports multiple build systems for different development workflows.

### CMake-Based Builds

Recommended for command-line control and flexibility.

#### Prerequisites
- CMake 3.21+
- ARM GCC toolchain in PATH
- `UNA_SDK` environment variable set

#### Building a Project

1. Navigate to the project's CMake directory (e.g., `MyApp/Software/Apps/MyApp-CMake/`)

2. Configure the build:
   ```bash
   cmake -G "Unix Makefiles" -S . -B build
   ```

3. Build the project:
   ```bash
   cmake --build build
   ```

4. Clean rebuild:
   ```bash
   rm -rf build
   cmake -S . -B build
   cmake --build build
   ```

### STM32CubeIDE Builds

For IDE-based development with integrated debugging.

#### Setting Up a Project

1. Copy a CubeIDE project from `SDK/Examples/Apps/<app-name>/Software/Apps/<app-name>CubeIDE/`

2. Import into CubeIDE:
   - `File > Import > Existing Projects into Workspace`
   - Select the copied project directories

3. Update project properties:
   - Right-click project > `Properties > C/C++ Build > Settings`
   - Verify toolchain paths and include directories

#### Building in CubeIDE

1. Select the project in Project Explorer
2. `Project > Build Project` or press Ctrl+B
3. For service+GUI apps, build both service and GUI projects separately

### TouchGFX GUI Builds

For applications with graphical user interfaces.

#### Prerequisites
- TouchGFX Designer installed
- ARM GCC toolchain
- CMake (for integrated builds)

#### Building TouchGFX Projects

1. Open TouchGFX Designer and load the `.touchgfx` project file

2. Generate code:
   - Click `Generate Code` in TouchGFX Designer
   - This creates C++ files in the `generated/` directory

3. Build using CMake (integrated):
   ```bash
   cd TouchGFX-GUI/
   cmake -G "Unix Makefiles" -S . -B build
   cmake --build build
   ```

4. Or build directly in TouchGFX Designer:
   - Use the built-in simulator for testing
   - Export for target hardware

## Compiling Examples

The SDK includes several example applications demonstrating different features and build systems.

### Available Examples

- **Alarm**: Basic alarm functionality
- **Cycling**: Activity tracking with GPS
- **GlanceARHR**: Glance interface for heart rate
- **GlanceHR**: Simple heart rate display
- **GlanceSteps**: Step counter glance
- **Hiking**: Full activity app with TouchGFX GUI
- **HRMonitor**: Heart rate monitoring
- **Running**: Running activity tracker

### Compiling Examples with CMake

Each example supports CMake builds. Here's how to build the Cycling example:

1. Set environment variables:
   ```bash
   export UNA_SDK=/path/to/sdk
   cd SDK/Examples/Apps/Cycling/Software/Apps/Cycling-CMake
   ```

2. Configure and build:
   ```bash
   cmake -G "Unix Makefiles" -S . -B build
   cmake --build build
   ```

3. Output: `.uapp` file in the `Output/` directory

### Compiling Examples with CubeIDE

For IDE-based compilation:

1. Copy the example to avoid modifying originals:
   ```bash
   cp -r SDK/Examples/Apps/Cycling/Software/Apps/CyclingService-CubeIDE SDK/Examples/Apps/MyCyclingService
   cp -r SDK/Examples/Apps/Cycling/Software/Apps/CyclingGUI-CubeIDE SDK/Examples/Apps/MyCyclingGUI
   ```

2. Import projects into CubeIDE

3. Build each project separately

4. Merge using SDK scripts:
   ```bash
   python3 SDK/Utilities/Scripts/app_merging/app_merging.py \
     -name Cycling \
     -type Activity \
     -normal_icon SDK/Examples/Apps/Cycling/Resources/icon_60x60.png \
     -small_icon SDK/Examples/Apps/Cycling/Resources/icon_30x30.png \
     -appid <unique_id> \
     -appver 1.0.0 \
     -scripts SDK/Libs/Source/AppSystem
   ```

### Compiling TouchGFX Examples

For examples with GUI components like Hiking:

1. Open TouchGFX Designer:
   ```bash
   cd SDK/Examples/Apps/Hiking/Software/Apps/TouchGFX-GUI
   # Launch TouchGFX Designer and open HikingGUI.touchgfx
   ```

2. Generate code in TouchGFX Designer

3. Build service component:
   ```bash
   cd SDK/Examples/Apps/Hiking/Software/Apps/Hiking-CMake
   cmake -G "Unix Makefiles" -S . -B build
   cmake --build build
   ```

4. Merge service and GUI:
   - Use the app merging script as shown above
   - Include both service and GUI ELF files

### Common Build Issues

- **Missing UNA_SDK**: Ensure the environment variable is set
- **Toolchain not found**: Verify ARM GCC is in PATH
- **CMake errors**: Clean build directory and reconfigure
- **TouchGFX issues**: Regenerate code after Designer changes
- **Merge failures**: Check icon files exist and APP_ID is unique

### Next Steps

- Explore the [SDK Setup Guide](Docs/sdk-setup.md) for detailed workflows
- Read the [Platform Overview](Docs/platform-overview.md) for architecture details
- Try building the [Alarm App Tutorial](Docs/build-your-first-app.md)
- Join the community at [UNAWatch/una-sdk](https://github.com/UNAWatch/una-sdk)

For additional support, see the [Community Support](Docs/community-support.md) guide.