# Development Workflow

This document outlines the complete development lifecycle for Una-Watch applications.

## Workflow Stages

### 1. Planning
Before writing code, decide on your app type:
- **Activity**: For long-running, interactive apps.
- **Utility**: For tools like calculators or settings.
- **Glance**: For notification-based widgets.
- **Clockface**: For time-keeping displays.

### 2. Development
Use the SDK interfaces to build your app logic.
- Implement the **Service process** for background tasks.
- Implement the **GUI process** for user interaction.
- Use **IPC messages** to communicate between them.

### 3. Building
Compile your app into an ELF binary using the provided toolchain.
```bash
make release
```
Ensure you are using the correct optimization flags for PIC execution.

### 4. Testing
- **Simulator**: Test UI and basic logic on your development machine.
- **Unit Tests**: Write tests for individual components.
- **Integration Tests**: Verify IPC and sensor data flow.

### 5. Debugging
- Use `ILogger` for real-time logging.
- Set breakpoints in the simulator.
- Use performance profiling to monitor CPU and memory usage.

### 6. Packaging
Package your ELF binary and assets into a `.uapp` container using `app_packer.py`.
- Inject version metadata.
- Embed app icons.

### 7. Deployment
Deploy your app to the watch via:
- **USB**: For direct development flashing.
- **BLE OTA**: For wireless updates.

### 8. Maintenance
- Monitor crash reports.
- Release version updates.
- Respond to user feedback.

## Technical Architecture Details

### Application Framework Implementation

**AppInstance Architecture:**

**Dual-Process Model:**
- **Service Component**: Background logic, sensor access, data processing
- **GUI Component**: User interface, TouchGFX integration, screen rendering

**Lifecycle State Machine:**
```cpp
enum AppPartState {
    NONE,       // Not loaded
    RUN,        // Running normally
    RESUMED,    // GUI active and visible
    GLANCE,     // Glance mode (notification display)
    SUSPENDED   // Backgrounded
};
```

**Message Processing Thread:**
- Dedicated OS thread with cancellable event loop
- Handler dispatch: lifecycle, app-specific, sensor layer messages
- Automatic queue clearing on termination to prevent memory leaks

**Dynamic Loading Mechanism:**
- ELF parsing with `Loader::ReadMainHeader()` for .uapp files
- Memory-mapped execution with `mSrvLoader.load()` and `mGuiLoader.load()`
- Offset-based loading: GUI loads after service + icons in .uapp structure

**Sensor Integration:**
- **SDK::Sensor::RequestDefault** for default sensor discovery
- **SDK::Sensor::RequestConnect** with period/latency negotiation
- Listener registration for service vs GUI process isolation

#### Application Framework Dual-Process Architecture

```mermaid
graph TD
    subgraph "Application Package (.uapp)"
        UAPP[.uapp File<br/>Container]
        SERVICE_ELF[Service ELF<br/>Background Logic]
        GUI_ELF[GUI ELF<br/>User Interface]
        METADATA[Metadata<br/>Icons/Config]
    end

    subgraph "Runtime Processes"
        SERVICE_PROC[Service Process<br/>PID: Service_PID]
        GUI_PROC[GUI Process<br/>PID: GUI_PID]
    end

    subgraph "Kernel Interfaces"
        KERNEL_SDK[SDK Interfaces<br/>IAppComm/ISystem/IFileSystem]
        MESSAGE_POOLS[Message Pools<br/>Events/Requests/Commands]
        SENSOR_LAYER[Sensor Layer<br/>Hardware Access]
        FILESYSTEM[FileSystem<br/>Storage Access]
    end

    subgraph "Inter-Process Communication"
        DUAL_APP_COMM[DualAppComm<br/>IPC Bridge]
        MSG_QUEUES[Message Queues<br/>20-slot Circular]
        EVENT_DISPATCH[Event Dispatch<br/>Type-safe Routing]
    end

    UAPP --> SERVICE_PROC
    UAPP --> GUI_PROC

    SERVICE_PROC --> KERNEL_SDK
    GUI_PROC --> KERNEL_SDK

    KERNEL_SDK --> MESSAGE_POOLS
    KERNEL_SDK --> SENSOR_LAYER
    KERNEL_SDK --> FILESYSTEM

    SERVICE_PROC --> DUAL_APP_COMM
    GUI_PROC --> DUAL_APP_COMM

    DUAL_APP_COMM --> MSG_QUEUES
    DUAL_APP_COMM --> EVENT_DISPATCH

    style SERVICE_PROC fill:#bbdefb
    style GUI_PROC fill:#c8e6c9
    style DUAL_APP_COMM fill:#fff3e0
```

