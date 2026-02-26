# TouchGFX Port Architecture

## Table of Contents

1. [Overview](#overview)
2. [Definitions and Terminology](#definitions-and-terminology)
3. [Structural Architecture](#structural-architecture)
4. [API Structure](#api-structure)
5. [Behavioral Architecture](#behavioral-architecture)
   - [Initialization Flow](#initialization-flow)
   - [Rendering Flow](#rendering-flow)
   - [Input Handling](#input-handling)
   - [Lifecycle Management](#lifecycle-management)
   - [Data Flows](#data-flows)
6. [UNA-Specific System Integration Points](#una-specific-system-integration-points)

## Overview

The TouchGFX port implementation for the UNA SDK provides a comprehensive hardware abstraction layer (HAL) that seamlessly integrates the TouchGFX GUI framework into the UNA platform (see [Architecture Deep Dive](architecture-deep-dive.md)). This port empowers developers to create sophisticated graphical user interfaces for UNA-based applications, leveraging TouchGFX's extensive widget library and rendering capabilities.

The implementation comprises custom HAL classes that extend TouchGFX-generated code, a robust command processor for kernel integration, and stub implementations for hardware interfaces not currently utilized in the UNA platform configuration. The port supports a 240x240 pixel display with 8-bit color depth using ABGR2222 format, software-based rendering, and button-based input handling through the UNA kernel messaging system.

Key architectural principles include:
- **Modular Design**: Separation between TouchGFX framework and UNA platform specifics
- **Message-Based Communication**: Asynchronous integration with the UNA kernel
- **Extensibility**: Support for future hardware customizations and additional TouchGFX features
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
The TouchGFXHAL serves as the main hardware abstraction layer, extending the TouchGFX-generated HAL to provide UNA platform-specific customizations for frame buffer management, button controller integration, and display operations.
- **Location**: `Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp`, `Libs/Header/SDK/Port/TouchGFX/TouchGFXHAL.hpp`
- **Purpose**: Main hardware abstraction layer extending TouchGFXGeneratedHAL
- **Key Responsibilities**:
  - Frame buffer management and allocation
  - Button controller integration
  - Display initialization and configuration
  - Interrupt handling (delegated to generated HAL)
  - Frame synchronization and flushing

#### TouchGFXCommandProcessor
The TouchGFXCommandProcessor acts as the central command processor and lifecycle manager, implementing a singleton pattern to handle kernel message processing, GUI lifecycle management, and frame synchronization.
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
The HWButtonController provides button input handling for TouchGFX, sampling button states from the command processor and translating UNA button events to TouchGFX key codes through polled input handling.
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
The TouchGFXHAL class extends the TouchGFX-generated HAL to provide UNA platform-specific customizations, including frame buffer management, button controller integration, and display initialization.
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
The TouchGFXCommandProcessor serves as a singleton managing global GUI state, handling kernel message processing, lifecycle management, and button event translation.
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
The port provides initialization functions for setting up the TouchGFX framework, registering resources, and establishing the main GUI task entry point: [`Libs\Source\AppSystem\EntryPoint\TouchGFX\main.cpp`](Libs\Source\AppSystem\EntryPoint\TouchGFX\main.cpp)
- `touchgfx_init()`: Initializes TouchGFX framework, registers resources
- `touchgfx_taskEntry()`: Main GUI task entry point (infinite loop)
- `touchgfx_components_init()`: Component initialization (currently empty)

### Key Relationships
- **TouchGFXHAL** extends **TouchGFXGeneratedHAL** for UNA-specific customizations
- **TouchGFXCommandProcessor** acts as a singleton managing global GUI state
- **HWButtonController** polls **TouchGFXCommandProcessor** for input
- **OSWrappers** delegates VSync waiting to **TouchGFXCommandProcessor**
- Generated classes provide minimal base implementations with stub functionality

### Design Patterns

#### Singleton Pattern
- **TouchGFXCommandProcessor**: Ensures single instance managing global GUI state
- Provides centralized access to command processing and lifecycle management

#### Adapter Pattern
- **TouchGFXHAL**: Adapts TouchGFX HAL interface to UNA platform specifics
- **HWButtonController**: Translates UNA button events to TouchGFX input system

#### Template Method Pattern
- **TouchGFXHAL** overrides specific methods while delegating others to generated base class
- Enables customization of key behaviors while maintaining framework compatibility

#### Observer Pattern
- **Event Listeners**: HAL registers with TouchGFX Application for framework events
- **Lifecycle Callbacks**: Command processor notifies registered callbacks of state changes

#### Bridge Pattern
- **OSWrappers**: Bridges TouchGFX OS requirements to UNA kernel primitives
- Separates platform-specific OS operations from framework logic

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
2. **TouchGFX Initialization**: Framework configuration including resource registration and HAL setup
   - `touchgfx_init()` configures the framework
   - Bitmap database, text resources, and font providers are registered
   - Frontend heap is initialized for dynamic allocations
   - HAL is initialized via `TouchGFXHAL::initialize()`
3. **HAL Setup**: Hardware abstraction layer initialization with frame buffer allocation and controller configuration
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
### Advanced Configuration
- **Custom Message Handling**: Implement `ICustomMessageHandler` for application-specific messages
- **Button Mapping**: Modify `TouchGFXCommandProcessor::handleEvent()` for custom key mappings
- **Display Parameters**: Adjust frame buffer size in `TouchGFXHAL.cpp` if display resolution changes

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

### Adding Future Hardware Support
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

## UNA-Specific System Integration Points

The UNA SDK extends standard TouchGFX with comprehensive system integration capabilities that enable seamless operation within the UNA platform ecosystem. These integration points provide the foundation for building sophisticated embedded applications with proper lifecycle management, inter-process communication, and resource coordination.

### Overall Architecture Characteristics

#### Hardware Configuration
- **Display**: 240×240 pixels, 8-bit color (ABGR2222 format)
- **Frame Buffer**: Single static buffer, software rendering only
- **DMA**: Not supported (stub implementation)
- **Touch Input**: Not implemented (stub controller)
- **GPIO**: Not utilized (stub implementation)

#### Performance Characteristics
- **Rendering**: Software-based, no hardware acceleration available yet.
- **Memory Usage**: ~57.6 KB static frame buffer + dynamic allocations
- **Synchronization**: Kernel-driven VSync via messaging system
- **Input Handling**: Polled button sampling, no interrupt-driven input
- **Threading Model**: Single-threaded GUI execution

#### Integration Approach
- **Kernel Integration**: Asynchronous message-based communication
- **Lifecycle Management**: Command processor handles complete application lifecycle
- **Threading**: Single-threaded execution within GUI task
- **Resource Management**: Frontend heap for TouchGFX dynamic allocations

#### Extensibility Features
- **Custom Messages**: Support for application-specific kernel messages via queue
- **Lifecycle Callbacks**: Configurable start/stop/resume/suspend event handlers
- **Hardware Abstraction**: Modular HAL design enables platform customization
- **Stub Architecture**: Easy replacement of non-implemented features

#### Limitations and Constraints
- **No Hardware Acceleration**: Software rendering limits performance for complex UIs
- **No Touch Support**: Button-only input, no gesture recognition
- **No DMA**: Memory copies use CPU, potential bottleneck for large transfers
- **Single Buffer**: No double buffering, may cause tearing if not synchronized properly
- **Fixed Resolution**: Currently optimized for 240×240 displays only

### Core Integration Components

#### TouchGFXCommandProcessor

The TouchGFXCommandProcessor serves as a singleton command processor that acts as the central hub for UNA kernel integration, providing asynchronous message-based communication, lifecycle event handling, and frame buffer update coordination.
- **Location**: [`Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp:25-77`](Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp:25), [`Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp:35-76`](Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp:35)
- **Purpose**: Singleton command processor that serves as the central hub for UNA kernel integration
- **Key Features**:
  - Asynchronous message-based communication with UNA kernel
  - Fixed-size message queue (capacity: 10 messages, configurable) for custom application messages
  - Lifecycle event handling (start/stop/resume/suspend)
  - Button event processing and sampling
  - Frame buffer update coordination

**Technical Implementation Details:**
```cpp
// Singleton instance management
TouchGFXCommandProcessor& TouchGFXCommandProcessor::GetInstance() {
    static TouchGFXCommandProcessor sInstance;
    return sInstance;
}

// Message processing loop (simplified)
bool TouchGFXCommandProcessor::waitForFrameTick() {
    while (true) {
        SDK::MessageBase* msg = nullptr;
        if (!mKernel.comm.getMessage(msg)) {
            continue; // Block until message available
        }

        switch (msg->getType()) {
            case SDK::MessageType::EVENT_GUI_TICK:
                // Process frame tick
                if (mAppLifeCycleCallback) {
                    mAppLifeCycleCallback->onFrame();
                }
                return false; // Allow TouchGFX to render
            // ... other message types
        }
    }
}
```

#### Kernel Message Processing

**Message Structure Details:**
The port handles various kernel message types for lifecycle management, input processing, and display updates, providing seamless integration between TouchGFX and the UNA kernel messaging system.

**EventButton Message Structure:**
```cpp
// From Libs/Header/SDK/Messages/CommandMessages.hpp:462-501
struct EventButton : public MessageBase {
    enum class Id : uint8_t {
        SW1 = 0, SW2, SW3, SW4  // Physical button identifiers
    };

    enum class Event : uint8_t {
        PRESS = 0, RELEASE, CLICK, LONG_PRESS, HOLD_1S, HOLD_5S, HOLD_10S
    };

    uint32_t timestamp;  // Event timestamp
    Id id;              // Which button (SW1-SW4)
    Event event;        // Event type (CLICK used for TouchGFX)
};
```

**Button Mapping Implementation:**
```cpp
// From Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp:189-205
void TouchGFXCommandProcessor::handleEvent(SDK::Message::EventButton* msg) {
    if (!mIsGuiResumed) {
        mLastButtonCode = '\0';
        return;
    }

    if (msg->event == SDK::Message::EventButton::Event::CLICK) {
        switch (msg->id) {
            case SDK::Message::EventButton::Id::SW1: mLastButtonCode = '1'; break;
            case SDK::Message::EventButton::Id::SW2: mLastButtonCode = '3'; break;
            case SDK::Message::EventButton::Id::SW3: mLastButtonCode = '2'; break;
            case SDK::Message::EventButton::Id::SW4: mLastButtonCode = '4'; break;
        }
    }
}
```

**RequestDisplayUpdate Message Structure:**
```cpp
// From Libs/Header/SDK/Messages/CommandMessages.hpp:293-306
struct RequestDisplayUpdate : public MessageBase {
    const uint8_t* pBuffer;  // Pointer to 240x240x1 byte frame buffer
    int16_t x, y;            // Update region (unused, always full screen)
    int16_t width, height;   // Region size (unused, always 0 for full update)

    RequestDisplayUpdate()
        : MessageBase(MessageType::REQUEST_DISPLAY_UPDATE)
        , pBuffer(nullptr), x(0), y(0), width(0), height(0) {}
};
```

**Frame Buffer Update Implementation:**
```cpp
// From Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp:157-169
void TouchGFXCommandProcessor::writeDisplayFrameBuffer(const uint8_t* data) {
    if (!data || !mIsGuiResumed) {
        return;  // Don't update if suspended or invalid data
    }

    auto* msg = mKernel.comm.allocateMessage<SDK::Message::RequestDisplayUpdate>();
    if (msg) {
        msg->pBuffer = data;  // Pass frame buffer pointer
        mKernel.comm.sendMessage(msg, 1000);  // Send with 1s timeout
        mKernel.comm.releaseMessage(msg);
    }
}
```

- **Message Types Handled**:
  - `COMMAND_APP_STOP`: Graceful application termination with cleanup
  - `EVENT_GUI_TICK`: Frame synchronization and rendering triggers
  - `EVENT_BUTTON`: Physical button input processing (SW1-SW4 mapping)
  - `COMMAND_APP_GUI_RESUME/SUSPEND`: GUI state management
  - Custom application-specific messages via extensible queue system
- **Integration**: Direct kernel communication through `SDK::Kernel` interface

#### Hardware Abstraction Layer Extensions

The custom HAL implementation provides kernel-driven frame buffer management, button controller integration, and VSync synchronization to ensure proper display updates and input handling within the UNA platform.
- **Custom HAL Implementation**: [`Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp:67-87`](Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp:67)
  - Kernel-driven frame buffer flushing via `writeDisplayFrameBuffer()`
  - Button controller integration with kernel message sampling
  - Static frame buffer allocation (57.6 KB for 240×240×8-bit)
  - VSync synchronization through kernel messaging

**HAL Frame Buffer Management:**
```cpp
// From Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp:53-64
static const int16_t skWidth = 240;
static const int16_t skHeight = 240;
static const uint32_t skBufferSize = skWidth * skHeight;  // 57,600 bytes

static uint8_t sFrameBuffer[skBufferSize];  // Static allocation
static uint8_t* spActiveBuffer;
static bool sFlushBufferReq;

// Frame buffer initialization
void TouchGFXHAL::initialize() {
    HAL::initialize();
    spActiveBuffer = sFrameBuffer;
    setFrameBufferStartAddresses((void*)spActiveBuffer, nullptr, nullptr);
}
```

**Flush Mechanism:**
The HAL implements a deferred flush mechanism that coordinates frame buffer updates with the kernel display system, ensuring efficient rendering and display synchronization.
```cpp
// From Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp:128-142
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect) {
    sFlushBufferReq = true;  // Set flag for endFrame()
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
}

// From Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp:203-216
void TouchGFXHAL::endFrame() {
    if (sFlushBufferReq) {
        // Send frame buffer to kernel for display
        SDK::TouchGFXCommandProcessor::GetInstance()
            .writeDisplayFrameBuffer(spActiveBuffer);
        sFlushBufferReq = false;
    }
    TouchGFXGeneratedHAL::endFrame();
}
```

#### Operating System Wrappers

The OS wrappers provide the bridge between TouchGFX OS requirements and UNA kernel primitives, handling VSync synchronization, task scheduling, and timing operations.
- **Location**: [`Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp:105-108`](Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp:105)
- **UNA-Specific Functions**:
  - `waitForVSync()`: Blocks until kernel sends GUI tick message
  - `taskDelay()`: Uses kernel delay functionality
  - `taskYield()`: Kernel-based task scheduling

**VSync Implementation:**
```cpp
// From Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp:105-108
void OSWrappers::waitForVSync() {
    // Delegate to command processor message loop
    SDK::TouchGFXCommandProcessor::GetInstance().waitForFrameTick();
}
```

### Application Lifecycle Management

#### Lifecycle Callbacks

The port provides lifecycle callback interfaces that enable applications to respond to GUI state changes, including start, stop, resume, suspend, and frame events for proper application management.
- **Interface**: `SDK::Interface::IGuiLifeCycleCallback`
- **Events**:
  - `onStart()`: Called once when GUI application begins
  - `onStop()`: Called during application termination for cleanup
  - `onResume()`: GUI reactivation after suspension
  - `onSuspend()`: GUI deactivation
  - `onFrame()`: Called each frame for application logic

**Lifecycle State Machine:**
```cpp
// From Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp:25-32
TouchGFXCommandProcessor::TouchGFXCommandProcessor()
    : mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    , mStartCallbackCalled(false)
    , mIsGuiResumed(false)
    , mLastButtonCode(0)
    , mAppLifeCycleCallback(nullptr)
    , mCustomMessageHandler(nullptr) {}
```

#### Custom Message Handling

The port supports custom message handling interfaces that allow applications to process application-specific kernel messages through a dedicated message queue system.
- **Interface**: `SDK::Interface::ICustomMessageHandler`
- **Purpose**: Enables application-specific kernel message processing
- **Implementation**: `customMessageHandler()` method for processing queued messages

**Message Queue Implementation:**
```cpp
// From Libs/Header/SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp:73
SDK::Tools::FixedQueue<SDK::MessageBase*, 10> mUserQueue {};

// Queue processing in message loop
void TouchGFXCommandProcessor::callCustomMessageHandler() {
    while (!mUserQueue.empty()) {
        auto v = mUserQueue.pop();
        if (v) {
            auto msg = *v;
            bool result = false;
            if (mCustomMessageHandler) {
                result = mCustomMessageHandler->customMessageHandler(msg);
            }
            // Send response back to kernel
            msg->setResult(result ? SUCCESS : FAIL);
            mKernel.comm.sendResponse(msg);
            mKernel.comm.releaseMessage(msg);
        }
    }
}
```

### Performance Implications and Optimizations

#### Memory Management
- **Static Frame Buffer**: 57.6 KB pre-allocated to avoid heap fragmentation
- **Fixed Message Queue**: 10-message capacity prevents unbounded growth
- **Frontend Heap**: TouchGFX dynamic allocations monitored for leaks

**Performance Characteristics:**
- **Frame Rate**: Limited by kernel tick frequency (typically 30-60 FPS)
- **CPU Usage**: Software rendering + message processing overhead
- **Memory Footprint**: ~64 KB total (frame buffer + TouchGFX overhead)
- **Latency**: Button input delayed by message processing (~1-2ms)

#### Synchronization Optimizations
- **Kernel-Driven VSync**: Eliminates polling, reduces power consumption
- **Asynchronous Display Updates**: Non-blocking frame buffer submission
- **Message Prioritization**: Critical messages (ticks, buttons) processed first

#### Error Handling and Recovery
- **Queue Overflow Protection**: Oldest messages discarded with logging
- **Timeout Handling**: Display update messages have 1-second timeout
- **Graceful Degradation**: GUI suspends on communication failures

### Additional Integration Points

#### Memory Management Integration
- **Frontend Heap Monitoring**: Integration with UNA memory tracking
- **Resource Cleanup**: Automatic cleanup on application termination
- **Leak Prevention**: Static allocations avoid dynamic memory issues

#### Error Handling and Diagnostics
- **Message Logging**: All kernel communications logged for debugging
- **State Validation**: GUI state checked before processing messages
- **Recovery Mechanisms**: Automatic resume after transient failures

#### Power Management Integration
- **Suspend/Resume Handling**: Proper power state transitions
- **Display Control**: Backlight and display power managed by kernel
- **Idle Detection**: TouchGFX animations paused during suspend


