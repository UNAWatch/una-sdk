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

1. **Copy App Template**: Copy entire app directory structure from `Apps/<ann_name>` to `Apps/<your_app>`
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

### Future Architecture

1. **Separate SDK/Framework Directory**: Like ESP-IDF, have a central SDK with:
   - Core libraries (AppSystem, Kernel, etc.)
   - Existed Apps as templates with TouchGFX integration
   - Toolchain configurations tool
   - Build tools and common CMake logic

2. **Project Structure**: Apps reference SDK via environment variable (e.g., `UNA_SDK`)

3. **Dual Build Workflows**:
   - **CubeIDE Workflow**: Keep existing TouchGFX + CubeIDE projects unchanged, copy to start new one.
   - **CMake Workflow**: New ESP-IDF-like build system using extracted common CMake logic

4. **Build Tool**: Create `una.py` (similar to idf.py) that:
   - Sets up environment
   - Handles CMake configuration for the new workflow
   - Manages app packaging and merging

### TouchGFX Integration (Maintained)

Keep existing TouchGFX framework:

- **Compatibility**: Existing TouchGFX projects remain functional
- **Integration**: TouchGFX libraries and generated code stay in project structure
- **Build process**: Maintains separate GUI/Service binaries with app_packer and app_merging

### Example Project Structure

```
watch-sdk/           # Like ESP-IDF
├── cmake/            # Common CMake modules
│   ├── toolchain-arm-none-eabi.cmake
│   ├── watch_project.cmake  # Common project setup
│   └── watch_components.cmake  # Component definitions
├── tools/
│   └── una.py       # Build tool
└── docs/

apps/                 # Project workspace (any folder structure)
├── my_app/
│   ├── CMakeLists.txt  # Minimal app-specific CMakeLists.txt
│   ├── Libs/          # App-specific libraries
│   │   ├── Sources/
│   │   └── Header/
│   ├── TouchGFX-GUI/  # Existing TouchGFX project
│   ├── Resources/     # Icons, etc.
│   └── Output/        # Build output
└── another_app/      # Different structure possible
    ├── CMakeLists.txt
    └── src/
```

### CMakeLists.txt Example (App-Level)

```cmake
# App-level CMakeLists.txt (minimal)
cmake_minimum_required(VERSION 3.21)
include($ENV{UNA_SDK}/cmake/watch_project.cmake)

project(my_app)

# App configuration
set(APP_ID "A12E9F4C8B7D3A65")
set(APP_NAME "MyApp")
set(DEV_ID "UNA")

# Include common build logic
watch_build_app()
```

### una.py Tool (CMake Workflow)

Similar to idf.py, for the CMake-based workflow:

```bash
export UNA_SDK=/path/to/watch-sdk

# Create new app template
una.py create-app my_app

# Build (CMake-based)
cd my_app
una.py build

# Package and merge
una.py package
```

### Dual Workflow Benefits

1. **Backward Compatibility**: Existing CubeIDE + TouchGFX projects remain fully functional
2. **Choice of Tools**: Developers can use familiar CubeIDE or new CMake workflow
3. **ESP-IDF-like Experience**: CMake workflow provides the same confidence as ESP-IDF
4. **Environment-Based**: UNA_SDK enables flexible project placement
5. **Shared SDK**: Common build logic centralized in SDK, reducing duplication
6. **Maintainable**: Clear separation between SDK and app-specific code

### Repository Structure Analysis: SDK-Centric vs App-Centric

#### Current Structure (App-Centric)
```
repo/
├── Apps/          # Individual app projects
├── SDK/           # Framework (referenced via UNA_SDK)
└── .git/
```

#### Proposed Structure (SDK-Centric)
```
watch-sdk-repo/   # Main SDK repository
├── Apps/          # Submodule with example apps
├── cmake/         # SDK CMake modules
├── tools/         # SDK tools
├── Libs/          # SDK core libraries
├── docs/          # SDK documentation
└── .git/
```

#### SDK-Centric Advantages
1. **Distribution**: Clone SDK → get working examples immediately
2. **Version Management**: SDK versions include compatible app examples
3. **Development Workflow**: Work on SDK and examples together
4. **CI/CD**: Test SDK changes against example apps
5. **Documentation**: Examples are part of SDK repository
6. **ESP-IDF Alignment**: Mirrors ESP-IDF's structure with examples/

#### Implementation for SDK-Centric Structure
1. Move `SDK/` contents to repository root
2. Convert `Apps/` to git submodule
3. Update CMake paths (remove UNA_SDK complexity)
4. Modify `una.py` to work within SDK repository
5. Update documentation and examples

### Migration Path

1. Extract common CMake logic from Running-CMake/CMakeLists.txt to SDK cmake modules
2. Create UNA_SDK environment variable support
3. Develop una.py build tool for CMake workflow
4. Keep existing CubeIDE projects unchanged
5. Update documentation with both workflows
6. Test CMake workflow compatibility with existing app structures
7. **Consider repository restructuring**: Make SDK the main repo with Apps as submodule
