# HRMonitor - Heart Rate Monitor

## Overview

The HRMonitor app is an **Activity**-type application designed for wearable devices. It provides continuous heart rate monitoring with real-time display and persistent data recording in FIT file format. The app connects to a heart rate sensor, shows the current BPM and trust level on screen, and writes a one-second FIT record throughout the session.

The app does not use GPS or any other sensor. All logic is HR-driven: the service connects to the heart rate sensor on startup, waits for the GUI to become active, then records one FIT record per second and forwards every valid sensor reading to the display. If the GUI never starts within 5 seconds, the service exits without saving a file.

Key features include:
- Real-time heart rate display with trust level indicator
- Continuous FIT recording at 1 Hz while the GUI is active
- Session summary with average and maximum heart rate
- Single-screen interface with exit via R2 button
- Automatic exit if the GUI fails to start within 5 seconds

## Architecture

The app is structured as two independent components: a service that handles sensor integration and FIT file recording, and a GUI that displays real-time heart rate data and provides application exit control. Communication between them occurs through a message-based system using the UNA SDK kernel infrastructure.

### High-Level Components

1. **Service Layer**: Heart rate sensor connection, data processing, FIT recording
2. **GUI Layer**: TouchGFX-based single-screen interface, heart rate display
3. **SDK Integration**: Kernel, sensor layer, file system, messaging
4. **Data Persistence**: FIT file format for heart rate activity data

### Component Interaction

```
[Heart Rate Sensor] <-> [Sensor Layer] <-> [Service]
                                               ^
                                               |
                        [Message System] <-> [GUI]
```

The service runs as a separate thread, continuously processing heart rate sensor data and writing FIT records. The GUI runs on the TouchGFX framework, handling user input and displaying heart rate data received from the service.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for heart rate monitoring and FIT recording.

### Core Classes and Structures

#### Service Class

```cpp
class Service
{
public:
    Service(SDK::Kernel& kernel);
    virtual ~Service();
    void run();

private:
    // -- Constants ---
    static constexpr uint32_t skSamplePeriod  = 1000;
    static constexpr uint32_t skSampleLatency = 2000;

    // -- Infrastructure ---
    SDK::Kernel& mKernel;
    bool         mGuiStarted;

    // -- Sensors ---
    SDK::Sensor::Connection mSensorHr;

    // -- Metrics ---
    float mHr;
    float mHrTl;

    // -- Activity ---
    ActivityWriter mActivityWriter;
};
```

### Sensor Integration

The service manages a single sensor connection:

| Sensor | Type constant | Period | Latency | Purpose |
|--------|--------------|--------|---------|---------|
| Heart Rate | `HEART_RATE` | 1000 ms | 2000 ms | BPM + trust level |

The sensor is connected at service startup (before the GUI is ready) and disconnected on exit, so the sensor driver has time to initialise and deliver the first reading by the time the GUI starts.

### Data Processing Pipeline

#### 1. Sensor Data Reception

Sensor data arrives as `EVENT_SENSOR_LAYER_DATA`. The `handleSensorsData()` method identifies the source via `matchesDriver()` and parses the frame:

```cpp
void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data)
{
    if (mSensorHr.matchesDriver(handle)) {
        if (mGuiStarted) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHr   = parser.getBpm();
                mHrTl = parser.getTrustLevel();
                SDK::send_msg<CustomMessage::HRValues>(mKernel, mHr, mHrTl);
            }
        }
    }
}
```

`mHr` and `mHrTl` are only updated while `mGuiStarted` is true. Values remain at 0.0 until the first valid sensor reading after GUI start.

#### 2. Data Validation

The HR parser's `isDataValid()` check rejects frames with out-of-range or missing signal. Trust level (0–3) is forwarded as-is to both the GUI and the FIT file; the app does not gate recording on trust level — all valid sensor frames contribute.

#### 3. FIT File Recording

Heart rate data is recorded every second during active monitoring:

```cpp
time_t utc = time(nullptr);
if (processedUtc != utc) {
    processedUtc = utc;

    ActivityWriter::RecordData fitRecord{};
    fitRecord.timestamp  = utc;
    fitRecord.heartRate  = static_cast<uint8_t>(mHr);
    fitRecord.trustLevel = static_cast<uint8_t>(mHrTl);
    mActivityWriter.addRecord(fitRecord);
}
```

`processedUtc` is initialised to `startTime` (the timestamp passed to `ActivityWriter::start()`), so the first FIT record is written at `startTime + 1s` — strictly after the FIT START event. This prevents the FIT validator from flagging a record at the same timestamp as the START event.

### Activity State Management

The service maintains a single boolean `mGuiStarted` to gate both sensor processing and FIT recording:

- **`onStartGUI()`** — sets `mGuiStarted = true`, sends an initial `HR_VALUES(0.0, 0.0)` to initialise the display.
- **`onStopGUI()`** — sets `mGuiStarted = false`, stops FIT recording.