#### Dynamic Loading and Memory Management Flow

```mermaid
flowchart TD
    A[App Package<br/>.uapp File] --> B[App::Manager::scan<br/>Package Discovery]

    B --> C{ELF Validation}
    C -->|Valid| D[Loader::ReadMainHeader<br/>Parse ELF Sections]
    C -->|Invalid| E[Error: Corrupt Package]

    D --> F[Memory Allocation<br/>Protected Memory Regions]
    F --> G{Service ELF<br/>Load First}

    G -->|Yes| H[Service Loader<br/>Load ELF + Relocate]
    G -->|No| I[GUI Loader<br/>Load ELF + Relocate<br/>Offset from Service]

    H --> J[Thread Creation<br/>Service Process Thread]
    I --> K[Thread Creation<br/>GUI Process Thread]

    J --> L[Kernel Registration<br/>PID Assignment]
    K --> L

    L --> M[Interface Binding<br/>SDK Kernel Access]
    M --> N[AppInstance Ready<br/>Lifecycle Management]

    N --> O{App State<br/>Transition}
    O -->|Start| P[InternalAppRunService<br/>Background Processing]
    O -->|Resume| Q[InternalAppRun<br/>GUI Active]
    O -->|Glance| R[InternalAppGlanceRun<br/>Notification Mode]
    O -->|Suspend| S[GUI Suspend<br/>Service Only]
    O -->|Stop| T[InternalAppStop<br/>Cleanup]

    T --> U[Memory Deallocation<br/>Thread Termination]
    U --> V[App Unloaded]

    style B fill:#bbdefb
    style N fill:#c8e6c9
    style V fill:#fff3e0
```

#### Application Message Processing Architecture

```mermaid
graph TD
    subgraph "Message Sources"
        KERNEL_EVENTS[Kernel Events<br/>System/App/BLE/Timer]
        USER_INPUT[User Input<br/>Buttons/Touch/Gestures]
        SENSOR_DATA[Sensor Data<br/>Periodic/Event-driven]
        SYSTEM_REQUESTS[System Requests<br/>Settings/Battery/Status]
    end

    subgraph "Message Classification"
        EVENTS[Events<br/>0x03010000-0x03040000<br/>Fire-and-forget]
        REQUESTS[Requests<br/>0x01070000-0x020A0000<br/>With Response]
        COMMANDS[Commands<br/>0x01010000-0x01060000<br/>With Confirmation]
    end

    subgraph "Processing Pipeline"
        MSG_RECEIVE[Message Receive<br/>getMessage Timeout]
        TYPE_DISPATCH[Type Dispatch<br/>std::visit Pattern]
        HANDLER_EXEC[Handler Execution<br/>App-specific Logic]
        RESPONSE_SEND[Response Send<br/>sendResponse Completion]
    end

    subgraph "App Components"
        SERVICE_LOGIC[Service Logic<br/>Background Processing]
        GUI_LOGIC[GUI Logic<br/>UI Updates]
        STATE_MGMT[State Management<br/>Lifecycle Transitions]
    end

    KERNEL_EVENTS --> EVENTS
    USER_INPUT --> REQUESTS
    SENSOR_DATA --> EVENTS
    SYSTEM_REQUESTS --> REQUESTS

    EVENTS --> MSG_RECEIVE
    REQUESTS --> MSG_RECEIVE
    COMMANDS --> MSG_RECEIVE

    MSG_RECEIVE --> TYPE_DISPATCH
    TYPE_DISPATCH --> HANDLER_EXEC
    HANDLER_EXEC --> RESPONSE_SEND

    HANDLER_EXEC --> SERVICE_LOGIC
    HANDLER_EXEC --> GUI_LOGIC
    HANDLER_EXEC --> STATE_MGMT

    SERVICE_LOGIC -.->|Data Processing| HANDLER_EXEC
    GUI_LOGIC -.->|UI Feedback| HANDLER_EXEC
    STATE_MGMT -.->|State Changes| HANDLER_EXEC

    style MSG_RECEIVE fill:#bbdefb
    style TYPE_DISPATCH fill:#c8e6c9
    style RESPONSE_SEND fill:#fff3e0
```

