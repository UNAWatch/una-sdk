# Repository Structure Inversion: SDK-Centric Architecture

## Overview

This document outlines the plan to invert the current repository structure from App-centric to SDK-centric, making the SDK the main repository with Apps as a submodule. This aligns with ESP-IDF's approach and provides better distribution, version management, and development workflow.

## Current Structure Analysis

### Current (App-Centric)

```
/una-watch-repo/
├── Apps/                    # Main app projects
│   ├── Alarm/
│   ├── Cycling/
│   ├── Running/
│   └── ...
├── SDK/                     # Framework (referenced externally)
│   ├── Libs/
│   ├── ThirdParty/
│   ├── Utilities/
│   ├── docs/
│   └── tools/
├── .git/                    # Git repo at root level
└── .gitignore
```

**Issues:**

- SDK is buried within Apps structure
- External UNA_SDK needed for CMake workflow
- Apps and SDK versioned separately
- CI/CD complexity for SDK testing

## Proposed Structure (SDK-Centric)

### New Repository Layout

```
/una-watch-sdk/             # Main SDK repository
├── Apps/                    # Git submodule with example apps
│   ├── Alarm/
│   ├── Cycling/
│   ├── Running/
│   └── ...
├── cmake/                   # SDK CMake modules (moved from SDK/)
│   ├── toolchain-arm-none-eabi.cmake
│   ├── una_project.cmake
│   └── una_components.cmake
├── tools/                   # SDK tools (moved from SDK/)
│   └── una.py
├── Libs/                    # SDK core libraries (moved from SDK/)
├── ThirdParty/              # Third-party dependencies
├── Utilities/               # Build utilities
├── docs/                    # SDK documentation
├── .git/                    # Main repository
├── README.md                # SDK README
└── .gitignore
```

## Migration Steps

### Phase 1: Repository Preparation

1. **Create New SDK Repository**

   ```bash
   # Create new repository for SDK
   mkdir una-watch-sdk
   cd una-watch-sdk
   git init
   ```

2. **Move SDK Contents to Root**

   ```bash
   # Copy SDK contents to new structure
   cp -r /path/to/old/SDK/* .
   mv SDK/cmake/* cmake/
   mv SDK/tools/* tools/
   mv SDK/Libs/* Libs/
   # ... continue for all directories
   ```

3. **Convert Apps to Submodule**

   ```bash
   # In new SDK repository
   git submodule add https://github.com/una-watch/apps.git Apps
   ```

### Phase 2: Code Updates

1. **Update CMake Paths**
   - Remove UNA_SDK dependency
   - Update all paths to be relative to repository root
   - Modify `watch_project.cmake` for new structure

2. **Update una.py Tool**
   - Remove UNA_SDK environment variable handling
   - Update paths to work within SDK repository
   - Modify app creation to work with submodule structure

3. **Update Documentation**
   - Rewrite quick-start guide for new structure
   - Update all path references
   - Add submodule setup instructions

### Phase 3: Build System Updates

1. **App CMakeLists.txt Updates**

   ```cmake
   # Apps no longer need UNA_SDK
   cmake_minimum_required(VERSION 3.21)
   include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/watch_project.cmake)

   project(my_app)
   # ... rest unchanged
   ```

### Phase 4: Testing and Validation

1. **Build System Testing**
   - Test CMake builds in new structure
   - Verify app creation and building
   - Test both CubeIDE and CMake workflows

2. **CI/CD Updates**
   - Update build pipelines for new structure
   - Test submodule handling
   - Verify release processes

## Benefits of SDK-Centric Structure

### 1. Distribution & Onboarding

```bash
# Single command gets everything
git clone --recursive https://github.com/una-watch/sdk.git
cd una-watch-sdk
una.py build-example Running
```

### 2. Version Management

- SDK v1.0.0 includes compatible Apps examples
- Clear version dependencies between SDK and examples
- Semantic versioning for SDK releases

### 3. Development Workflow

```bash
# Work on SDK and apps together
cd una-watch-sdk
# Edit SDK code
# Test against Apps examples
git add .
git commit -m "SDK improvements"
```

### 4. CI/CD Integration

```yaml
# GitHub Actions example
- name: Build SDK Examples
  run: |
    git submodule update --init --recursive
    ./tools/una.py build-all-examples
```

### 5. ESP-IDF Alignment

- Mirrors ESP-IDF's `examples/` directory structure
- Familiar pattern for embedded developers
- Consistent with industry standards

## Implementation Timeline

### Week 1: Repository Setup

- Create new SDK repository
- Move contents and establish new structure
- Set up submodules

### Week 2: Code Migration

- Update CMake files for new paths
- Modify una.py tool
- Update documentation

### Week 3: Testing & Validation

- Test all build workflows
- Update CI/CD pipelines
- Validate app compatibility

### Week 4: Release & Migration

- Release new SDK structure
- Update user documentation
- Provide migration guide for existing users

## Migration Guide for Users

### For Existing Users

```bash
# Old structure users
cd /path/to/old/repo
export UNA_SDK=/path/to/old/repo/SDK

# New structure
git clone --recursive https://github.com/una-watch/sdk.git
cd una-watch-sdk
# UNA_SDK no longer needed!
```

### For App Developers

```bash
# Old way
cp -r Apps/Running Apps/MyApp
export UNA_SDK=/path/to/sdk

# New way
cd una-watch-sdk/Apps
cp -r Running MyApp
cd MyApp
# Build directly - no environment variables needed
```

## Risk Assessment

### Low Risk

- CMake build system changes are backward compatible
- CubeIDE workflow remains unchanged
- App source code doesn't change

### Medium Risk

- Submodule management complexity
- Path changes in build scripts
- Documentation updates required

### Mitigation Strategies

- Comprehensive testing before release
- Provide migration scripts
- Maintain old structure documentation for transition period

## Conclusion

The SDK-centric repository structure provides significant benefits for distribution, development workflow, and maintenance while maintaining full backward compatibility. This aligns with ESP-IDF's proven approach and industry best practices for embedded SDKs.
