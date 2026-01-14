# SDK Overview

The Una-Watch SDK provides a comprehensive suite of tools, libraries, and interfaces for building high-performance wearable applications.

## Key Components

- **Core Interfaces**: Type-safe APIs for system services, communication, and hardware access.
- **Message System**: High-performance inter-process communication (IPC) framework.
- **Sensor APIs**: Simplified access to PPG, IMU, and GNSS sensors.
- **File System**: Multi-volume storage management (Flash, USB, External).
- **Build Tools**: Automated scripts for compilation and `.uapp` packaging.
- **Simulator**: Desktop-based environment for rapid prototyping and testing.

## Unique Architecture

Una-Watch apps are unique in the wearable space:
- **Pure Machine Code**: Compiled binaries running directly on the MCU.
- **Position Independent**: Abstracted from kernel memory layout.
- **Shared libc**: Memory-efficient standard library utilization.

For a deeper look into how this works, see the [Architecture Deep Dive](architecture-deep-dive.md).

## SDK Project Structure

The SDK provides a comprehensive set of interfaces that apps use to interact with the watch's kernel and hardware:

### IAppComm (Communication Interface)

```cpp
class IAppComm {
public:
    virtual uint32_t getProcessId() const = 0;
    virtual bool getMessage(MessageBase*& msg, uint32_t timeoutMs = 0xFFFFFFFF) = 0;
    virtual void sendResponse(MessageBase* msg) = 0;
    virtual void releaseMessage(MessageBase* msg) = 0;
    virtual bool sendMessage(MessageBase* msg, uint32_t timeoutMs = 0) = 0;

    template<typename T>
    T* allocateMessage() { ... }
};
```

- **Process Identity**: Unique process identifier assigned by kernel.
- **Message Reception**: Receive messages with configurable timeout (blocking by default).
- **Response Handling**: Send responses for request-response patterns.
- **Memory Management**: Explicit message release back to kernel pool.
- **Type-safe Allocation**: Template-based message allocation from kernel pools.

### ISystem (System Services Interface)

```cpp
class ISystem {
public:
    virtual void exit(int status = 0) = 0;
    virtual uint32_t getTimeMs() = 0;
    virtual void delay(uint32_t ms) = 0;
    virtual void yield() = 0;
};
```

- **Lifecycle**: Terminate the application with an exit status.
- **Timing**: Get system time in milliseconds and perform millisecond-precision delays.
- **Scheduling**: Yield execution back to the kernel.

### IFileSystem (Storage Interface)

```cpp
class IFileSystem {
public:
    virtual bool mkdir(const char* path) = 0;
    virtual std::unique_ptr<IFile> file(const char* path) = 0;
    virtual std::unique_ptr<IDirectory> dir(const char* path) = 0;
    virtual bool exist(const char* path) const = 0;
    virtual bool remove(const char* path) = 0;
    virtual bool rename(const char* oldPath, const char* newPath) = 0;
    virtual bool copy(const char* oldPath, const char* newPath) = 0;
    virtual bool objectInfo(const char* path, ObjectInfo& item) const = 0;
};
```

- **Object-Oriented API**: Uses `IFile` and `IDirectory` objects for file and directory operations.
- **Path Management**: Support for internal flash (`0:/`), external storage (`1:/`), and USB (`2:/`).
- **File Operations**: Standard read/write/seek via `IFile` interface.
- **Metadata**: Retrieve object information (size, type, timestamps).

### ILogger (Debugging Interface)

```cpp
class ILogger {
public:
    virtual void printf(const char *format, ...) = 0;
    virtual void vprintf(const char *format, va_list args) = 0;
    virtual void mvprintf(const char *level, const char *module_name,
                         const char *func, int line, const char *fmt, va_list args) = 0;
};
```

- **Formatted Output**: Standard `printf`-style logging.
- **Metadata Support**: Log with level, module name, function, and line number.

### ISensorManager (Sensor Interface)

```cpp
class ISensorManager {
public:
    virtual SDK::Interface::ISensorDriver* getDefaultSensor(SDK::Sensor::Type type) = 0;
    virtual std::vector<SDK::Interface::ISensorDriver*> getSensorList(SDK::Sensor::Type type) = 0;
};
```

- **Sensor Discovery**: Retrieve default sensors or a list of available sensors by type.
- **Driver-based Access**: Interact with sensors via the `ISensorDriver` interface.

### IGlance (Glance Application Interface)

```cpp
class IGlance {
public:
    virtual Info glanceGetInfo() = 0;
    virtual void glanceUpdate()  = 0;
    virtual void glanceClose()   = 0;
};
```

- **Lifecycle Management**: Methods for initializing, updating, and closing glance views.
- **Meta-information**: Retrieve control information and alternative names for the glance.

### Message System Architecture

#### Message Types and Ranges