#### Sensor Integration and Data Flow

```mermaid
flowchart TD
    A[App Sensor Request] --> B{SDK::Sensor::RequestDefault}

    B -->|Heart Rate| C[Sensor::Type::HEART_RATE]
    B -->|Accelerometer| D[Sensor::Type::ACCELEROMETER]
    B -->|Other| E[Sensor::Type::*]

    C --> F[Sensor::Manager::getSensorList]
    D --> F
    E --> F

    F --> G{Sensor Available?}
    G -->|Yes| H[Return Sensor Handle<br/>1-based Index]
    G -->|No| I[Error: No Sensor<br/>Available]

    H --> J[SDK::Sensor::RequestConnect]
    J --> K[Handle Assignment<br/>Period Negotiation]
    K --> L[Latency Configuration<br/>ms/microseconds]

    L --> M[Listener Registration<br/>Service vs GUI Process]
    M --> N[Sensor Layer<br/>Connection Established]

    N --> O[Periodic Sampling<br/>Event Loop]
    O --> P[Data Acquisition<br/>Sensor Driver]
    P --> Q[Data Processing<br/>Calibration/Filtering]

    Q --> R[Callback Dispatch<br/>onSensorUpdate]
    R --> S{Process Type}

    S -->|Service| T[Service Process<br/>Background Logic]
    S -->|GUI| U[GUI Process<br/>UI Updates]

    T --> V[Data Storage<br/>Analysis/Trends]
    U --> W[Display Update<br/>Real-time Feedback]

    V --> X[Persistent Storage<br/>File System]
    W --> Y[TouchGFX Render<br/>Screen Update]

    style A fill:#e1f5fe
    style N fill:#c8e6c9
    style R fill:#fff3e0
```

## App Development Framework

### App Architecture & Lifecycle

**Dual-Process Model:**
- **Service Process**: Background logic, sensor access, data processing, glance notifications
- **GUI Process**: User interface, TouchGFX integration, screen rendering, user interaction

**App Types:**
- **Activity**: Full-screen GUI app with service background processing
- **Utility**: Specialized function app (calculator, settings, etc.)
- **Glance**: Notification-only app without GUI (240x60 pixel display)
- **Clockface**: Watch face app with time display

**Lifecycle States:**
```cpp
enum AppPartState {
    NONE,       // Not loaded
    RUN,        // Running normally
    RESUMED,    // GUI active and visible
    GLANCE,     // Glance mode (notification display)
    SUSPENDED   // Backgrounded
};
```

#### App Lifecycle & Management

```mermaid
stateDiagram-v2
    [*] --> Created
    Created --> Initialized
    Initialized --> ServiceRunning
    ServiceRunning --> FullRunning
    FullRunning --> ServiceOnly
    ServiceOnly --> FullRunning
    FullRunning --> GlanceMode
    GlanceMode --> ServiceOnly

    ServiceRunning --> Stopped
    ServiceOnly --> Stopped
    GlanceMode --> Stopped
    Stopped --> Destroyed
    Destroyed --> [*]

    Created : App Manager addApp()
    Initialized : AppInstance init()
    ServiceRunning : InternalAppRunService
    FullRunning : InternalAppRun (GUI)
    GlanceMode : InternalAppGlanceRun
    ServiceOnly : GUI Suspend
    Stopped : InternalAppStop
    Destroyed : AppInstance deinit()
    Destroyed --> [*]

    note right of Created
        .uapp file loaded
        ELF parsed
        Memory allocated
    end note

    note right of Initialized
        Thread created
        Message queues ready
        Kernel registered
    end note

    note right of ServiceRunning
        Service ELF loaded
        Background processing
        Sensor connections
    end note

    note right of FullRunning
        GUI ELF loaded
        TouchGFX active
        User interaction
    end note

    note right of GlanceMode
        Notification display
        240x60 pixel canvas
        Low power mode
    end note
```

### SDK Interfaces for App Development

**Core Interfaces Available to Apps:**

**1. IAppComm (Communication)**
- Message passing between app and kernel
- Type-safe message allocation from pools
- Process identity with unique PIDs

**2. ISystem (System Services)**
- Timing functions and delays
- Battery status and power management
- Device identification

