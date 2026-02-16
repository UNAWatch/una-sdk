# UNA-Watch SDK

The Una-Watch SDK provides a comprehensive suite of tools, libraries, and interfaces for building high-performance wearable applications. This SDK enables developers to create apps for the UNA watch platform with support for sensor data, GUI interfaces, file systems, and inter-process communication.

For full setup instructions (Linux/Windows, toolchain, environment variables, and a buildable example), see [Docs/sdk-setup.md](Docs/sdk-setup.md).

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

Keep this README high-level; detailed prerequisites, toolchain notes, and environment variables are documented in [Docs/sdk-setup.md](Docs/sdk-setup.md).

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

See [Docs/sdk-setup.md](Docs/sdk-setup.md) for the exact toolchain requirements and how to set `UNA_SDK`.

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

Each example supports CMake builds. For a complete, copy-and-build walkthrough, follow the **Alarm CMake** example in [Docs/sdk-setup.md](Docs/sdk-setup.md).

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
- Try building the [Alarm App Tutorial](Docs/Examples/Alarm-ARCHITECTURE.md)
- Join the community at [UNAWatch/una-sdk](https://github.com/UNAWatch/una-sdk)

For additional support, see the [Community Support](Docs/community-support.md) guide.
