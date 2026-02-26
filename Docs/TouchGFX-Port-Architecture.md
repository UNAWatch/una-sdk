# TouchGFX Port Architecture

## Table of Contents

1. [Overview](#overview)
2. [Definitions and Terminology](#definitions-and-terminology)
3. [Prerequisites and Dependencies](#prerequisites-and-dependencies)
4. [Structural Architecture](#structural-architecture)
5. [API Structure](#api-structure)
6. [Behavioral Architecture](#behavioral-architecture)
   - [Initialization Flow](#initialization-flow)
   - [Rendering Flow](#rendering-flow)
   - [Input Handling](#input-handling)
   - [Lifecycle Management](#lifecycle-management)
   - [Data Flows](#data-flows)
7. [Configuration and Setup](#configuration-and-setup)
8. [Design Patterns](#design-patterns)
9. [Error Handling and Debugging](#error-handling-and-debugging)
10. [Extensibility and Customization Guide](#extensibility-and-customization-guide)
11. [Best Practices and Performance Tips](#best-practices-and-performance-tips)
12. [Overall Architecture Characteristics](#overall-architecture-characteristics)
13. [UNA-Specific System Integration Points](#una-specific-system-integration-points)
14. [References and Further Reading](#references-and-further-reading)
15. [FAQ](#faq)

## Overview

The TouchGFX port implementation for the UNA SDK provides a comprehensive hardware abstraction layer (HAL) that seamlessly integrates the TouchGFX GUI framework into the UNA platform. This port empowers developers to create sophisticated graphical user interfaces for UNA-based applications, leveraging TouchGFX's extensive widget library and rendering capabilities.

The implementation comprises custom HAL classes that extend TouchGFX-generated code, a robust command processor for kernel integration, and stub implementations for hardware interfaces not currently utilized in the UNA platform configuration. The port supports a 240x240 pixel display with 8-bit color depth using ABGR2222 format, software-based rendering, and button-based input handling through the UNA kernel messaging system.

Key architectural principles include:
- **Modular Design**: Clean separation between TouchGFX framework and UNA platform specifics
- **Message-Based Communication**: Asynchronous integration with the UNA kernel
- **Extensibility**: Support for future hardware additions and customizations
- **Performance Optimization**: Efficient resource usage within platform constraints

## Definitions and Terminology

- **ABGR2222**: A 8-bit color format where each pixel uses 2 bits per channel (Alpha, Blue, Green, Red) for compact color representation
- **UNA Kernel**: The core operating system component of the UNA platform, responsible for task scheduling, messaging, and hardware abstraction
- **Stub Implementation**: A minimal, non-functional implementation of an interface, used when the corresponding hardware or feature is not available
- **Frame Buffer**: A memory region storing pixel data for display rendering
- **VSync (Vertical Synchronization)**: Synchronization signal ensuring display updates occur at the correct refresh rate
- **HAL (Hardware Abstraction Layer)**: Software layer that provides a consistent interface to hardware components
- **Frontend Heap**: Memory allocation system used by TouchGFX for dynamic GUI elements
- **Message Queue**: A fixed-size buffer (capacity: 10 messages) for storing kernel messages in the TouchGFX command processor

## Prerequisites and Dependencies

### Software Dependencies
- TouchGFX Designer 4.23.0+ or compatible version
- UNA SDK with kernel messaging system
- C++17 compatible compiler
- STM32CubeMX for project generation (optional)

### Hardware Requirements
- 240x240 pixel display supporting 8-bit color depth
- At least 57.6 KB RAM for frame buffer (240×240×1 byte/pixel)
- Button input interface (4 buttons: SW1-SW4)
- UART or similar communication interface for kernel messaging

### Development Environment
- Windows/Linux development host
- STM32CubeIDE or compatible IDE
- TouchGFX Environment setup

## Structural Architecture

### Class Hierarchy

```mermaid
graph TD
    A[TouchGFXGeneratedHAL] --> B[TouchGFXHAL]
    B --> C[HWButtonController]

    D[TouchGFXCommandProcessor] --> E[FixedQueue<MessageBase*, 10>]
    D --> F[Kernel Reference]

    G[OSWrappers] --> H[TouchGFXCommandProcessor]
    I[STM32DMA] --> J[Stub Implementation]
    K[STM32TouchController] --> L[Stub Implementation]
    M[TouchGFXGPIO] --> N[Stub Implementation]
```

### Key Classes and Interfaces

#### TouchGFXHAL
- **Location**: `Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp`, `Libs/Header/SDK/Port/TouchGFX/TouchGFXHAL.hpp`
- **Purpose**: Main hardware abstraction layer extending TouchGFXGeneratedHAL
- **Key Responsibilities**:
  - Frame buffer management and allocation
  - Button controller integration
  - Display initialization and configuration
  - Interrupt handling (delegated to generated HAL)
  - Frame synchronization and flushing

#### TouchGFXCommandProcessor
- **Location**: `Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp`, `Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp`
- **Purpose**: Central command processor and lifecycle manager (singleton pattern)
- **Key Responsibilities**:
  - Kernel message processing and routing
  - GUI lifecycle management (start/stop/resume/suspend)
  - Button event handling and translation
  - Custom message routing
  - Frame synchronization via VSync
  - Display update message sending

#### HWButtonController
- **Location**: `Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp` (nested class)
- **Purpose**: Button input controller for TouchGFX
- **Key Responsibilities**:
  - Sampling button states from command processor
  - Translating UNA button events to TouchGFX key codes
  - Polled input handling

#### Generated and Stub Classes
- **TouchGFXGeneratedHAL**: Base HAL implementation generated by TouchGFX tools
- **OSWrappers**: Operating system abstraction layer bridging TouchGFX to UNA kernel
- **STM32DMA**: DMA interface (stub implementation - not supported)
- **STM32TouchController**: Touch input controller (stub implementation - not implemented)
- **TouchGFXGPIO**: GPIO interface (stub implementation - not utilized)

### Data Structures
- **Frame Buffer**: Static `uint8_t sFrameBuffer[57600]` (240×240×1 byte/pixel)
- **Active Buffer Pointer**: `uint8_t* spActiveBuffer`
- **Flush Request Flag**: `bool sFlushBufferReq`
- **Button State**: `uint8_t mLastButtonCode`
- **Message Queue**: `FixedQueue<MessageBase*, 10> mUserQueue` for custom messages
- **Kernel Reference**: Direct access to UNA kernel for messaging

## API Structure

### Public Interfaces

#### TouchGFXHAL
```cpp
class TouchGFXHAL : public TouchGFXGeneratedHAL {
public:
    TouchGFXHAL(DMA_Interface& dma, LCD& display, TouchController& tc, uint16_t width, uint16_t height);
    virtual void initialize();
    virtual void disableInterrupts();
    virtual void enableInterrupts();
    virtual void configureInterrupts();
    virtual void enableLCDControllerInterrupt();
    virtual bool beginFrame();
    virtual void endFrame();
    virtual void flushFrameBuffer();
    virtual void flushFrameBuffer(const Rect& rect);
    virtual bool blockCopy(void* dest, const void* src, uint32_t numBytes);

protected:
    virtual uint16_t* getTFTFrameBuffer() const;
    virtual void setTFTFrameBuffer(uint16_t* adr);
};
```

#### TouchGFXCommandProcessor
```cpp
class TouchGFXCommandProcessor {
public:
    static TouchGFXCommandProcessor& GetInstance();
    void setAppLifeCycleCallback(IGuiLifeCycleCallback* cb);
    void setCustomMessageHandler(ICustomMessageHandler* h);
    bool waitForFrameTick();
    bool getKeySample(uint8_t& key);
    void writeDisplayFrameBuffer(const uint8_t* data);
    void callCustomMessageHandler();
};
```

### Configuration Functions
- `touchgfx_init()`: Initializes TouchGFX framework, registers resources
- `touchgfx_taskEntry()`: Main GUI task entry point (infinite loop)
- `touchgfx_components_init()`: Component initialization (currently empty)

### Key Relationships
- **TouchGFXHAL** extends **TouchGFXGeneratedHAL** for UNA-specific customizations
- **TouchGFXCommandProcessor** acts as a singleton managing global GUI state
- **HWButtonController** polls **TouchGFXCommandProcessor** for input
- **OSWrappers** delegates VSync waiting to **TouchGFXCommandProcessor**
- Generated classes provide minimal base implementations with stub functionality

## Behavioral Architecture

### Initialization Flow

```mermaid
sequenceDiagram
    participant Kernel
    participant App
    participant TouchGFX
    participant HAL

    Kernel->>App: Launch GUI Application
    App->>TouchGFX: touchgfx_init()
    TouchGFX->>TouchGFX: Register resources (bitmaps, fonts, texts)
    TouchGFX->>HAL: TouchGFXHAL::initialize()
    HAL->>HAL: Parent HAL initialization
    HAL->>HAL: Enable frame rate compensation
    HAL->>HAL: Register event listener
    HAL->>HAL: Allocate frame buffer (static 240x240)
    HAL->>HAL: Initialize button controller
    TouchGFX->>App: Initialization complete
    App->>TouchGFX: touchgfx_taskEntry() - infinite loop
```

1. **Application Startup**: UNA kernel launches the GUI application process
2. **TouchGFX Initialization**:
   - `touchgfx_init()` configures the framework
   - Bitmap database, text resources, and font providers are registered
   - Frontend heap is initialized for dynamic allocations
   - HAL is initialized via `TouchGFXHAL::initialize()`
3. **HAL Setup**:
   - Parent generated HAL initialization
   - Frame rate compensation enabled for smooth rendering
   - Event listener registered with TouchGFX Application
   - Static frame buffer allocated (240×240×1 byte)
   - Button controller initialized and configured

### Rendering Flow

```mermaid
sequenceDiagram
    participant TouchGFX
    participant HAL
    participant CmdProc
    participant Kernel
    participant Display

    TouchGFX->>HAL: beginFrame()
    TouchGFX->>TouchGFX: Widget rendering to frame buffer
    TouchGFX->>HAL: flushFrameBuffer()
    HAL->>HAL: Set flush request flag
    HAL->>HAL: Call generated HAL flush
    TouchGFX->>HAL: endFrame()
    HAL->>CmdProc: writeDisplayFrameBuffer() if flush requested
    CmdProc->>Kernel: Send RequestDisplayUpdate message
    Kernel->>Display: Update display hardware
```

1. **Frame Start**: `HAL::beginFrame()` prepares for rendering
2. **Drawing Operations**: TouchGFX widgets render directly to the frame buffer
3. **Frame Flush**: `TouchGFXHAL::flushFrameBuffer()` is called
   - Sets internal flush request flag
   - Delegates to generated HAL for framework notification
4. **Display Update**: Command processor sends display update message to kernel
5. **Frame End**: `HAL::endFrame()` completes the frame cycle

### Input Handling

```mermaid
sequenceDiagram
    participant HW
    participant Kernel
    participant CmdProc
    participant ButtonCtrl
    participant TouchGFX

    HW->>Kernel: Button press event
    Kernel->>CmdProc: EVENT_BUTTON message
    CmdProc->>CmdProc: Map button ID to key code
    CmdProc->>CmdProc: Store mLastButtonCode
    TouchGFX->>ButtonCtrl: sample() call
    ButtonCtrl->>CmdProc: getKeySample()
    CmdProc->>ButtonCtrl: Return key code
    ButtonCtrl->>TouchGFX: Key event
```

1. **Button Events**: Hardware button presses generate kernel `EVENT_BUTTON` messages
2. **Event Processing**: Command processor receives and processes button events
3. **Key Mapping**: Button IDs are translated to TouchGFX key codes:
   - SW1 → '1'
   - SW2 → '3'
   - SW3 → '2'
   - SW4 → '4'
4. **Sampling**: `HWButtonController::sample()` retrieves the last button code via `getKeySample()`

### Lifecycle Management

```mermaid
stateDiagram-v2
    [*] --> Starting
    Starting --> Running: onStart() callback
    Running --> Suspended: COMMAND_APP_GUI_SUSPEND
    Suspended --> Running: COMMAND_APP_GUI_RESUME
    Running --> [*]: COMMAND_APP_STOP
    note right of Running : EVENT_GUI_TICK triggers onFrame()
```

1. **Start**: `onStart()` callback invoked on first frame tick
2. **Resume/Suspend**: Handled via kernel commands with corresponding callbacks
3. **Stop**: `onStop()` callback executed, application terminates
4. **Frame Ticks**: `EVENT_GUI_TICK` messages trigger `onFrame()` callbacks for rendering

### Data Flows

#### Rendering Data Flow
```
TouchGFX Widgets → Frame Buffer (uint8_t[57600]) → flushFrameBuffer() → Display Update Message → Kernel → Display Hardware
```

#### Input Data Flow
```
Hardware Buttons → Kernel → EVENT_BUTTON Message → TouchGFXCommandProcessor.mLastButtonCode → HWButtonController → TouchGFX Key Events
```

#### Lifecycle Data Flow
```
Kernel Commands (START/STOP/RESUME/SUSPEND) → TouchGFXCommandProcessor → Lifecycle Callbacks → TouchGFX Application
```

#### VSync Synchronization
```
Kernel → EVENT_GUI_TICK Message → TouchGFXCommandProcessor.waitForFrameTick() → OSWrappers.waitForVSync() → TouchGFX Frame Processing
```

## Configuration and Setup

### Basic Setup
1. Include TouchGFX port headers in your application
2. Initialize the command processor singleton
3. Set lifecycle callbacks if needed
4. Call TouchGFX initialization functions
5. Start the GUI task

### Code Example: Basic Application Setup
```cpp
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

// In your application initialization
void setupGUI() {
    auto& cmdProc = SDK::TouchGFXCommandProcessor::GetInstance();

    // Set lifecycle callbacks (optional)
    cmdProc.setAppLifeCycleCallback(&myLifeCycleCallback);

    // Initialize TouchGFX
    touchgfx_init();

    // Start GUI task (never returns)
    touchgfx_taskEntry();
}
```

### Advanced Configuration
- **Custom Message Handling**: Implement `ICustomMessageHandler` for application-specific messages
- **Button Mapping**: Modify `TouchGFXCommandProcessor::handleEvent()` for custom key mappings
- **Display Parameters**: Adjust frame buffer size in `TouchGFXHAL.cpp` if display resolution changes

## Design Patterns

### Singleton Pattern
- **TouchGFXCommandProcessor**: Ensures single instance managing global GUI state
- Provides centralized access to command processing and lifecycle management

### Adapter Pattern
- **TouchGFXHAL**: Adapts TouchGFX HAL interface to UNA platform specifics
- **HWButtonController**: Translates UNA button events to TouchGFX input system

### Template Method Pattern
- **TouchGFXHAL** overrides specific methods while delegating others to generated base class
- Enables customization of key behaviors while maintaining framework compatibility

### Observer Pattern
- **Event Listeners**: HAL registers with TouchGFX Application for framework events
- **Lifecycle Callbacks**: Command processor notifies registered callbacks of state changes

### Bridge Pattern
- **OSWrappers**: Bridges TouchGFX OS requirements to UNA kernel primitives
- Separates platform-specific OS operations from framework logic

## Error Handling and Debugging

### Common Issues and Troubleshooting

| Issue | Symptoms | Solution |
|-------|----------|----------|
| GUI not starting | Application hangs on startup | Check kernel messaging setup, verify TouchGFX resources |
| Display not updating | Screen remains blank | Verify frame buffer allocation, check display update messages |
| Button input not working | Buttons don't respond | Check button mapping in `handleEvent()`, verify kernel button events |
| Memory allocation failures | Application crashes | Ensure sufficient heap space for TouchGFX allocations |
| Frame rate issues | Jerky animation | Check VSync timing, verify `EVENT_GUI_TICK` frequency |

### Debugging Techniques
- **Logging**: Enable debug logging in `TouchGFXCommandProcessor` for message flow tracing
- **Frame Buffer Inspection**: Add debug output to inspect frame buffer contents
- **Message Queue Monitoring**: Track queue usage to detect overflow conditions
- **Performance Profiling**: Use GPIO signals for frame timing analysis (when implemented)

### Error Codes and Messages
- **Queue Full**: Custom message queue reaches capacity (10 messages) - oldest message discarded
- **Invalid Message Type**: Unknown kernel message received - logged and ignored
- **Display Update Failure**: Frame buffer write fails - check kernel communication

## Extensibility and Customization Guide

### Adding Custom Hardware Support
1. **Extend HAL**: Create custom HAL class inheriting from `TouchGFXHAL`
2. **Implement Interfaces**: Override methods for new hardware (e.g., DMA, touch)
3. **Update Stubs**: Replace stub implementations with functional code

### Custom Message Handling
```cpp
class MyCustomHandler : public ICustomMessageHandler {
public:
    bool customMessageHandler(MessageBase* msg) override {
        switch (msg->getType()) {
            case MY_CUSTOM_MESSAGE_TYPE:
                // Handle custom message
                return true;
            default:
                return false;
        }
    }
};

// Register handler
cmdProc.setCustomMessageHandler(new MyCustomHandler());
```

### Button Customization
Modify `TouchGFXCommandProcessor::handleEvent()` to change key mappings or add new buttons.

### Display Configuration
Adjust `skWidth`, `skHeight`, and `skBufferSize` in `TouchGFXHAL.cpp` for different display sizes.

## Best Practices and Performance Tips

### Memory Management
- **Static Allocation**: Use static frame buffer to avoid heap fragmentation
- **Frontend Heap**: Monitor usage for dynamic GUI elements
- **Message Queue**: Keep queue size optimized (currently 10) for your use case

### Performance Optimization
- **Frame Rate**: Balance between smooth animation and CPU usage
- **Rendering**: Minimize overdraw by using efficient widget layouts
- **Input Polling**: Button sampling is polled - consider interrupt-driven approaches for low-latency input

### Code Organization
- **Separation of Concerns**: Keep TouchGFX logic separate from UNA platform code
- **Modular Design**: Use callbacks for lifecycle events rather than tight coupling
- **Error Handling**: Implement robust error handling for kernel communication failures

### Development Tips
- **Testing**: Use simulator builds for rapid development and testing
- **Version Control**: Keep generated and custom code in separate files
- **Documentation**: Document any custom extensions or modifications

## Overall Architecture Characteristics

### Hardware Configuration
- **Display**: 240×240 pixels, 8-bit color (ABGR2222 format)
- **Frame Buffer**: Single static buffer, software rendering only
- **DMA**: Not supported (stub implementation)
- **Touch Input**: Not implemented (stub controller)
- **GPIO**: Not utilized (stub implementation)

### Performance Characteristics
- **Rendering**: Software-based, no hardware acceleration available
- **Memory Usage**: ~57.6 KB static frame buffer + dynamic allocations
- **Synchronization**: Kernel-driven VSync via messaging system
- **Input Handling**: Polled button sampling, no interrupt-driven input
- **Threading Model**: Single-threaded GUI execution

### Integration Approach
- **Kernel Integration**: Asynchronous message-based communication
- **Lifecycle Management**: Command processor handles complete application lifecycle
- **Threading**: Single-threaded execution within GUI task
- **Resource Management**: Frontend heap for TouchGFX dynamic allocations

### Extensibility Features
- **Custom Messages**: Support for application-specific kernel messages via queue
- **Lifecycle Callbacks**: Configurable start/stop/resume/suspend event handlers
- **Hardware Abstraction**: Modular HAL design enables platform customization
- **Stub Architecture**: Easy replacement of non-implemented features

### Limitations and Constraints
- **No Hardware Acceleration**: Software rendering limits performance for complex UIs
- **No Touch Support**: Button-only input, no gesture recognition
- **No DMA**: Memory copies use CPU, potential bottleneck for large transfers
- **Single Buffer**: No double buffering, may cause tearing if not synchronized properly
- **Fixed Resolution**: Currently optimized for 240×240 displays only

## UNA-Specific System Integration Points

The UNA SDK extends standard TouchGFX with comprehensive system integration capabilities that enable seamless operation within the UNA platform ecosystem. These integration points provide the foundation for building sophisticated embedded applications with proper lifecycle management, inter-process communication, and resource coordination.

### Core Integration Components

#### TouchGFXCommandProcessor
- **Location**: [`Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp`](Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp), [`Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp`](Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp)
- **Purpose**: Singleton command processor that serves as the central hub for UNA kernel integration
- **Key Features**:
  - Asynchronous message-based communication with UNA kernel
  - Fixed-size message queue (capacity: 10 messages) for custom application messages
  - Lifecycle event handling (start/stop/resume/suspend)
  - Button event processing and sampling
  - Frame buffer update coordination

#### Kernel Message Processing
- **Message Types Handled**:
  - `COMMAND_APP_STOP`: Graceful application termination with cleanup
  - `EVENT_GUI_TICK`: Frame synchronization and rendering triggers
  - `EVENT_BUTTON`: Physical button input processing (SW1-SW4 mapping)
  - `COMMAND_APP_GUI_RESUME/SUSPEND`: GUI state management
  - Custom application-specific messages via extensible queue system
- **Integration**: Direct kernel communication through `SDK::Kernel` interface

#### Hardware Abstraction Layer Extensions
- **Custom HAL Implementation**: [`Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp`](Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp)
  - Kernel-driven frame buffer flushing via `writeDisplayFrameBuffer()`
  - Button controller integration with kernel message sampling
  - Static frame buffer allocation (57.6 KB for 240×240×8-bit)
  - VSync synchronization through kernel messaging

#### Operating System Wrappers
- **Location**: [`Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp`](Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp)
- **UNA-Specific Functions**:
  - `waitForVSync()`: Blocks until kernel sends GUI tick message
  - `taskDelay()`: Uses kernel delay functionality
  - `taskYield()`: Kernel-based task scheduling

### Application Lifecycle Management

#### Lifecycle Callbacks
- **Interface**: `SDK::Interface::IGuiLifeCycleCallback`
- **Events**:
  - `onStart()`: Called once when GUI application begins
  - `onStop()`: Called during application termination for cleanup
  - `onResume()`: GUI reactivation after suspension
  - `onSuspend()`: GUI deactivation
  - `onFrame()`: Called each frame for application logic

#### Custom Message Handling
- **Interface**: `SDK::Interface::ICustomMessageHandler`
- **Purpose**: Enables application-specific kernel message processing
- **Implementation**: `customMessageHandler()` method for processing queued messages

### Key Integration Points Summary

| Integration Point | Standard TouchGFX | UNA Extension | Purpose |
|-------------------|-------------------|---------------|---------|
| Frame Synchronization | Hardware VSync | Kernel message-based | Ensures proper timing without hardware interrupts |
| Input Handling | Touch/DMA controllers | Kernel button messages | Physical button integration via messaging |
| Lifecycle Management | Application init/exit | Full lifecycle callbacks | Proper resource management and state transitions |
| Inter-Process Communication | N/A | Message queue system | Custom application messaging with kernel |
| Task Scheduling | OS task switching | Kernel yield/delay | Cooperative multitasking within UNA ecosystem |
| Frame Buffer Updates | DMA/hardware transfer | Kernel message dispatch | Display updates through UNA communication system |

### Benefits of UNA Integration

- **Seamless Platform Integration**: TouchGFX applications operate as first-class citizens within the UNA ecosystem
- **Resource Coordination**: Proper lifecycle management prevents resource conflicts
- **Extensible Communication**: Message-based architecture supports custom application features
- **Robust Error Handling**: Kernel-level supervision and graceful failure recovery
- **Performance Optimization**: Kernel-driven synchronization minimizes unnecessary processing

## References and Further Reading

### TouchGFX Documentation
- [TouchGFX Designer Guide](https://support.touchgfx.com/docs/introduction/welcome)
- [TouchGFX HAL Documentation](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/hal)
- [TouchGFX Framework Overview](https://support.touchgfx.com/docs/introduction/touchgfx-framework)

### UNA SDK Documentation
- UNA Kernel Messaging System
- UNA Hardware Abstraction Layer
- UNA Application Lifecycle Management

### Related Projects
- STM32 TouchGFX Examples
- Embedded GUI Development Best Practices
- Real-time Operating System Integration Patterns

## FAQ

### Q: Why is the frame buffer static and not dynamically allocated?
**A:** Static allocation ensures predictable memory usage and avoids heap fragmentation in resource-constrained embedded systems. The fixed 240×240 resolution allows for compile-time size calculation.

### Q: Can I add touch support to this port?
**A:** Yes, implement the `STM32TouchController` stub with actual touch hardware interfacing. Update the HAL initialization to use the functional controller instead of the stub.

### Q: How do I handle custom kernel messages in my TouchGFX application?
**A:** Implement the `ICustomMessageHandler` interface and register it with the command processor. Messages will be queued and processed during the GUI task loop.

### Q: What happens if the message queue overflows?
**A:** The oldest message is discarded with a warning logged. Monitor queue usage in production to ensure adequate capacity.

### Q: Can I use hardware acceleration for rendering?
**A:** Currently not supported. The DMA stub can be implemented if hardware acceleration becomes available on the UNA platform.

### Q: How do I debug display update issues?
**A:** Enable logging in the command processor to trace display update messages. Verify kernel-side display handling and check for message delivery failures.

### Q: Is double buffering supported?
**A:** Not currently implemented. The single buffer design simplifies memory management but may require careful synchronization to avoid visual artifacts.

### Q: Can I change the display resolution?
**A:** Yes, but requires modifying `skWidth`, `skHeight`, and `skBufferSize` in `TouchGFXHAL.cpp`, and regenerating TouchGFX assets for the new resolution.

This comprehensive architecture provides a robust foundation for TouchGFX integration in the UNA SDK, effectively balancing framework requirements with platform constraints while ensuring clean separation of concerns and extensibility for future enhancements.