Recording only occurs while the GUI is active. When `mGuiStarted` is false, sensor data is ignored and no FIT records are written, but the service continues running and listening for `COMMAND_APP_NOTIF_GUI_RUN`.

### Service Lifecycle

The service's `run()` method handles two exit conditions:

**Normal exit — user presses R2:**
1. GUI sends `COMMAND_APP_STOP` → service disconnects sensor, writes Lap + Session, returns.

**Early exit — GUI never starts:**
1. `SDK::Timer guiInitTimeout(TIMER_SECONDS(5))` starts at service entry.
2. If `mGuiStarted` is still false when the timer expires, the service breaks out of the loop and exits without saving a FIT file.

### getMessage Timeout

`mKernel.comm.getMessage(msg, 500)` uses a 500 ms timeout. This ensures the FIT record check (`processedUtc != utc`) runs at least twice per second, maintaining a reliable 1 Hz recording cadence independently of sensor message frequency.

### Activity Statistics

The service accumulates session statistics for the FIT Lap and Session summary messages:

```cpp
float    hrAvgSum   = 0.0f;
uint32_t hrAvgCount = 0;
float    hrMax      = 0.0f;
```

Only samples with `mHr > 1.0f` contribute to the average and maximum, filtering out zero readings that appear before the sensor delivers valid data.

### Persistence (FIT)

FIT file lifecycle is managed by `ActivityWriter`:

| Method | Called when | Writes |
|--------|-------------|--------|
| `start(info)` | Service entry | FileHeader (placeholder), FileID, DeveloperDataID, FieldDescription (`hr_trust_level`), message definitions, START event |
| `addRecord(record)` | Every second while `mGuiStarted` | RECORD with timestamp, heart_rate, hr_trust_level |
| `addLap(lap)` | Service exit | LAP with duration, avg HR, max HR |
| `stop(track)` | Service exit | STOP event, SESSION, ACTIVITY, updated FileHeader, CRC |
| `discard()` | (not used in this app) | Deletes file |

FIT file structure per session:

```
FileHeader (placeholder)
-> FileID -> DeveloperDataID -> FieldDescription(hr_trust_level)
-> [message definitions]
-> Event(START)  @ startTime
-> Record        @ startTime + 1s
-> Record        @ startTime + 2s
-> ...
-> Event(STOP)   @ endTime
-> Session       @ endTime
-> Activity      @ endTime
-> FileHeader    (updated with final data_size)
-> FileCRC
```

Files are stored under `Activity/YYYYMM/activity_YYYYMMDDThhmmss.fit`.

See [Docs/FitFiles-Structure.md](../FitFiles-Structure.md) for FIT implementation details and `ActivityWriter` usage.

## GUI

The GUI is built using the TouchGFX framework and follows the Model-View-Presenter pattern.

### Project Structure

```
HRMonitor/Software/Apps/TouchGFX-GUI/
+-- HRMonitorGUI.touchgfx   # TouchGFX Designer project
+-- application.config      # Application configuration
+-- target.config           # Target hardware settings
+-- touchgfx.cmake          # CMake integration
+-- gui/                    # Custom GUI code
+-- assets/                 # Images, fonts, texts
+-- generated/              # Auto-generated TouchGFX code
+-- simulator/              # Simulator builds
```

### Model-View-Presenter Pattern

- **Model**: `Model.hpp/cpp` — service communication, lifecycle, idle timeout
- **View**: `MainView` — UI rendering and button input
- **Presenter**: `MainPresenter` — logic binding model and view

### Model

The Model class serves as the central data hub:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    void bind(ModelListener* listener);
    void tick();
    void handleKeyEvent(uint8_t key);
    void exitApp();
};
```

Key responsibilities:
- Lifecycle callbacks (`onStart`, `onResume`, `onSuspend`, `onStop`)
- Custom message handling from the service (`HR_VALUES`)
- Idle timeout management (`kScreenTimeoutSteps` = 30 s)
- Application exit via `mKernel.sys.exit()`

### Screens (1 total)

| Screen | Class | Purpose |
|--------|-------|---------|
| Main | `MainView` | Displays current heart rate and trust level; R2 exits |

### Message Handling System

The Model implements `ICustomMessageHandler` to receive heart rate updates from the service:

```cpp
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::HR_VALUES: {
            auto* m = static_cast<CustomMessage::HRValues*>(msg);
            modelListener->updateHR(m->heartRate, m->trustLevel);
        } break;

        default:
            break;
    }
    return true;
}
```

### Screen Navigation

The app uses a single-screen design with no transitions:

```
MainView
    |
