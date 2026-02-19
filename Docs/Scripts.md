# Scripts

This document provides comprehensive analysis of the Python, Bash, and PowerShell scripts found in the directories .github/, Examples/.github/, Utilities/Scripts/ including environment tools.

## Script Usage Overview

| Script | Where/When/Who Calls It | Purpose |
|--------|-------------------------|---------|
| Utilities/Scripts/app_merging/app_merging.py | CMake post-build in una-app.cmake; manual developer use (Docs/sdk-setup.md, sdk-overview.md, app_merging/Readme.txt) | Merges app service/GUI binaries with icons/resources into a final app package during build or manually. |
| Utilities/Scripts/app_packer/app_packer.py | CMake post-build in una-app.cmake for service and GUI targets; referenced in docs (sdk-overview.md, development-workflow.md) | Packages ELF binaries with assets and version into .uapp containers in post-build steps. |
| Utilities/Scripts/png2abgr2222/png2abgr2222.py | No external callers found in searched files; self-documented examples for manual use | Converts input PNG images from RGBA to ABGR2222 byte arrays and generates C header files. |
| Utilities/Scripts/export-stm32-tools.ps1 | Manual Windows SDK setup (Docs/sdk-setup.md) | Exports STM32 tools during manual Windows environment setup. |
| Utilities/Scripts/build-cube/una-version.sh | Sourced by build-cube.sh, setup-environment.sh; called in cmake/una-app.cmake and GitHub Actions (tutorials-ci.yml, apps-ci.yml) | Initializes UNA version and build metadata environment variables for Cube builds. |
| Utilities/Scripts/build-cube/find-cube.sh | Sourced by setup-environment.sh; called in GitHub Actions (tutorials-ci.yml, apps-ci.yml) | Locates STM32 CubeIDE installation and sets environment variables. |
| Utilities/Scripts/build-cube/build-cube.sh | Called in GitHub Actions apps-ci.yml | Builds specified CubeIDE projects using prepared environment. |
| Utilities/Scripts/build-cube/setup-environment.sh | Used in `.vscode/tasks.json` | Prepares environment for CubeIDE builds by sourcing dependency scripts. |
| .github/scripts/generate-tutorials-app-list.py | GitHub Actions tutorials-ci.yml (runs to generate cmake_apps.json) | Generates list of tutorial apps in JSON format for CI workflows. |
| Examples/.github/scripts/generate-app-list.py | GitHub Actions apps-ci.yml (runs to list projects and filter by app) | Generates list of example app projects for CI build matrix. |

## Script: Utilities/Scripts/app_merging/app_merging.py

### Overview
This Python script merges service and GUI ELF binaries into a unified UAPP format, incorporating icon conversion, version management, and CRC integrity checks.

### Purpose
To create deployable application packages for the Una-Watch platform by combining service and GUI components with metadata, icons, and integrity verification.

### Functionality
The script performs comprehensive app packaging:
1. ELF binary validation and loading
2. Icon conversion from PNG to ABGR2222 format
3. Version parsing and metadata inclusion
4. Header construction with app identification
5. CRC calculation for integrity verification
6. Optional C header generation for embedded use

### How It Works
1. **Input Discovery**: Scans output directory for newest .srv and .gui files
2. **Icon Processing**: Converts 30x30 and 60x60 PNG icons to ABGR2222 format with 90-degree rotation
3. **Version Resolution**: Parses semantic versions and determines LibC compatibility
4. **Header Construction**: Builds extended header with app ID, version, and metadata
5. **Binary Merging**: Combines service, GUI, icons, and header into single blob
6. **Integrity Check**: Appends CRC32 for verification
7. **Output Generation**: Creates .uapp file and optional .h header

