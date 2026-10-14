(sdk-overview)=
# SDK Overview

The UNA Watch SDK provides a comprehensive suite of tools, libraries, and interfaces for building high-performance wearable applications. For platform architecture overview, see [platform overview](platform-overview.md).

## Key Components

- **Core Interfaces**: Type-safe APIs for system services, communication, and hardware access.
- **Message System**: High-performance inter-process communication (IPC) framework.
- **Sensor APIs**: Simplified access to PPG, IMU, and GNSS sensors.
- **File System**: Multi-volume storage management (Flash, USB, External).
- **Build Tools**: Automated scripts for compilation and `.uapp` packaging.
- **Simulator**: Desktop-based environment for rapid prototyping and testing.

## SDK Setup and Build Reference

To start using this SDK, see the [SDK setup](sdk-setup.md)

## SDK Project Structure

```
una-sdk/
├── Libs/
│   ├── Header/SDK/           # Public SDK headers
│   │   ├── Interfaces/       # Core API interfaces
│   │   ├── Messages/         # Message type definitions
│   │   ├── SensorLayer/      # Sensor data structures
│   │   ├── Kernel/           # Kernel provider interfaces
│   │   ├── Port/             # Platform port headers (TouchGFX)
│   │   ├── Simulator/        # Simulator headers (mocks, sim sensors)
│   │   └── Wrappers/         # Standard library wrappers
│   └── Source/               # SDK implementation
│       ├── Port/TouchGFX/    # TouchGFX integration
│       └── Simulator/        # Mock kernel and desktop simulator runtime
├── ThirdParty/               # Vendored dependencies
│   ├── coreJSON/             # JSON parsing library
│   ├── tinycbor_version/     # CBOR encoder/decoder
│   └── touchgfx/             # TouchGFX framework (sources + prebuilt libs)
├── Utilities/Scripts/        # Build and packaging tools
├── Examples/                 # Sample reference applications
├── cmake/                    # CMake toolchain and helpers
└── Docs/                     # Documentation and tutorials
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

#### SDK::Fit (Fitness Data)

A native FIT-format encoder for exporting activity data in the standard format compatible with major fitness platforms (no external dependency). Define each message's fields from the FIT profile, then stream values through the builder.

```cpp
SDK::Fit::FitWriter fit(file);
fit.begin(/*profileVersion=*/0);
fit.defineMessage(0, SDK::Fit::mesgNum(SDK::Fit::MesgNum::Record),
                  {SDK::Fit::field::Record::Timestamp, SDK::Fit::field::Record::HeartRate});
fit.data(0).u32(timestamp).u8(heartRate).write();
fit.finish();
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

- **CMake Integration**: Post-build scripts for automatic `.uapp` packaging
- **Version Management**: Automatic version incrementing
- **Dependency Checking**: Validates build prerequisites

## SDK Interfaces

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

#### Defining a Custom Message Type

Custom messages use the `0x00000000 - 0x0000FFFF` range and are routed directly
between an app's service and GUI processes. An ID outside that range is not
treated as application-specific: it will not reach a custom message handler.

Declare the ID as a `SDK::MessageType::Type` constant — a scoped `enum class`
will not convert in the constructor or in `switch (msg->getType())` — and derive
from `MessageBase`:

```cpp
constexpr SDK::MessageType::Type MY_UPDATE = 0x00000001;

#pragma pack(push, 4)
struct MyUpdate : public SDK::MessageBase {
    MyUpdate() : SDK::MessageBase(MY_UPDATE), value(0), flags(0) {}

    uint32_t value;      // plain data only
    uint8_t  flags;
};
#pragma pack(pop)

static_assert(sizeof(MyUpdate) <= 256,
              "must fit the largest kernel message pool");
```

The `#pragma pack(push, 4)` matches what the shipping example apps do and what
`MessageBase` itself uses. It changes nothing for a type whose members are all
4-byte-aligned or smaller, as above. What it buys is padding: give the type a
`double` or a 64-bit integer that does not already land on an 8-byte boundary
and the compiler would otherwise insert four bytes in front of it. A message
holding `uint32_t` then `uint64_t` is 44 bytes on the watch packed, against 48
unpacked — worth having against a 256-byte ceiling.