The SDK uses a structured message system for inter-process communication:

| Range | Type | Purpose |
|-------|------|---------|
| 0x01010000 - 0x01060000 | Commands | Kernel-to-app directives (response expected) |
| 0x01070000 - 0x01090000 | App Control | App-to-kernel lifecycle requests |
| 0x02010000 - 0x020A0000 | System/Hardware | Requests for system info, display, backlight, etc. |
| 0x03010000 - 0x03040000 | Events | System-level notifications (fire-and-forget) |
| 0x030A0000 - 0x030E0000 | Glances | Glance-specific updates and events |
| 0x03100000 - 0x03180000 | Sensors | Sensor discovery and data events |
| 0x00000000 - 0x0000FFFF | Custom | Application-specific internal communication |

#### Key System Messages

```cpp
// Application Control
COMMAND_APP_RUN              // Start application
COMMAND_APP_STOP             // Stop application
REQUEST_APP_TERMINATE        // App requesting its own termination

// System Info & Settings
REQUEST_BATTERY_STATUS       // Get battery level
REQUEST_SYSTEM_SETTINGS      // Get watch settings
REQUEST_SYSTEM_INFO          // Get firmware version, device name

// Hardware Control
REQUEST_DISPLAY_CONFIG       // Get screen dimensions
REQUEST_BACKLIGHT_SET        // Set screen brightness
REQUEST_VIBRO_PLAY           // Trigger vibration
REQUEST_BUZZER_PLAY          // Trigger sound

// Sensor Messages
REQUEST_SENSOR_LAYER_GET_DEFAULT // Request default sensor
REQUEST_SENSOR_LAYER_CONNECT     // Start sensor sampling
EVENT_SENSOR_LAYER_DATA          // Asynchronous sensor data update
```

#### Message Processing Pattern

```cpp
// App message handling loop
void Service::run() {
    MessageBase* msg = nullptr;
    while (comm->getMessage(msg, 100)) {  // 100ms timeout
        switch (msg->getType()) {
            case MessageType::EVENT_SENSOR_LAYER_DATA:
                handleSensorData(static_cast<SensorDataEvent*>(msg));
                break;
            case MessageType::COMMAND_APP_STOP:
                handleStopCommand(static_cast<StopCommand*>(msg));
                break;
            // ... other message types
        }

        // Send response if required
        if (msg->needsResponse()) {
            comm->sendResponse(msg);
        }

        // Always release message when done
        comm->releaseMessage(msg);
    }
}
```

## SDK Project Structure

```
SDK/
├── Libs/
│   ├── Header/SDK/           # SDK Interface Headers
│   │   ├── Interfaces/       # Core API interfaces
│   │   ├── Messages/         # Message type definitions
│   │   ├── SensorLayer/      # Sensor data structures
│   │   ├── Kernel/           # Kernel provider interfaces
│   │   └── Wrappers/         # Standard library wrappers
│   └── Source/SDK/           # SDK Implementation
├── Port/
│   └── TouchGFX/             # TouchGFX integration
├── ThirdParty/               # External dependencies
│   ├── coreJSON/            # JSON parsing library
│   └── FitSDK/              # Fitness data format
├── Utilities/Scripts/        # Build and packaging tools
├── Simulator/                # Development simulator
└── Visual Studio/            # Windows development support
```

### High-Level Utilities

In addition to core interfaces, the SDK provides several high-level utilities to simplify common tasks:

#### GSModel & GSBridge (GUI-Service Communication)

The `GSModel` provides a type-safe, bidirectional bridge between the GUI and Service processes using `std::variant` and `std::visit`. This is the recommended way to handle application-level events between processes.

```cpp
// Define events in G2SEvents.hpp and S2GEvents.hpp
// In Service:
GSModel model(serviceHandler);
model.process(); // Drain GUI->Service events

// In GUI:
IGUIModel* bridge = ...;
bridge->post(G2SEvent::SomeEvent{data});
```

#### Glance UI System

A lightweight UI framework for the 240x60 pixel notification area. It uses a `Form` to manage `GlanceControl` items like Text, Image, Line, and Rectangle. This system is optimized for low-power notification display.

```cpp
SDK::Glance::Form form(240, 60);
auto text = form.createText();
text.init({0, 0}, {240, 20}, "Notification", FONT_SMALL, COLOR_WHITE);
```

#### FitHelper (Fitness Data)

Simplifies the generation of Garmin FIT files, allowing apps to export activity data in a standard format compatible with major fitness platforms.

```cpp
SDK::Component::FitHelper fit(msgID, msgDef);
fit.writeMessage(data, &file);
```

#### TrackMapBuilder (GPS Visualization)

Creates a simplified pixel-based map representation of GPS tracks. It handles coordinate scaling, rotation, and filtering to fit a track into a circular or rectangular display area.