### Key Parameters
- `-name`: Application name for output file naming
- `-autostart`: Sets autostart flag in app metadata
- `-glance_capable`: Marks app as Glance-compatible
- `-type`: App type (Activity, Utility, Glance, Clockface)
- `-out`: Custom output directory
- `-header`: Generate C header file
- `-normal_icon`: Path to 60x60 icon PNG
- `-small_icon`: Path to 30x30 icon PNG
- `-appid`: 16-hex AppID
- `-appver`: App version in A.B.C format
- `-scripts`: Path to SDK Scripts directory

### Dependencies
- **Python 3** with PIL (Pillow) for image processing
- **zlib** for CRC32 calculation
- **pathlib** and **struct** for file and binary operations
- **argparse** for command-line parsing

### Usage Examples
1. **Basic app merging**:
   ```bash
   python app_merging.py -name MyApp -type Activity -out Output -normal_icon icon60.png -small_icon icon30.png -appid 0123456789ABCDEF -appver 1.0.0 -scripts /path/to/Scripts
   ```
   Creates MyApp_1.0.0.uapp with merged binaries and icons.

2. **With autostart and header**:
   ```bash
   python app_merging.py -name MyApp -autostart -header -type Utility -out Output -normal_icon icon60.png -small_icon icon30.png -appid 0123456789ABCDEF -appver 1.1.0 -scripts /path/to/Scripts
   ```
   Generates both .uapp and .h files with autostart capability.

### Error Handling
- Validates icon dimensions and formats
- Ensures required binaries exist in output directory
- Verifies CRC integrity of input files
- Checks app ID format and version syntax
- Validates linker script and LibC version parsing

### Integration Context
Core component of the app build pipeline, transforming compiled binaries into deployable packages. Used after separate service and GUI builds to create unified applications for the watch platform.

## Script: Utilities/Scripts/app_packer/app_packer.py

### Overview
This Python script converts ELF binaries into custom UAPP format packages, incorporating relocation handling, section padding, and optional C header generation for embedded deployment.

### Purpose
To package ARM ELF binaries into a self-contained format suitable for loading on embedded targets, with support for dynamic relocation and memory layout management.

### Functionality
The script provides comprehensive ELF processing:
1. ELF file parsing and section analysis
2. Relocation table generation for runtime linking
3. Section padding and memory layout calculation
4. Header construction with metadata
5. CRC integrity verification
6. Flexible output naming with custom extensions

### How It Works
1. **ELF Analysis**: Parses sections, identifies loadable segments, and calculates padding
2. **Relocation Processing**: Extracts ARM relocation information for runtime application
3. **Memory Layout**: Assembles sections with proper alignment and padding patterns
4. **Header Generation**: Creates UAPP header with size and relocation metadata
5. **Binary Assembly**: Combines header, relocation table, and section data
6. **Integrity Check**: Appends CRC32 for verification
7. **Output Creation**: Generates .uapp file (or custom extension) and optional C header

### Key Parameters
- `-e, --elf`: Path to input ELF file (required)
- `-o, --out`: Output directory (required)
- `-v, --version`: Version in major.minor format (default: 1.0)
- `-header`: Generate C header alongside binary
- `-ext`: Custom file extension (changes output filename to <elf_basename>.<ext>)

### Dependencies
- **Python 3** runtime environment
- **pyelftools**: For ELF file parsing and analysis
- **zlib**: For CRC32 calculation
- **argparse** and **struct**: For command-line and binary operations

### Usage Examples
1. **Standard UAPP generation**:
   ```bash
   python app_packer.py -e app.elf -o Output -v 1.2
   ```
   Creates userapp_1.2.uapp from app.elf

2. **With custom extension and header**:
   ```bash
   python app_packer.py -e firmware.elf -o Output -v 2.1 -ext bin -header
   ```
   Generates firmware.bin and firmware.h

### Error Handling
- Validates ELF file structure and section sizes
- Ensures sections are 4-byte aligned
- Checks for unhandled section types
- Reports missing or oversized sections
- Verifies version format

### Integration Context
Used in post-build processing to convert standard ELF outputs into custom embedded formats. Supports different deployment scenarios through flexible naming and header generation options.

