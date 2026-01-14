Una-Watch SDK Documentation
===========================

This document covers the Software Development Kit (SDK) for developing applications on the Una-Watch platform.

SDK Overview
------------

The Una-Watch SDK provides all the necessary tools, libraries, and interfaces for developing third-party applications that run on the watch. The SDK includes:

- **Core Interfaces**: Type-safe APIs for system services, communication, and hardware access
- **Message System**: Inter-process communication framework
- **Sensor APIs**: Hardware sensor integration
- **File System**: Multi-volume storage access
- **Build Tools**: Scripts for packaging and deployment
- **Simulator**: Development environment for testing apps
- **Third-party Libraries**: Integrated dependencies

SDK Architecture
----------------

Core Interfaces
~~~~~~~~~~~~~~~

The SDK provides a comprehensive set of interfaces that apps use to interact with the watch's kernel and hardware:

IAppComm (Communication Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class IAppComm {
   public:
       virtual bool allocateMessage(MessageBase*& msg, MessageType type) = 0;
       virtual bool sendMessage(MessageBase* msg, uint32_t timeout = 0) = 0;
       virtual bool getMessage(MessageBase*& msg, uint32_t timeout = 0) = 0;
       virtual bool sendResponse(MessageBase* msg) = 0;
       virtual uint32_t getPid() const = 0;
   };

- **Message Allocation**: Type-safe message creation from kernel pools
- **IPC Communication**: Send/receive messages between app and kernel
- **Process Identity**: Unique PID assignment for each app process

ISystem (System Services Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class ISystem {
   public:
       virtual void delay(uint32_t milliseconds) = 0;
       virtual uint32_t getBatteryLevel() const = 0;
       virtual bool isCharging() const = 0;
       virtual const char* getDeviceName() const = 0;
       virtual uint32_t getFirmwareVersion() const = 0;
   };

- **Timing**: Millisecond-precision delays
- **Power Status**: Battery level and charging state
- **Device Info**: Identification and version information

IFileSystem (Storage Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class IFileSystem {
   public:
       virtual bool open(FileHandle& handle, const char* path, FileMode mode) = 0;
       virtual bool close(FileHandle handle) = 0;
       virtual bool read(FileHandle handle, void* buffer, size_t size, size_t& read) = 0;
       virtual bool write(FileHandle handle, const void* buffer, size_t size, size_t& written) = 0;
       virtual bool mkdir(const char* path) = 0;
       virtual bool listDir(const char* path, DirEntry* entries, size_t maxEntries, size_t& count) = 0;
   };

- **Multi-Volume Support**: Access to internal flash (0:/), external storage (1:/), USB (2:/)
- **File Operations**: Standard read/write/seek operations
- **Directory Management**: Create directories, list contents

ILogger (Debugging Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class ILogger {
   public:
       virtual void log(LogLevel level, const char* format, ...) = 0;
       virtual void logTimestamp(LogLevel level, const char* format, ...) = 0;
   };

- **Log Levels**: Error, Warning, Info, Debug
- **Timestamp Integration**: Automatic timestamping with system time
- **Formatted Output**: printf-style logging

ISensorManager (Sensor Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class ISensorManager {
   public:
       virtual bool requestDefaultSensor(SensorType type, SensorHandle& handle) = 0;
       virtual bool connectSensor(SensorHandle handle, uint32_t periodMs, uint32_t latencyUs = 0) = 0;
       virtual bool disconnectSensor(SensorHandle handle) = 0;
       virtual bool getSensorData(SensorHandle handle, SensorData& data) = 0;
   };

- **Sensor Discovery**: Automatic sensor enumeration by type
- **Connection Management**: Configurable sampling periods and latency
- **Data Access**: Type-safe sensor data retrieval

IGlance (Notification Interface)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   class IGlance {
   public:
       virtual void showNotification(const char* title, const char* message, uint32_t durationMs = 3000) = 0;
       virtual void updateProgress(uint8_t percentage) = 0;
       virtual void hideNotification() = 0;
   };

- **Notification Display**: 240x60 pixel notification area
- **Progress Indication**: Visual progress bars
- **Auto-dismiss**: Configurable display duration

Message System Architecture
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Message Types and Ranges
^^^^^^^^^^^^^^^^^^^^^^^^^

The SDK uses a structured message system for inter-process communication:

.. list-table:: Message Type Ranges
   :header-rows: 1

   * - Range
     - Type
     - Purpose
     - Response
   * - 0x0000-0x0FFF
     - Events
     - Fire-and-forget notifications
     - No
   * - 0x1000-0x1FFF
     - Requests
     - Synchronous calls
     - Yes
   * - 0x2000-0x2FFF
     - Responses
     - Request acknowledgments
     - No
   * - 0x3000-0x3FFF
     - Commands
     - Kernel directives
     - Yes

Key System Messages
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // System Requests
   REQUEST_SYSTEM_SETTINGS      // Get watch settings
   REQUEST_BATTERY_STATUS       // Get battery level
   REQUEST_DISPLAY_CONFIG       // Get screen dimensions
   REQUEST_BACKLIGHT_SET        // Set screen brightness
   REQUEST_GLANCE_CONFIG        // Get glance area size

   // Sensor Messages
   REQUEST_SENSOR_DEFAULT       // Request default sensor
   REQUEST_SENSOR_CONNECT       // Connect to sensor
   REQUEST_SENSOR_DISCONNECT    // Disconnect from sensor

   // File System Messages
   REQUEST_FILE_OPEN           // Open file
   REQUEST_FILE_READ           // Read file data
   REQUEST_FILE_WRITE          // Write file data
   REQUEST_DIR_LIST            // List directory contents

Message Processing Pattern
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // App message handling loop
   void Service::run() {
       MessageBase* msg = nullptr;
       while (getMessage(msg, 100)) {  // 100ms timeout
           std::visit([this](auto&& arg) {
               using T = std::decay_t<decltype(arg)>;
               if constexpr (std::is_same_v<T, SystemRequest>) {
                   handleSystemRequest(arg);
               } else if constexpr (std::is_same_v<T, SensorData>) {
                   handleSensorData(arg);
               }
               // ... other message types
           }, msg->data);

           // Send response if required
           if (msg->expectsResponse()) {
               sendResponse(createResponse(msg));
           }
       }
   }

SDK Project Structure
---------------------

::

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

Build Tools and Scripts
-----------------------

App Packaging Script (app_packer.py)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   python app_packer/app_packer.py -e <elf_file> -o <output_dir> -v <version>

- **ELF Processing**: Parses compiled ELF files
- **Package Creation**: Generates .uapp container files
- **Metadata Injection**: Embeds version and configuration data

App Merging Script (app_merging.py)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   python app_merging.py -name <name> -type <Activity|Utility|Glance|Clockface> \
                         -autostart -header -normal_icon <60x60.png> \
                         -small_icon <30x30.png> -appid <16hex> -appver <A.B.C> \
                         -scripts <SDK/Utilities/Scripts>

- **App Metadata**: Defines app properties and capabilities
- **Icon Processing**: Converts and embeds PNG icons
- **Type Configuration**: Sets app behavior and permissions
- **ID Assignment**: Unique application identifiers

Build Integration Scripts
^^^^^^^^^^^^^^^^^^^^^^^^^

- **CubeIDE Integration**: Post-build scripts for automatic packaging
- **Version Management**: Automatic version incrementing
- **Dependency Checking**: Validates build prerequisites

Simulator Environment
---------------------

The SDK includes a comprehensive simulator for app development and testing:

Simulator Features
^^^^^^^^^^^^^^^^^^

- **Hardware Emulation**: Mock implementations of all watch hardware
- **Sensor Simulation**: Realistic sensor data generation
- **UI Testing**: TouchGFX simulator integration
- **Debugging Tools**: Enhanced logging and breakpoint support

Simulator Architecture
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

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

Development Workflow with Simulator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **Code Development**: Write app logic using SDK interfaces
2. **Compile for Simulator**: Build with simulator target
3. **Run Tests**: Execute automated test suites
4. **UI Testing**: Interact with simulated watch interface
5. **Debugging**: Step through code with full debugger support

Third-Party Libraries
---------------------

Core Components
^^^^^^^^^^^^^^^

- **coreJSON**: Lightweight JSON parsing for settings and data
- **FitSDK**: ANT+ fitness data format support
- **FreeRTOS**: Real-time operating system
- **TouchGFX**: UI framework for embedded systems
- **FatFs**: File system implementation

Integration Points
^^^^^^^^^^^^^^^^^^

- **Automatic Inclusion**: Libraries linked automatically in build process
- **Header-Only**: Many utilities available as headers only
- **Namespace Isolation**: Prevent symbol conflicts
- **Version Management**: Controlled library versions for compatibility

Development Best Practices
--------------------------

Memory Management
^^^^^^^^^^^^^^^^^

- **Pool Allocation**: Use kernel message pools for IPC
- **RAII Pattern**: Automatic resource cleanup
- **Stack Awareness**: Monitor stack usage in constrained environment

Performance Optimization
^^^^^^^^^^^^^^^^^^^^^^^^

- **Event-Driven Design**: Avoid polling, use callbacks
- **Message Batching**: Group related operations
- **Sensor Optimization**: Configure appropriate sampling rates

Error Handling
^^^^^^^^^^^^^^

- **Graceful Degradation**: Handle missing hardware gracefully
- **Timeout Management**: Prevent infinite waits
- **Logging**: Comprehensive error reporting

Testing
^^^^^^^

- **Unit Tests**: Test individual components
- **Integration Tests**: Validate IPC communication
- **Simulator Validation**: Test on simulated hardware
- **Hardware Testing**: Final validation on real device

SDK Version Compatibility
-------------------------

Versioning Scheme
^^^^^^^^^^^^^^^^^

- **Major Version**: Breaking API changes
- **Minor Version**: New features, backward compatible
- **Patch Version**: Bug fixes and improvements

Migration Guide
^^^^^^^^^^^^^^^

- **Deprecation Notices**: Advance warning of API changes
- **Compatibility Layers**: Support for older app versions
- **Upgrade Tools**: Automated migration scripts

.. toctree::
   :maxdepth: 4
   :caption: Contents:

   README-Kernel.md
   README-SDK.md
   README-Apps.md

Indices and tables
==================

API Reference
=============

.. doxygenindex::

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