[R2: exit] --> exitApp()
```

### Custom Containers

The app uses one SDK widget container from `Templates/TouchGFX-Widgets`:

- `Buttons` — button arc indicators (L1/L2/R1/R2). Only R2 is lit (AMBER) on the main screen.

### Input Handling

**Button Mapping**:

| Button | Action |
|--------|--------|
| R2 | Exit application |

Any button press resets the idle timer.

### Idle Timeout

The app implements automatic screen timeout:
- Timeout: `App::Config::kScreenTimeoutSteps` = 30 seconds at the frame rate
- On timeout, `modelListener->onIdleTimeout()` is called, which triggers `exitApp()`
- Timer resets on any button press (`handleKeyEvent`)

## Custom Message System

All service → GUI messages are declared in `Libs/Header/Commands.hpp`. Message structs inherit from `SDK::MessageBase` and are wrapped in `#pragma pack(push, 4)`.

```cpp
namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;

struct HRValues : public SDK::MessageBase {
    float heartRate;
    float trustLevel;

    HRValues()
        : SDK::MessageBase(HR_VALUES)
        , heartRate(0.0f)
        , trustLevel(0.0f)
    {}

    explicit HRValues(float heartRate, float trustLevel)
        : HRValues()
    {
        this->heartRate  = heartRate;
        this->trustLevel = trustLevel;
    }
};

} // namespace CustomMessage
```

Each message keeps a default constructor (which sets its message type) and a field-filling constructor that delegates to it. The field-filling constructor is what makes the one-line send below possible.

**Message summary**:

| Message | Direction | Trigger |
|---------|-----------|---------|
| `HR_VALUES` | Service -> GUI | Every valid sensor reading while `mGuiStarted` is true; also once on GUI start with (0.0, 0.0) |

### Sending Messages

Fire-and-forget sends go through `SDK::send_msg<T>()`, which allocates the message from the kernel pool, forwards its arguments to the message's constructor, sends it, and releases it — returning `false` if allocation or the send failed:

```cpp
SDK::send_msg<CustomMessage::HRValues>(mKernel, mHr, mHrTl);
```

There is no per-app sender class to write: the message type carries the field-filling constructor and `send_msg` supplies the rest. That is the only send this app needs — every HRMonitor message is fire-and-forget.

When the reply matters, `SDK::make_msg<T>()` is the counterpart: it returns an RAII `MessageGuard` that releases on scope exit, so you can send with a timeout and read the result back (`msg.send(timeout) && msg.ok()`). The Cycling, Hiking, Running, and Alarm services use it that way to query `SDK::Message::RequestSystemSettings` — see [Cycling](Cycling-Architecture.md#sending-messages) for a worked example.

## Build and Setup

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `HRMonitor/Software/Apps/HRMonitor-CMake/`

```cmake
# App configuration
set(APP_NAME "HRMonitor")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "A14B7E9C2F8D6A53")

# Include SDK build scripts
include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
```

### Build Targets

**Service Build**:
```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
)
una_app_build_service(${APP_NAME}Service.elf)
```

**GUI Build**:
```cmake
set(GUI_SOURCES
    ${TOUCHGFX_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI}
)
una_app_build_gui(${APP_NAME}GUI.elf)
```

**Complete App**:
```cmake
una_app_build_app()
```

### Dependencies

**SDK Components**:
- UNA SDK common, service, and GUI sources
- Sensor layer interfaces
- Kernel and messaging systems

**App Libraries** (`Libs/`):
- `Service` — service entry point, sensor handling, FIT recording coordination
- `ActivityWriter` — FIT file lifecycle management
- `Commands.hpp` — shared message types for the service↔GUI contract

### Development Setup

See [SDK Setup and Build Overview](../sdk-setup.md) for comprehensive development environment setup, build instructions, and toolchain requirements.

### Simulator Build

The TouchGFX project includes a Visual Studio simulator build located in `simulator/msvs/Application.vcxproj`. The `ConfigurationSimulator.hpp` file controls which sensors are simulated; `HEAT_RATE_SIM_ENABLE 1` activates the HR sensor simulator with configurable BPM range (`HEAT_RATE_SIM_MIN_HR` / `HEAT_RATE_SIM_MAX_HR`).

## Conclusion

The HRMonitor app is the simplest activity-recording UNA application: a single screen, a single sensor, and a single message type. Its minimal footprint makes it a clear reference for the SDK's core patterns — sensor subscription, service-to-GUI messaging, FIT recording, and MVP-based display.

Key architectural strengths include:
- **Separation of concerns** — Service owns all sensor logic and FIT recording; GUI owns only display and exit
- **Minimal message surface** — A single `HR_VALUES` message type covers the entire Service -> GUI data path
- **Sensor abstraction** — `SDK::Sensor::Connection` and `SensorDataParser::HeartRate` isolate the app from driver details
- **FIT persistence** — `ActivityWriter` handles the full recording lifecycle (open, record, lap, session, CRC) independently of the service loop