## Script: Utilities/Scripts/png2abgr2222/png2abgr2222.py

### Overview
This Python script converts RGBA PNG images to ABGR2222 compressed format, generating C headers with embedded byte arrays and metadata macros for embedded graphics.

### Purpose
To provide compressed icon data for embedded systems, converting standard PNG files into a compact 2-bit-per-channel format suitable for limited-color displays.

### Functionality
The script offers comprehensive image conversion:
1. PNG loading and validation (square images required)
2. 90-degree rotation for proper orientation
3. Color reduction to 2 bits per channel (ABGR2222)
4. C header generation with arrays and macros
5. Support for multiple input files in single header

### How It Works
1. **Image Loading**: Opens PNG files and validates square dimensions
2. **Transformation**: Rotates 90 degrees clockwise and converts to ABGR2222
3. **Color Processing**: Reduces each RGBA channel to 2 bits, packs into single byte
4. **Symbol Generation**: Creates valid C identifiers from filenames
5. **Header Creation**: Generates pragma-once header with arrays and size macros

### Key Parameters
- `--inputs`: One or more input PNG files (required)
- `-o, --out`: Output header file path (default: derived from first input)
- `--name`: Base symbol name for single-input files
- `--prefix`: Optional prefix for generated symbols

### Dependencies
- **Python 3** with PIL (Pillow) library
- **pathlib** and **argparse** for file and argument handling

### Usage Examples
1. **Single icon conversion**:
   ```bash
   python png2abgr2222.py --inputs icon.png -o icon.h
   ```
   Creates icon.h with ABGR2222 data and macros

2. **Multiple icons with custom prefix**:
   ```bash
   python png2abgr2222.py --inputs icon1.png icon2.png -o icons.h --prefix APP_
   ```
   Generates icons.h with APP_ICON1 and APP_ICON2 symbols

3. **Custom symbol name**:
   ```bash
   python png2abgr2222.py --inputs battery.png --name BATTERY_ICON -o battery.h
   ```

### Error Handling
- Validates input files exist and are PNG format
- Ensures images are square (width == height)
- Checks for valid symbol name generation
- Reports conversion progress and output location

### Integration Context
Essential for embedded GUI development, converting designer-provided PNG assets into compressed formats suitable for watch displays. Integrates with build systems to generate headers for static icon inclusion.

## Script: Utilities/Scripts/export-stm32-tools.ps1

### Overview
This PowerShell script automates the detection and configuration of essential STM32 development tools on Windows systems. It locates common toolchains and utilities (arm-none-eabi-gcc, make, cmake, python), adds their directories to the user PATH permanently, and sets the UNA_SDK environment variable pointing to the SDK root directory.

### Purpose
The primary purpose is to simplify environment setup for Una-Watch SDK development on Windows. By automatically discovering installed tools in standard locations and updating the PATH, it eliminates manual configuration steps required for building STM32-based applications using CMake, Make, and the ARM GCC toolchain.

### Functionality
The script provides:
1. **Tool Discovery**: Searches PATH first, then common installation directories for required executables
2. **PATH Management**: Identifies unique tool directories not already in user PATH and appends them permanently
3. **Environment Setup**: Permanently sets UNA_SDK environment variable with forward-slashed SDK root path
4. **Dry-Run Preview**: Optional mode to preview changes without applying them

### How It Works
1. **SDK Root Calculation**: Determines SDK root by navigating three parent directories from script location using `Split-Path`
2. **Tool Definition**: Maps tool names to descriptions (arm-none-eabi-gcc → STM32 Toolchain GCC, etc.)
3. **Search Strategy**: 
   - Attempts `Get-Command` for each tool
   - Falls back to recursive search in base directories: `%USERPROFILE%\AppData\Local\Programs`, `C:\Program Files (x86)`, `C:\Program Files`, `C:\ST`