It is not what keeps the watch and simulator layouts agreeing. They cannot agree
in absolute terms anyway, since `MessageBase` is 32 bytes on one and 40 on the
other. Measured from the end of the header, though, that same `uint64_t` sits at
offset 4 on *both* targets with the pragma and at offset 8 on both without it —
so the packing decides the offset, and the target does not. (That holds for any
member up to 8-byte alignment; 32 and 40 differ mod 16, so an over-aligned one
would not follow it.) Nor do the two need to agree: a custom message is only ever
read by the peer process of the same build.

**Allocate through the SDK, never on the stack or as a static.**
`MessageBase::operator new` is deleted, so `IAppComm::allocateMessage<T>()` — or
the `SDK::make_msg` / `SDK::send_msg` helpers — is the only way to obtain a pool
block.

A stack instance still *compiles*, and nothing traps on it. The receiving
process reads the message on a later frame, by which time the sender's frame is
gone — and it does not only read: it writes a result, and decrements the
reference count, into another thread's dead stack. The kernel may log once per boot that a message was released with no matching
claim, and even that is suppressed for the rest of the boot if the pool has ever
reported running out of owner slots. The pool itself ignores the foreign address
silently. Assume nothing tells you.

**A message type must not declare a destructor, and must not hold a member that
needs one** — no `std::string`, no container, no owning pointer, nothing with
cleanup of its own.

This is not a style preference. `allocateMessage<T>()` is a header template, so
the placement-new that constructs your type is compiled into your application's
image and the vtable pointer it writes points into that image. The kernel frees
the image when the process is unloaded, and may destroy the message afterwards —
during teardown, or when the surviving half of an app releases something its
peer allocated — so it destroys messages **non-virtually** rather than
dispatching through a vtable that is no longer there. A destructor you declare
therefore does not run, and whatever it would have released is leaked.

The compiler cannot warn about this. `MessageBase` has a virtual destructor, so
`std::is_trivially_destructible` is false for every message type and the trait
that would express the rule is unusable. It is a contract, not a check.

**Size is checkable, and worth checking.** The largest kernel message pool is
currently 256 bytes, and `sizeof(T)` includes the `MessageBase` header — 32
bytes on the watch and 40 in a 64-bit simulator build, so 224 and 216 bytes of
payload respectively. Size against the smaller of the two and make sure the
assert holds in both builds. A larger request
returns `nullptr` and the send is dropped; the simulator allocates with a plain
`new[]`, so it will never show you this.

The `static_assert` is the one part of the contract a compiler will enforce for
you. Note it asserts against a literal: the pool sizes live in the firmware, not
in a constant the SDK exports, so re-check it against the kernel you are
building for rather than treating 256 as permanent.

**Variable-length data goes in a bounded inline array plus a count**, sized
against that 256-byte ceiling. That is what the shipping example apps do, and
for a custom message it is the only form that works. Do not put a pointer in one.

The SDK's own display API does pass a pointer, which is why this needs saying
rather than assuming: that path is safe for two separate reasons, and a custom
message has neither. Its buffer is **static**, so the pointer cannot dangle
however long the message takes to be read — and the send blocks until the kernel
has copied the frame out, so the contents cannot change underneath the reader.

Making your own buffer static fixes only the first of those. A custom message
waits for the GUI's next drained frame, which never comes while the GUI is
suspended, and up to ten can be queued at once — all pointing at the same
buffer, all seeing whatever it contains when the GUI finally runs, not what it
contained when each was sent. The pointer stays valid and the data is still
wrong.

And do not reason "my send blocks, so the storage outlives the read". A send
with a timeout waits for a *response*, not for delivery. When it times out you
do not know where the message is — still queued, already in the receiver's
handler, or evicted by later sends — only that your call has returned. A pointer
to a local is then a use-after-free if the handler ever runs; a buffer you go on
to reuse is read intact but stale, reporting whatever you wrote last rather than
what you sent. Only the evicted case is harmless, and it is the one you cannot
count on. None of this needs the GUI to be suspended, though suspension is what
makes it likely — and it is the case you are least likely to test. There is no
MMU to catch any of it.

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
- **Shared libc++**: A memory-optimized standard C++ library shared across all applications to minimize binary size.
- **FreeRTOS**: The underlying real-time operating system (kernel-side).
- **TouchGFX**: UI framework for embedded systems.

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