**3. IFileSystem (Storage)**
- File operations on multiple volumes (0:/, 1:/, 2:/)
- Directory navigation and content listing

**4. ILogger (Debugging)**
- Formatted logging with multiple levels
- System timestamp integration

### Message System Architecture

**Message Type Ranges:**
- **Custom/App-specific** (0x00000000-0x0000FFFF): Application-specific internal communication
- **Commands** (0x01010000-0x01060000): Kernel-to-app directives (response expected)
- **Requests** (0x01070000-0x020A0000): App-to-kernel lifecycle and hardware requests
- **Events** (0x03010000-0x03040000): System-level notifications (fire-and-forget)
- **Sensors** (0x03100000-0x03180000): Sensor discovery and data events

**Key System Messages:**
```cpp
REQUEST_BATTERY_STATUS       // Get battery level
REQUEST_SYSTEM_SETTINGS      // Get watch settings
REQUEST_DISPLAY_CONFIG       // Get screen dimensions (GUI only)
REQUEST_BACKLIGHT_SET        // Set screen brightness
REQUEST_SENSOR_LAYER_CONNECT // Start sensor sampling
```

### Sensor Layer Integration

**Sensor Access Pattern:**
```cpp
// Modern way using SDK::Sensor::Connection wrapper
SDK::Sensor::Connection hrSensor(SDK::Sensor::Type::HEART_RATE, 1000.0f);
hrSensor.connect();

// Manual way using messages
auto msg = SDK::make_msg<SDK::Message::Sensor::RequestDefault>(kernel);
msg->id = SDK::Sensor::Type::HEART_RATE;
if (msg.send(100) && msg.ok()) {
    uint32_t handle = msg->handle;
    // ... use handle to connect
}
```

#### Sensor Data Flow Architecture

```mermaid
graph TD
    subgraph "Hardware Sensors"
        PPG[PPG Sensor<br/>Heart Rate]
        IMU[IMU Sensor<br/>Accel/Gyro]
        BARO[Barometer<br/>Pressure/Altitude]
        TEMP[Temperature<br/>Ambient]
    end

    subgraph "Sensor Drivers"
        PPG_DRV[PPG Driver<br/>PAH8316LS]
        IMU_DRV[IMU Driver<br/>BMI270]
        BARO_DRV[Barometer Driver<br/>MS5837]
        TEMP_DRV[Temp Driver<br/>Internal]
    end

    subgraph "Sensor Manager"
        SM[Sensor::Manager<br/>Coordination Hub]
        SM_THREAD[Sensor Thread<br/>Polling Loop]
        SM_QUEUE[Data Queue<br/>Sample Buffer]
    end

    subgraph "App Integration"
        APP_SENSOR_LISTENER[Sensor Listener<br/>App Callback]
        APP_DATA_QUEUE[App Data Queue<br/>Processed Samples]
        APP_SERVICE[App Service<br/>Data Processing]
    end

    PPG --> PPG_DRV
    IMU --> IMU_DRV
    BARO --> BARO_DRV
    TEMP --> TEMP_DRV

    PPG_DRV --> SM
    IMU_DRV --> SM
    BARO_DRV --> SM
    TEMP_DRV --> SM

    SM --> SM_THREAD
    SM_THREAD --> SM_QUEUE

    SM_QUEUE --> APP_SENSOR_LISTENER
    APP_SENSOR_LISTENER --> APP_DATA_QUEUE
    APP_DATA_QUEUE --> APP_SERVICE

    SM -.->|Period Negotiation| PPG_DRV
    SM -.->|Latency Control| IMU_DRV
    SM -.->|Handle Assignment| BARO_DRV

    style SM fill:#bbdefb
    style SM_THREAD fill:#c8e6c9
    style APP_SERVICE fill:#fff3e0
```

### App Development Workflow

**1. Project Structure Setup:**
```
AppName/
├── Software/
│   ├── App/
│   │   └── AppName-CubeIDE/          # STM32CubeIDE project
│   │       ├── Core/
│   │       │   ├── Inc/main.h
│   │       │   └── Src/main.cpp      # App entry point
│   │       └── SDK/                  # SDK headers
│   └── Libs/
│       ├── Header/Service.hpp        # Service class
│       └── Source/Service.cpp        # Service implementation
├── Resources/
│   ├── icon_30x30.png               # Small app icon
│   └── icon_60x60.png               # Large app icon
└── Output/Release/                  # Built .uapp files
```