4. **Path Deduplication**: Collects unique directories, filters those absent from current user PATH
5. **Execution Modes**:
   - **DryRun**: Outputs preview of paths to add and new PATH/UNA_SDK values
   - **Normal**: Updates user PATH via `[Environment]::SetEnvironmentVariable` and sets UNA_SDK with forward slashes

### Key Parameters
- `-DryRun`: Switch enabling preview mode; displays changes without modification

### Dependencies
- **PowerShell**: Native Windows PowerShell environment
- **Target Tools**: 
  - arm-none-eabi-gcc (STM32 ARM GCC toolchain)
  - make (GNU Make)
  - cmake (CMake build system)
  - python (Python interpreter)
- **Installation Locations**: Tools must be in PATH or standard directories like Program Files or C:\ST

### Usage Examples
1. **Preview changes (dry run)**:
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File "Utilities/Scripts/export-stm32-tools.ps1" -DryRun
   ```
   Output shows paths to add and UNA_SDK value.

2. **Apply environment setup**:
   ```powershell
   powershell.exe -ExecutionPolicy Bypass -File "Utilities/Scripts/export-stm32-tools.ps1"
   ```
   Permanently updates PATH and sets UNA_SDK.

3. **Run from PowerShell directly**:
   ```powershell
   & "Utilities/Scripts/export-stm32-tools.ps1"
   ```

### Error Handling
- Uses `try-catch` blocks around `Get-Command` to handle missing tools gracefully
- Continues processing even if individual tools are not found, warning via `Write-Host`
- No script termination on missing tools; focuses on configuring available ones
- Silently handles inaccessible search directories with `-ErrorAction SilentlyContinue`

### Integration Context
Designed as a one-time setup script for Windows developers working with the Una-Watch SDK. It prepares the environment for cross-platform build tools (CMake, Make) and STM32-specific toolchain, enabling seamless building of examples and applications. Typically run after tool installation (STM32CubeIDE, MSYS2, etc.) and before using CMake-based workflows or STM32CubeIDE projects.

## Script: Utilities/Scripts/build-cube/una-version.sh

### Overview
This comprehensive Bash script determines the build version for Una-Watch projects based on Git repository state, supporting semantic versioning, tags, and dirty state detection.

### Purpose
To provide accurate version information for builds, enabling proper versioning of binaries and documentation of build provenance in CI/CD environments.

### Functionality
The script implements sophisticated version detection:
1. Git repository state analysis
2. Semantic version parsing from tags
3. Dirty state detection for uncommitted changes
4. Safe directory configuration for Docker environments
5. Fallback to default version when Git is unavailable

### How It Works
1. **Git Availability Check**: Verifies git command availability, falls back to default version if unavailable
2. **Build Version Override**: Uses existing BUILD_VERSION if already set
3. **Directory Setup**: Changes to specified or current directory for Git operations
4. **Safe Directory Config**: Configures Git safe.directory for containerized environments
5. **Repository Analysis**: Determines if in a Git repository, extracts version from tags or commit hashes
6. **Version Construction**: Builds version string based on tags, commits, and dirty state

### Key Parameters
- `$1`: Optional directory path to use for version detection (defaults to current directory)

### Dependencies
- **Git**: Primary version detection mechanism
- **Standard Bash utilities**: For string manipulation and file operations

### Usage Examples
1. **Get version for current directory**:
   ```bash
   ./una-version.sh
   echo $BUILD_VERSION
   ```
   Output: Version string like "1.2.3", "1.2.3-abc1234", or "1.2.3-abc1234-dirty"

2. **Get version for specific directory**:
   ```bash
   ./una-version.sh /path/to/repo
   ```

### Error Handling
- Gracefully handles non-Git environments with default version
- Manages Git dubious ownership issues in Docker
- Logs detailed information about version detection process
- Exits early if version already set or Git unavailable

### Integration Context
Critical component of build systems, ensuring that all built artifacts include accurate version metadata. Supports CI/CD by providing consistent versioning across different build environments.

## Script: Utilities/Scripts/build-cube/find-cube.sh

### Overview
This Bash script locates and configures the STM32CubeIDE environment, including setting up necessary paths and installing system dependencies for headless builds.

### Purpose
To automate the detection and setup of STM32CubeIDE installation and required build tools in headless Linux environments, ensuring consistent build environments across different systems.

### Functionality
The script performs:
1. Installation of system dependencies (Python, Git, PIL)
2. Detection of STM32CubeIDE installation directory
3. Verification of arm-none-eabi-objcopy tool availability
4. Environment variable setup for build tools

### How It Works
1. **Dependency Installation**: Uses apt to install required packages for build processes
2. **Path Configuration**: Sets PATH to include tool directories
3. **IDE Detection**: Scans /opt/st/ for STM32CubeIDE installations, selecting the latest version
4. **Tool Verification**: Checks for presence and executability of arm-none-eabi-objcopy
5. **Environment Setup**: Exports PATH and reports tool locations

### Key Parameters
- No command-line parameters. All operations are automatic based on system state.

### Dependencies
- **apt package manager**: For system dependency installation
- **STM32CubeIDE**: Must be installed in /opt/st/stm32cubeide_*/ directory structure
- **System packages**: python3, python3-bincopy, python3-git, python3-pil, python-is-python3

### Usage Examples
1. **Setup build environment**:
   ```bash
   source ./find-cube.sh
   ```
   This installs dependencies, locates CubeIDE, and sets up PATH for subsequent build operations.

### Error Handling
- Exits with error if STM32CubeIDE is not found in expected locations
- Exits with error if arm-none-eabi-objcopy is missing
- Lists available STM32CubeIDE installations for debugging if not found

### Integration Context
Used as part of the build setup process in CI/CD pipelines. The script ensures that all necessary tools are available and properly configured before attempting CubeIDE builds.

## Script: Utilities/Scripts/build-cube/build-cube.sh

### Overview
This Bash script provides automated headless building of STM32CubeIDE projects with integrated version management and preference patching capabilities.

### Purpose
To enable reliable, automated builds of STM32CubeIDE projects in headless environments (such as CI/CD systems) while automatically injecting build version information.

### Functionality
The script handles:
1. Version detection from Git repository state
2. Patching of Eclipse project preferences with build version
3. Execution of headless STM32CubeIDE build process
4. Proper error handling and status reporting

### How It Works
1. **Version Sourcing**: Sources the `una-version.sh` script to determine BUILD_VERSION
2. **Path Derivation**: Extracts project name from IMPORT_PATH and sets up build data directory
3. **Preference Patching**: Locates and modifies the project's Eclipse preferences file to inject __BUILD_VERSION__
4. **Headless Build**: Executes STM32CubeIDE with headless build parameters
5. **Status Reporting**: Captures and reports build exit codes

### Key Parameters
- `IMPORT_PATH`: Required path to the STM32CubeIDE project directory to be built

### Dependencies
- **STM32CubeIDE**: Must be installed and accessible via PATH or /opt/st/
- **Git**: For version detection and repository state checking
- **una-version.sh**: Companion script for version determination
- **Eclipse project structure**: Expects standard CubeIDE project layout with .settings/org.eclipse.cdt.core.prefs

### Usage Examples
1. **Build a specific CubeIDE project**:
   ```bash
   ./build-cube.sh /path/to/MyProject
   ```
   This will build the project, patching version information and storing build artifacts.

2. **Integration in CI/CD**:
   ```bash
   export UNA_WORKSPACE=/path/to/sdk
   ./build-cube.sh $UNA_WORKSPACE/Examples/Apps/MyApp/Software/Apps/MyApp-CubeIDE
   ```

### Error Handling
- Exits with error code if IMPORT_PATH is not provided
- Continues without version patching if BUILD_VERSION is not set
- Reports build failure exit codes from STM32CubeIDE
- Warns if preferences file is not found but continues build

### Integration Context
Designed for use in automated build environments. The version patching ensures that built binaries include proper version metadata, while the headless build capability allows integration with continuous integration systems that don't have graphical interfaces.

## Script: Utilities/Scripts/build-cube/setup-environment.sh

### Overview
This Bash script serves as a high-level setup script that coordinates the environment preparation for STM32CubeIDE builds by sourcing dependent setup scripts.

### Purpose
To provide a single entry point for setting up the complete STM32CubeIDE build environment, including tool detection and version management.

### Functionality
The script orchestrates the setup process by:
1. Displaying setup progress messages
2. Updating PATH with build-cube script directory
3. Sourcing the find-cube.sh script for tool detection
4. Sourcing the una-version.sh script with optional version parameter

### How It Works
1. **Path Setup**: Adds the build-cube directory to PATH for script accessibility
2. **Tool Detection**: Sources find-cube.sh to locate and configure STM32CubeIDE
3. **Version Detection**: Sources una-version.sh to determine build version, optionally passing a directory parameter

### Key Parameters
- `$1`: Optional directory parameter passed to una-version.sh for version detection context

### Dependencies
- **find-cube.sh**: For STM32CubeIDE tool detection and setup
- **una-version.sh**: For build version determination
- **UNA_SDK environment variable**: Expected to point to SDK root directory

### Usage Examples
1. **Setup environment for current directory**:
   ```bash
   ./setup-environment.sh
   ```

2. **Setup with specific directory for version detection**:
   ```bash
   ./setup-environment.sh /path/to/project
   ```

### Error Handling
- Inherits error handling from sourced scripts
- Displays setup progress and completion messages

### Integration Context
Acts as a coordinator script in build pipelines, ensuring all prerequisites are met before proceeding with STM32CubeIDE builds. Commonly used in automated build environments.

## Script: .github/scripts/generate-tutorials-app-list.py

### Overview
This Python script is designed to generate a list of CMake-based application projects within the tutorials directory structure. It traverses the `Docs/Tutorials` directory to identify and catalog CMake projects, while providing flexibility to exclude certain applications and generate project-specific lists.

### Purpose
The primary purpose of this script is to automate the discovery and listing of tutorial applications that use CMake build systems. It supports CI/CD workflows by providing a programmatic way to enumerate buildable tutorial projects, with built-in exclusion capabilities for managing which applications should be included in builds or deployments.

### Functionality
The script operates in two main modes:
1. **General mode**: Scans the entire tutorials directory and outputs a JSON object containing all discovered CMake applications
2. **App-specific mode**: When called with the `--app` flag, it lists the specific CMake project directories for a given application name

### How It Works
1. **Directory Traversal**: Uses `os.walk()` to recursively traverse the `Docs/Tutorials` directory
2. **Directory Filtering**: Applies exclusion rules to skip hidden directories, build artifacts, and other irrelevant folders
3. **Project Discovery**: Identifies directories ending with `-CMake` as valid project directories
4. **App Name Extraction**: Derives application names from the directory structure relative to the tutorials base path
5. **Exclusion Handling**: Reads the `APPS_EXCLUDED` environment variable to filter out specified applications
6. **Output Generation**: Produces either a newline-separated list of project paths (app-specific mode) or JSON output (general mode)

### Key Parameters
- `--app <app_name>`: Optional command-line argument to generate project list for a specific application
- `APPS_EXCLUDED`: Environment variable containing newline-separated list of application names to exclude

### Dependencies
- **Python 3**: The script uses Python 3 features and syntax
- **Standard Library Modules**:
  - `os`: For file system operations and path manipulation
  - `json`: For generating JSON output in general mode
  - `sys`: For command-line argument processing and error output

### Usage Examples
1. **Generate complete app list**:
   ```bash
   python3 .github/scripts/generate-tutorials-app-list.py
   ```
   Output:
   ```json
   {"cmake_apps": ["Buttons", "Display", "Touch"]}
   ```

2. **Generate projects for specific app**:
   ```bash
   python3 .github/scripts/generate-tutorials-app-list.py --app Buttons
   ```
   Output:
   ```
   /path/to/Docs/Tutorials/Buttons/Software/Buttons-CMake
   ```

3. **Exclude specific apps via environment**:
   ```bash
   APPS_EXCLUDED="OldTutorial
   DeprecatedApp" python3 .github/scripts/generate-tutorials-app-list.py
   ```

### Error Handling
- Validates the presence of app name when `--app` flag is used
- Gracefully handles cases where excluded apps are requested
- Uses standard error output for error messages

### Integration Context
This script appears to be part of a GitHub Actions workflow system, likely used in CI/CD pipelines to dynamically determine which tutorial projects need to be built or tested. The exclusion mechanism allows for flexible project management without code changes.

## Script: Examples/.github/scripts/generate-app-list.py

### Overview
This Python script serves as a discovery tool for application projects in the examples directory, supporting both CMake and CubeIDE build systems. It provides comprehensive project enumeration capabilities with exclusion support, designed for automated build and deployment workflows.

### Purpose
The script facilitates automated discovery and categorization of example applications across different IDE and build system configurations. It enables CI/CD systems to programmatically identify buildable projects while maintaining flexibility through exclusion mechanisms.

### Functionality
The script operates in two primary modes:
1. **General mode**: Performs a comprehensive scan of the examples directory, categorizing applications by build system (CMake vs CubeIDE)
2. **App-specific mode**: Generates a list of project directories for a specific application across all supported build systems

### How It Works
1. **Directory Scanning**: Recursively walks through the `Examples/Apps` directory using `os.walk()`
2. **Exclusion Filtering**: Applies predefined rules to skip irrelevant directories (hidden, generated, build artifacts)
3. **Project Classification**: Identifies project directories by examining naming patterns:
   - `-CMake` suffix indicates CMake-based projects
   - `-CubeIDE` suffix indicates STM32CubeIDE projects
4. **Application Grouping**: Extracts application names from the directory hierarchy relative to the Apps base path
5. **Exclusion Processing**: Filters applications based on the `APPS_EXCLUDED` environment variable
6. **Output Formatting**: Produces either newline-separated project paths (app-specific mode) or structured JSON (general mode)

### Key Parameters
- `--app <app_name>`: Command-line flag to generate project list for a specific application
- `APPS_EXCLUDED`: Environment variable with newline-separated application names to exclude from processing

### Dependencies
- **Python 3**: Requires Python 3 runtime environment
- **Standard Library Modules**:
  - `os`: Handles file system navigation and path operations
  - `json`: Enables JSON output generation for structured data
  - `sys`: Manages command-line arguments and error reporting

### Usage Examples
1. **Generate complete categorized app list**:
   ```bash
   python3 Examples/.github/scripts/generate-app-list.py
   ```
   Output:
   ```json
   {
     "cmake_apps": ["Alarm", "Cycling", "Fitness"],
     "cubeide_apps": ["Alarm", "Cycling"]
   }
   ```

2. **List projects for specific application**:
   ```bash
   python3 Examples/.github/scripts/generate-app-list.py --app Alarm
   ```
   Output:
   ```
   /path/to/Examples/Apps/Alarm/Software/Apps/Alarm-CMake
   /path/to/Examples/Apps/Alarm/Software/Apps/AlarmGUI-CubeIDE
   /path/to/Examples/Apps/Alarm/Software/Apps/AlarmService-CubeIDE
   ```

3. **Exclude applications during processing**:
   ```bash
   APPS_EXCLUDED="BetaApp
   Prototype" python3 Examples/.github/scripts/generate-app-list.py
   ```

### Error Handling
- Enforces required app name argument when using `--app` flag
- Silently handles requests for excluded applications by producing no output
- Directs error messages to standard error stream

### Integration Context
This script is likely a component of GitHub Actions workflows for example project CI/CD. It supports multi-platform build automation by distinguishing between CMake (cross-platform) and CubeIDE (STM32-specific) projects, enabling targeted build strategies for different development environments.