```cpp
SDK::TrackMapBuilder builder;
builder.addPoint({lat, lon});
auto screenMap = builder.build(radiusPx);
```

#### Serialization (CBOR & JSON)

The SDK includes stream-based readers and writers for both CBOR and JSON formats, optimized for memory-constrained environments. These are used for settings persistence and BLE data exchange.

- **CBOR**: `CborStreamReader`, `CborStreamWriter` (efficient binary format)
- **JSON**: `JsonStreamReader`, `JsonStreamWriter` (human-readable format)

#### Signal Processing & Tools

- **Filters**: Basic signal processing filters like `FilterSmooth` for noise reduction.
- **FixedQueue**: A template-based fixed-capacity queue for efficient data buffering.
- **CircularBuffer**: A thread-safe circular buffer implementation integrated with kernel synchronization primitives.
- **SwTimer**: High-level software timers for application-level periodic tasks.

```bash
python app_packer/app_packer.py -e <elf_file> -o <output_dir> -v <version>
```

- **ELF Processing**: Parses compiled ELF files
- **Package Creation**: Generates .uapp container files
- **Metadata Injection**: Embeds version and configuration data

### App Merging Script (app_merging.py)

```bash
python app_merging.py -name <name> -type <Activity|Utility|Glance|Clockface> \
                     -autostart -header -normal_icon <60x60.png> \
                     -small_icon <30x30.png> -appid <16hex> -appver <A.B.C> \
                     -scripts <SDK/Utilities/Scripts>
```

- **App Metadata**: Defines app properties and capabilities
- **Icon Processing**: Converts and embeds PNG icons
- **Type Configuration**: Sets app behavior and permissions
- **ID Assignment**: Unique application identifiers

### Build Integration Scripts

- **CubeIDE Integration**: Post-build scripts for automatic packaging
- **Version Management**: Automatic version incrementing
- **Dependency Checking**: Validates build prerequisites

## Simulator Environment

The SDK includes a comprehensive simulator for app development and testing:

### Simulator Features

- **Hardware Emulation**: Mock implementations of all watch hardware
- **Sensor Simulation**: Realistic sensor data generation
- **UI Testing**: TouchGFX simulator integration
- **Debugging Tools**: Enhanced logging and breakpoint support

### Simulator Architecture

```cpp
// Mock hardware interfaces
class MockLcd : public ILcd {
    void setPixel(int x, int y, Color color) override {
        // Render to simulator window
        simulatorWindow.setPixel(x, y, color);
    }
};

class MockSensorManager : public ISensorManager {
    bool requestDefaultSensor(SensorType type, SensorHandle& handle) override {
        // Create simulated sensor
        return sensorSimulator.createSensor(type, handle);
    }
};
```

### Development Workflow with Simulator

1. **Code Development**: Write app logic using SDK interfaces
2. **Compile for Simulator**: Build with simulator target
3. **Run Tests**: Execute automated test suites
4. **UI Testing**: Interact with simulated watch interface
5. **Debugging**: Step through code with full debugger support

## Third-Party Libraries & Standard Library

### Core Components

- **coreJSON**: Lightweight JSON parsing for settings and data.
- **FitSDK**: Official Garmin FitSDK for activity data format support.
- **Shared libc++**: A memory-optimized standard C++ library shared across all applications to minimize binary size.
- **FreeRTOS**: The underlying real-time operating system (kernel-side).
- **TouchGFX**: High-performance UI framework for embedded systems.

### Integration Points

- **Automatic Inclusion**: Libraries linked automatically in build process
- **Header-Only**: Many utilities available as headers only
- **Namespace Isolation**: Prevent symbol conflicts
- **Version Management**: Controlled library versions for compatibility

## Development Best Practices

### Memory Management

- **Pool Allocation**: Use kernel message pools for IPC
- **RAII Pattern**: Automatic resource cleanup
- **Stack Awareness**: Monitor stack usage in constrained environment

### Performance Optimization

- **Event-Driven Design**: Avoid polling, use callbacks
- **Message Batching**: Group related operations
- **Sensor Optimization**: Configure appropriate sampling rates

### Error Handling

- **Graceful Degradation**: Handle missing hardware gracefully
- **Timeout Management**: Prevent infinite waits
- **Logging**: Comprehensive error reporting

### Testing

- **Unit Tests**: Test individual components
- **Integration Tests**: Validate IPC communication
- **Simulator Validation**: Test on simulated hardware
- **Hardware Testing**: Final validation on real device

## SDK Version Compatibility

### Versioning Scheme

- **Major Version**: Breaking API changes
- **Minor Version**: New features, backward compatible
- **Patch Version**: Bug fixes and improvements

### Migration Guide

- **Deprecation Notices**: Advance warning of API changes
- **Compatibility Layers**: Support for older app versions
- **Upgrade Tools**: Automated migration scripts