**2. Service Implementation:**
```cpp
class Service : public SDK::Interface::IApp::Callback,
                public SDK::Interface::IGlance {
private:
    SDK::Kernel& mKernel;
    bool mTerminate = false;

public:
    Service(SDK::Kernel& kernel) : mKernel(kernel) {
        // Register this instance as a lifecycle callback
        // Note: IApp interface must be queried via IKIP if not passed directly
    }

    void run() {
        while (!mTerminate) {
            // Main service loop
            mKernel.sys.delay(1000);

            // Handle messages
            SDK::MessageBase* msg = nullptr;
            if (mKernel.comm.getMessage(msg, 0)) {
                // ... handle message
                mKernel.comm.releaseMessage(msg);
            }
        }
    }

    // Lifecycle callbacks
    void onCreate() override { /* Initialize */ }
    void onStart() override { /* Start processing */ }
    void onStop() override { mTerminate = true; }
};
```

**3. Message Handling:**
```cpp
bool getMessage(MessageBase*& msg, uint32_t timeout) {
    return mKernel.comm.getMessage(msg, timeout);
}

void sendResponse(MessageBase* msg) {
    mKernel.comm.sendResponse(msg);
}
```

### App Capabilities System

**Permission-Based Features:**
- **Phone Notifications**: iOS ANCS integration
- **USB Charging Screen**: Custom charging UI
- **Music Control**: Media playback integration

### App Deployment

**OTA Update Process:**
1. File transfer via BLE to `2:/Update/` directory
2. CRC and signature verification
3. Atomic replacement with rollback capability
4. System reboot to activate new version

#### App Development Data Flow

```mermaid
graph LR
    subgraph "App Development"
        DEV[Developer<br/>Writes Code]
        SDK[SDK Headers<br/>Interfaces]
        BUILD[STM32CubeIDE<br/>Build Process]
        PACK[Post-build Scripts<br/>app_packer.py]
    end

    subgraph "App Package"
        UAPP[.uapp File<br/>ELF + Icons + Metadata]
        HEADER[MainHeader<br/>App Info]
        SERVICE[Service ELF<br/>Background Logic]
        GUI[GUI ELF<br/>User Interface]
        ICONS[Icons<br/>30x30 + 60x60 PNG]
    end

    subgraph "Runtime Loading"
        MANAGER[App::Manager<br/>Package Scanner]
        LOADER[App Loader<br/>ELF Parser]
        MEMORY[Memory Allocator<br/>Protected Regions]
        KERNEL[Kernel Interfaces<br/>SDK Bridge]
    end

    subgraph "Execution"
        SRV_PROCESS[Service Process<br/>PID Assigned]
        GUI_PROCESS[GUI Process<br/>PID Assigned]
        COMM[DualAppComm<br/>IPC Bridge]
        MESSAGES[Message System<br/>Kernel Communication]
    end

    DEV --> SDK
    SDK --> BUILD
    BUILD --> PACK
    PACK --> UAPP

    UAPP --> HEADER
    UAPP --> SERVICE
    UAPP --> GUI
    UAPP --> ICONS

    MANAGER --> LOADER
    LOADER --> MEMORY
    MEMORY --> KERNEL

    KERNEL --> SRV_PROCESS
    KERNEL --> GUI_PROCESS
    SRV_PROCESS --> COMM
    GUI_PROCESS --> COMM
    COMM --> MESSAGES

    style DEV fill:#e1f5fe
    style UAPP fill:#c8e6c9
    style SRV_PROCESS fill:#fff3e0
    style GUI_PROCESS fill:#fff3e0
```

This framework enables **independent app development** with clean SDK interfaces, resource management, and comprehensive tooling support.

### Building Individual Apps

1. Open STM32CubeIDE
2. Import the CubeIDE project: `File > Import > Existing Projects into Workspace`
3. Select the `.cproject` file in `Apps/<AppName>/Software/App/<AppName>-CubeIDE/`
4. Build the project (Project > Build All)
5. The post-build script will automatically generate a `.uapp` file in `Output/Release/`

### App Installation

- Apps are installed as `.uapp` files
- Deploy via the watch's companion app or development interface
- Apps can be updated independently of the main firmware