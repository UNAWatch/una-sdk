# Platform Overview

This document provides an overview of the Una-Watch platform's unique architecture and capabilities.

## Unique architecture

Unlike traditional smartwatch platforms, Una-Watch apps are:
- **Pure Machine Code**: Compiled ELF binaries executing directly in MCU memory
- **Position-Independent**: Apps abstracted from kernel using PIC (Position-Independent Code)
- **Shared libc**: Memory-efficient library sharing across all apps
- **Dual-Process**: Service (background) + GUI (interface) process architecture

## Platform Architecture

### Core Concepts

#### Pure Machine Code Execution
Una-Watch apps are not interpreted or virtualized. They are compiled ARM Cortex-M ELF binaries that execute directly in the MCU memory. This provides:
- **Native Performance**: Zero abstraction overhead.
- **Direct Hardware Access**: Apps can interact with peripherals at MCU speeds.
- **Efficiency**: Minimal memory and CPU footprint.

#### Position-Independent Code (PIC)
To allow apps to be loaded at any memory address without re-linking, the SDK uses Position-Independent Code.
- **Kernel Abstraction**: Apps are completely abstracted from the kernel's memory layout.
- **Dynamic Loading**: The kernel can load, run, and unload apps on demand.
- **Process Isolation**: Ensures that apps do not interfere with each other or the kernel.

#### Shared libc Integration
Una-Watch implements a shared libc architecture to save memory.
- **Common Base**: All apps share the same standard library implementation provided by the kernel.
- **Reduced Footprint**: Significant reduction in the size of each `.uapp` file.
- **C++ Support**: Full support for modern C++ features (strings, vectors, etc.) via the shared library.

#### Dual-Process Model
Every Una-Watch app consists of two distinct components:
1. **Service Process**: Handles background logic, sensors, and BLE.
2. **GUI Process**: Handles user interface and interaction.

#### IPC Mechanisms
Communication between processes is handled via a high-performance message-passing system.
- **Kernel Pools**: Messages are allocated from pre-defined kernel memory pools.
- **Type-Safe**: Messages are strongly typed using C++ structures.
- **Asynchronous**: Non-blocking message exchange for smooth UI performance.

#### Memory Management
- **Pool Allocation**: Prevents heap fragmentation in long-running applications.
- **RAII Patterns**: Automatic resource cleanup using modern C++ practices.
- **Protected Regions**: Memory regions are protected to prevent unauthorized access.

#### Power Management
- **Deep Sleep**: The kernel automatically enters low-power modes when idle.
- **Wake Sources**: Apps can define specific wake sources (timers, sensors, buttons).
- **Battery Optimization**: Adaptive sampling rates and event-driven design.

## Next Steps

For detailed SDK setup and build workflows, see [SDK Setup and Build Overview](sdk-setup.md).

For hands-on app development tutorials, see [Build Your First App](build-your-first-app.md).

For SDK APIs and interfaces, see [SDK Reference](sdk-overview.md) and [Architecture Deep Dive](architecture-deep-dive.md).
