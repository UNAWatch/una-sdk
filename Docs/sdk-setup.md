# SDK Setup and Build Overview

This guide covers SDK installation, manual environment setup, and primary workflows using CMake or STM32CubeIDE.

The focus is on a **manual, transparent setup** (no helper scripts required on Linux) so you can see exactly which tools are used and which environment variables are expected.

For platform architecture, see [Platform Overview](platform-overview.md). For tutorials, see [Overview: Alarm App](build-your-first-app.md).

## Prerequisites

- Linux/macOS/Windows development machine
- CMake 3.21+ (for manual builds)
- A build tool: `make` (Unix Makefiles)
- Python 3 + pip (for packaging/build utilities)
- **ST ARM GCC Toolchain (CRITICAL)**: STM32CubeIDE or STM32CubeCLT version **required**. System `gcc-arm-none-eabi` is often incompatible (newlib syscall stubs such as `_write` can be missing). See [Toolchain Setup](#toolchain-setup).
- USB cable for device flashing
- Git for cloning the SDK

The rest of this document provides OS-specific, end-to-end setup sections (Linux/Windows) that follow the same flow:

**Get required software → Prepare → Clone and setup environment → Copy example and build**

## Toolchain Setup

**CRITICAL**: Use STMicroelectronics GNU Tools for STM32 (from **STM32CubeIDE** or **STM32CubeCLT**).

The system `gcc-arm-none-eabi` shipped by many distros (often GCC 13+) is frequently incompatible with this SDK's bare-metal/newlib expectations and may fail with undefined syscall stubs (`_write`, `_close`, etc.).

### Linux Setup

#### Get required software

- [Git](https://git-scm.com/) (clone + version string generation)
- Python 3 + pip (packaging utilities)
- CMake 3.21+
- make
- **STM32CubeIDE** or **STM32CubeCLT** (provides the **ST** `arm-none-eabi-gcc` toolchain)

Install hints (keep it minimal; equivalents work on other distros):

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install -y git python3 python3-pip cmake build-essential
```

#### Prepare

##### Toolchain (CubeIDE/CubeCLT) and PATH

1) Install the ST toolchain (choose one):

- **STM32CubeCLT** (CLI-first):
  ```bash
  chmod +x ~/Downloads/STM32CubeCLT_Linux64_v1-*.run
  ~/Downloads/STM32CubeCLT_Linux64_v1-*.run
  ```

- **STM32CubeIDE** (IDE + toolchain):
  Download/install from: https://www.st.com/en/development-tools/stm32cubeide.html

2) Ensure the ST toolchain is on your `PATH`.

If you installed **CubeCLT**, the toolchain is typically under:

```bash
export PATH="$HOME/.local/share/stm32cube/bundles/gnu-tools-for-stm32/*/bin:$PATH"
```

If `arm-none-eabi-gcc` is still not found and you installed **CubeIDE**, add the directory that contains `arm-none-eabi-gcc` to your `PATH` (it is usually a `.../plugins/...gnu-tools-for-stm32.../tools/bin` folder inside the CubeIDE installation).

##### Clone and setup environment

```bash
# Clone
git clone --recursive https://github.com/UNAWatch/una-sdk.git
cd una-sdk

# Export environment for the current shell
export UNA_SDK="$PWD"

# Satisfy python dependencies
python3 -m pip install -r "$UNA_SDK/Utilities/Scripts/app_packer/requirements.txt"
```

##### Verify your environment

This SDK expects the toolchain and build tools to be discoverable via `PATH`.

```bash
which arm-none-eabi-gcc || true
which cmake || true
which make || true
which python3 || true
python3 -m pip --version

arm-none-eabi-gcc --version || true
cmake --version || true
make --version || true
```

If `which arm-none-eabi-gcc` prints nothing, your ST toolchain bin directory is not on `PATH` yet. Add it (see **Prepare** above) and re-open your terminal.

#### Copy example and build

This uses the same **Alarm CMake** example flow as Windows: **copy project → create build dir → configure → build**.

```bash
# Copy entire project for simplicity
cp -r "$UNA_SDK/Examples/Apps/Alarm/"* MyAlarm

# Create build dir
mkdir -p MyAlarm/build

# Configure
cmake -G "Unix Makefiles" -S MyAlarm/Software/Apps/Alarm-CMake -B MyAlarm/build

# Build
cmake --build MyAlarm/build
```

#### Optional (Linux)

##### Persist `UNA_SDK` and toolchain `PATH`

If you don't want to export variables every time, add them to your shell profile.

For bash:

```bash
echo 'export UNA_SDK="$HOME/path/to/una-sdk"' >> ~/.bashrc
echo 'export PATH="$HOME/.local/share/stm32cube/bundles/gnu-tools-for-stm32/*/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

For zsh:

```bash
echo 'export UNA_SDK="$HOME/path/to/una-sdk"' >> ~/.zshrc
echo 'export PATH="$HOME/.local/share/stm32cube/bundles/gnu-tools-for-stm32/*/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

##### Note about distro `gcc-arm-none-eabi`

If you installed `gcc-arm-none-eabi` from your distro repositories and builds fail with missing syscall stubs, switch to the **ST** toolchain from CubeIDE/CubeCLT and ensure its `arm-none-eabi-gcc` is the one found first in `PATH`.

### Windows Setup

#### Get required software

- [Git](https://git-scm.com/install/windows)
- [Python 3](https://www.python.org/downloads/windows/) -  Notes:
  - Enable "Add python.exe to PATH" checkmark
  - Click "Disable path length limit" after installation
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html#st-get-software) - Provides Toolchain, CMake, Make utilities (*note*: STM32CubeCLT does not includes `make` program, which is required)
- [VS Code](https://code.visualstudio.com/download) - IDE

#### Prepare

##### Clone and setup environment

```powershell
# Clone
git clone --recursive git@github.com:UNAWatch/una-sdk.git

# Export environment (persistent)
. ./una-sdk/Utilities/Scripts/export-stm32-tools.ps1

# Satisfy python dependencies
pip install -r ${env:UNA_SDK}/Utilities/Scripts/app_packer/requirements.txt
```

Reboot PC to apply environment variables

#### Copy example and build

```powershell
# Copy entire project for simplicity
cp -r ${env:UNA_SDK}/Examples/Apps/Alarm/* MyAlarm

# Create build dir
mkdir -p MyAlarm\build

# Call CMake
cmake -G "Unix Makefiles" -S MyAlarm/Software/Apps/Alarm-CMake -B MyAlarm/build

# Build
cmake --build MyAlarm/build
```

## CMake-Based Workflow Development

The CMake workflow is recommended for command-line control and is fully standalone. Apps reference the SDK via `UNA_SDK` and use relative paths for flexibility.

### `Examples/` Project Structure Overview
```
MyApp/  # App root (can be anywhere)
├── Software/
│   ├── Apps/
│   │   ├── MyApp-Service-CubeIDE/ # CubeIDE project with Service
│   │   ├── MyApp-GUI-CubeIDE/ # CubeIDE project with GUI
│   │   ├── MyApp-CMake/  # CMake project dir
│   │   │   ├── CMakeLists.txt
│   │   │   ├── MyAppService.ld  # Linker script
│   │   │   └── syscalls.cpp
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
    cp -r Examples/Apps/<app-name>/Software/Apps/<app-name>-CMake/* MyApp/

   # Copy libs
   mkdir -p MyApp/Libs
    cp -r Examples/Apps/<app-name>/Software/Libs/* MyApp/Libs/

   # For GUI: Copy TouchGFX
   mkdir -p MyApp/TouchGFX-GUI
    cp -r Examples/Apps/<app-name>/Software/Apps/TouchGFX-GUI/* MyApp/TouchGFX-GUI/
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

### Building Apps Manually
Navigate to the app's CMake directory:
```bash
cd MyApp/Software/Apps/MyApp-CMake
# Ensure UNA_SDK and other vars are exported

# Configure (creates build dir)
cmake -G "Unix Makefiles" -S . -B build

# Build
cmake --build build

# Output: .uapp in Output/
```

For clean rebuild: `rm -rf build && cmake -S . -B build && cmake --build build`.

## Alternative Workflow: STM32CubeIDE

For IDE users, copy CubeIDE projects from Examples.

### Creating New Apps in CubeIDE
1. **Copy Projects**:
   - Copy from `Examples/Apps/<app-name>` to `Examples/Apps/<your-app-name>`

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

## TouchGFX (require a Windows host)

TouchGFX is a high-performance graphics framework designed for STM32 microcontrollers, enabling rich graphical user interfaces on embedded devices with limited resources.

For now this is single supported way to create graphical applications for UNAwatch platform.

**Note**: Glances operates without TouchGFX framework, it uses embedded lightweight drawing API which allow to draw simple text and icons in glances menu.

### Installation

1. **Download and Install TouchGFX Designer**:
   - Create a free account on the [STMicroelectronics website](https://www.st.com).
   - Download the latest version of TouchGFX from the ST Developer Zone.
   - Install the TouchGFX Designer tool on your development machine (Windows, Linux, or macOS supported).

2. **Framework Integration**:
    - In the UNA SDK, TouchGFX is pre-integrated in example GUI projects (e.g., `Examples/Apps/<app>/Software/Apps/TouchGFX-GUI/`).
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
