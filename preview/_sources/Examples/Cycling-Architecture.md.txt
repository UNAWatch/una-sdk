# Cycling - Cycling Activity Tracking

## Overview

The Cycling app is a comprehensive cycling activity tracking application designed for wearable devices, specifically targeting cycling activities. It provides real-time tracking of distance, speed, pace, heart rate, elevation, and other essential metrics for cyclists. The app integrates multiple sensors including GPS, heart rate monitor, barometric pressure sensor, and battery monitoring to deliver accurate and detailed activity data.

The application follows a modular architecture with separate service and GUI components, communicating through a custom message system. It supports features like automatic lap splitting based on distance or time, activity data persistence in FIT file format, and a rich TouchGFX-based user interface with multiple watch faces and screens.

Key features include:
- Real-time GPS tracking with position and speed data
- Heart rate monitoring with trust level assessment
- Elevation tracking using barometric pressure
- Automatic and manual lap recording
- Multiple watch face layouts
- Activity summary with per-lap breakdown
- Battery level monitoring
- Settings for alerts and notifications

## Architecture

The Cycling app follows a client-server architecture pattern where the service component handles all backend logic, sensor management, and data processing, while the GUI component manages user interaction and display. Communication between these components occurs through a message-based system using the UNA SDK's kernel infrastructure.

### High-Level Components

1. **Service Layer**: Core business logic, sensor integration, data processing
2. **GUI Layer**: TouchGFX-based user interface, screen management
3. **SDK Integration**: Kernel, sensor layer, file system, messaging
4. **Data Persistence**: FIT file format for activity data, JSON for settings

### Component Interaction

```
[Hardware Sensors] <-> [Sensor Layer] <-> [Service]
        ^                    ^                ^
        |                    |                |
[Kernel Messages] <-- [Message System] --> [GUI]
```

The service runs as a separate process/thread, continuously processing sensor data and maintaining activity state. The GUI runs on the TouchGFX framework, handling user input and displaying data received from the service.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for activity tracking and sensor management.

### Core Classes and Structures

#### Service Class

The main service class handles all backend logic for activity tracking. It manages sensor connections, processes sensor data, maintains activity state, and communicates with the GUI through the UNA SDK's messaging system. The service runs as a separate process/thread and handles lifecycle management through the kernel interface.

```cpp
class Service
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    SDK::Kernel& mKernel;
    bool         mGuiStarted;
    // ... additional members
};
```

#### Key Data Structures

**Track Data (`Track::Data`)** — real-time metrics snapshot sent to the GUI every second. Contains pace, distance, time, lap counters, HR (current/avg/max/lap), speed (current/avg/max/lap), and elevation — all in SI units (m, m/s, s/m, bpm). Defined in `Libs/Header/Track.hpp`.

### Sensor Integration

Each sensor is represented by an `SDK::Sensor::Connection` object. Polled sensors are configured with a period and latency; event-based sensors fire when the hardware generates an event and ignore those parameters.

**Polled sensors** (1000 ms period / 1000 ms latency):

| Sensor | Type constant | Purpose |
|--------|--------------|---------|
| GPS Location | `GPS_LOCATION` | Coordinates, altitude, fix state; used for map building |
| GPS Speed | `GPS_SPEED` | Instantaneous speed → `mSpeedCounter` |
| GPS Distance | `GPS_DISTANCE` | Incremental distance → `mDistanceCounter` |
| Heart Rate | `HEART_RATE` | BPM + trust level (1–3) → `mHrCounter` |
| Pressure | `PRESSURE` | Barometric altitude (filtered) → `mAltitudeCounter` |
| Battery Metrics | `BATTERY_METRICS` | Voltage → `mBatteryVoltage` |

**Event-based sensors** (period/latency ignored):

| Sensor | Type constant | Purpose |
|--------|--------------|---------|
| Battery Level | `BATTERY_LEVEL` | State of charge → `mBatterySoc` |
| Wrist Motion | `WRIST_MOTION` | Wrist raise → backlight activation |

#### Sensor Connection Management

- GPS and Wrist Motion connected on GUI start (for fix acquisition before tracking begins)
- All remaining sensors connected when tracking starts
- All sensors disconnected when tracking stops

### Data Processing Pipeline

The data processing pipeline transforms raw sensor data into meaningful fitness metrics.

#### 1. Sensor Data Reception

Sensor data arrives through the kernel's message system. The `handleSensorsData()` method identifies the source via `matchesDriver()`, constructs the appropriate parser, and forwards the value:

```cpp
void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data) {
    if (mSensorGpsLocation.matchesDriver(handle)) {
        SDK::SensorDataParser::GpsLocation parser(data[0]);
        if (parser.isDataValid()) {
            mGps.timestamp = parser.getTimestamp();
            mGps.fix = parser.isCoordinatesValid();
            if (mGps.fix) {
                parser.getCoordinates(mGps.latitude, mGps.longitude, mGps.altitude);
            }
        }
    }
    // ... additional sensor handlers
}
```

#### 2. Data Capture Infrastructure

GPS coordinates, battery level, and sensor metrics are collected through three complementary mechanisms that together ensure accurate, pause-aware data:

**GPS state (`mGps`)** — a simple struct updated on every GPS location event: fix flag, latitude/longitude, altitude (m), and timestamp. Used both for map building and for flagging whether coordinate/speed data is valid before writing to FIT.

**Battery samplers** — two `SDK::Metric::ThrottledSample` instances (`mBatterySoc`, `mBatteryVoltage`) that periodically write state-of-charge (%) and voltage (V) into the FIT file without flooding it with redundant records.

**SDK Metric counters** — all sensor values that feed into `Track::Data` pass through one of three SDK counter types from `SDK::Metric`. Counters are the central reason accurate per-lap and per-activity statistics are possible with minimal service logic: each counter automatically excludes time spent in the paused state from active totals, separates per-lap values from session totals via `resetLap()`, and silently rejects sensor anomalies (rollbacks, out-of-range spikes) before they corrupt averages.

| Counter | Suitable for | Used for |
|---------|-------------|---------|
| `MonotonicCounter<T>` | Cumulative values that only increase; ignores decreasing sensor readings | `mTimeCounter`, `mDistanceCounter` |
| `VariableCounter` | Fluctuating values; filters readings outside a valid range; tracks avg/min/max per lap | `mSpeedCounter`, `mHrCounter` |
| `DeltaCounter` | Bidirectional changes; accumulates ascent and descent separately with a noise threshold | `mAltitudeCounter` |

Raw barometric altitude is pre-filtered through a `SimpleLPF` (α = 0.8) before being passed to `mAltitudeCounter`, because `DeltaCounter` does not perform any filtering itself.

#### 3. Parser Classes

Each sensor has a corresponding `SDK::SensorDataParser::*` class. The pattern is always the same: construct the parser from `data[0]`, check `isDataValid()`, then extract the value and feed it into the appropriate counter or struct field. The pressure sensor is slightly more complex because it requires a sea-level calibration on the first reading:

```cpp
SDK::SensorDataParser::Pressure parser(data[0]);
if (parser.isDataValid()) {
    if (!mAltitudeCounter.isValid()) {
        mSeaLevelPressure = parser.getP0();  // calibrate on first reading
    }
    float altitude = parser.getAltitude(parser.getPressure(), mSeaLevelPressure);
    mAltitudeCounter.add(mAltitudeFilter.execute(altitude));  // pre-filter before DeltaCounter
}
```

All other parsers (`GpsLocation`, `GpsSpeed`, `GpsDistance`, `HeartRate`, `BatteryLevel`, `BatteryMetrics`, `WristMotion`) follow the simpler `isDataValid()` → `counter.add(value)` / action pattern.

#### 4. Track Processing Logic

The `processTrack()` method runs every second during active tracking:

```cpp
void Service::processTrack() {
    // GPS map building
    if (mGps.fix && mTrackState == Track::State::ACTIVE) {
        mTrackMapBuilder.addPoint({mGps.latitude, mGps.longitude});
    }

    // Aggregate data for GUI
    mTrackData.totalTime = mTimeCounter.getValueActive();
    mTrackData.distance  = mDistanceCounter.getValueActive();
    mTrackData.speed     = mSpeedCounter.getCurrent();

    // Calculate pace (min/km or min/mile)
    mTrackData.pace = getPace(mTrackData.speed, mSpeedCounter.getMinValid());

    // Update GUI
    SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);

    // FIT file recording
    if (mTrackState == Track::State::ACTIVE) {
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);
    }
}
```

#### 5. FIT File Recording

Activity data is recorded in FIT (Flexible and Interoperable Data Transfer) format, the standard for fitness devices.

Each record is assembled from the current counter values and GPS state. Fields are optional — each is only written to the FIT file when marked valid via `RecordData::set(Field, bool)`. Fields include: coordinates, speed, altitude, heart rate, and battery (both `mBatterySoc` and `mBatteryVoltage` `ThrottledSample` instances must both be `isDue()` before the battery fields are written).

#### 6. Error Handling and Data Validation

**GPS Fix State Management**:

Fix state changes are tracked via `mPreviousGpsFixState`. On every change the GUI is notified with a `GpsFix` message. The very first acquired fix also triggers `notifyFirstFix()` — backlight on, buzzer pattern (150 ms × 3), and a strong vibro click:

```cpp
if (mPreviousGpsFixState != mGps.fix) {
    mPreviousGpsFixState = mGps.fix;
    if (!firstFix) {
        notifyFirstFix();
        firstFix = true;
    }
    SDK::send_msg<CustomMessage::GpsFix>(mKernel, mGps.fix);
}
```

**Heart Rate Trust Level Filtering**:

HR readings are only written to the FIT file when the value is above 20 bpm and the sensor trust level is in the valid range 1–3 (0 means no signal):

```cpp
bool hasHeartRate = (mHrCounter.getCurrent() > 20 &&
                    mTrackData.hrTrustLevel >= 1 &&
                    mTrackData.hrTrustLevel <= 3);
fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
```

**Wrist Motion Backlight Activation**:

```cpp
SDK::SensorDataParser::WristMotion parser(data[0]);
if (parser.isDataValid()) {
    backlightOn();  // brightness 100%, auto-off after skBacklightTimeout (5000 ms)
}
```

`backlightOn()` is a shared helper used across the service (first GPS fix, lap end, wrist motion). It sends a `RequestBacklightSet` message with brightness 100% and the default 5-second auto-off timeout.

### Activity State Management

The service maintains track state through `Track::State` enum:
- `INACTIVE`: No active tracking
- `ACTIVE`: Currently recording activity
- `PAUSED`: Tracking suspended

State transitions are handled by `startTrack()`, `stopTrack()`, `pauseTrack()`.

### Lap Management

Laps can be triggered automatically based on configurable thresholds:
- **Distance-based**: Configurable via `MenuDistanceView` (`Settings::Alerts::Distance::Id`)
- **Time-based**: Configurable via `MenuTimeView` (`Settings::Alerts::Time::Id`)
- **Manual**: User-initiated via R2 button during tracking

Lap data includes timing, distance, average speed, HR, and ascent/descent (written to FIT).

### Settings and Persistence

Settings are stored in JSON format and include:
- Alert distance threshold (`Settings::Alerts::Distance::Id`)
- Alert time threshold (`Settings::Alerts::Time::Id`)
- Auto-pause enable flag (`auto_pause_en`; persisted only — see below)
- Phone notification enablement

#### Auto-pause (hidden in UI)

- **UI:** Auto Pause is not shown on the settings wheel (`ID_AUTO_PAUSE` is removed from `App::MenuNav::Root::Settings::Id`, and `MenuSettingsView` no longer contains auto-pause menu switch cases).
- **Persisted:** `Settings::autoPauseEn` and JSON key `auto_pause_en` are still loaded and saved by `SettingsSerializer`.
- **Runtime:** No service logic reads `autoPauseEn` today; only manual pause/resume (`TRACK_PAUSE` / `pauseTrack()`) is implemented.
- **Rationale:** Auto-pause was never fully implemented; showing the toggle caused confusion. The feature is deferred while other work is prioritized.
- **Re-enabling:** Re-add `ID_AUTO_PAUSE` to `AppMenu.hpp`, restore auto-pause branches in `MenuSettingsView`/`MenuSettingsPresenter`, then implement detection in `Service` gated on `mSettings.autoPauseEn`.

### Activity Data Management and Persistence

#### FIT File Format Implementation

**ActivityWriter** — writes activity data to a FIT file during tracking. Key methods: `start()`, `addRecord()` (called every second), `addLap()`, `pause()`, `resume()`, `stop()`, `discard()`. Each `RecordData` carries an optional-field bitmask so only valid sensor readings are written.

#### Activity Summary Persistence

**ActivitySummarySerializer** — loads and saves `ActivitySummary` as JSON. Used at activity end (`save`) and on next app launch (`load`) to restore the last session for display in the summary screens.

**ActivitySummary Structure**:
```cpp
struct LapSummary {
    time_t duration;  ///< Lap duration in seconds
    float  distance;  ///< Lap distance in m
    float  speed;     ///< Average speed in m/s
};

struct ActivitySummary {
    time_t utc;                       ///< Last activity UTC time
    time_t time;                      ///< Total track time in seconds
    float  distance;                  ///< Total track distance in m
    float  speedAvg;                  ///< Average speed in m/s
    float  elevation;                 ///< Elevation in m
    float  paceAvg;                   ///< Average pace in s/m
    float  hrMax;                     ///< Maximum heart rate in bpm
    float  hrAvg;                     ///< Average heart rate in bpm
    SDK::TrackMapScreen map;          ///< Track map
    std::vector<LapSummary> laps;     ///< Per-lap summary data
};
```

#### Settings Persistence

**SettingsSerializer** — loads and saves `Settings` as JSON. Called on startup (`load`) and whenever the user changes a setting (`save`).

**Settings Structure**:
```cpp
struct Settings {
    bool autoPauseEn  = false;  // persisted; not on settings wheel until feature is implemented
    bool phoneNotifEn = true;
    Alerts::Distance::Id alertDistanceId = Alerts::Distance::ID_OFF;
    Alerts::Time::Id     alertTimeId     = Alerts::Time::ID_OFF;
};
```

#### Data Synchronization

**Real-time GUI Updates**:
- Track data sent every second during active tracking
- Battery level updates on change
- GPS fix status notifications
- Lap completion events

**Persistent Storage**:
- FIT files written continuously during activity
- Summary updated on activity completion
- Settings saved on change

## GUI

The GUI is built using the TouchGFX framework and follows the Model-View-Presenter pattern.

### Project Structure

```
Cycling/Software/Apps/TouchGFX-GUI/
├── CyclingGUI.touchgfx   # TouchGFX Designer project
├── application.config    # Application configuration
├── target.config         # Target hardware settings
├── touchgfx.cmake        # CMake integration
├── gui/                  # Generated and custom GUI code
├── assets/               # Images, fonts, texts
├── generated/            # Auto-generated code
└── simulator/            # Simulator builds
```

### Model-View-Presenter Pattern

The GUI follows MVP architecture:

- **Model**: `Model.hpp/cpp` — Data management and service communication
- **View**: Various view classes (`TrackView`, etc.) — UI rendering
- **Presenter**: Presenter classes — Logic binding model and view

### Model

The Model class (`gui/model/Model.hpp`) serves as the central data hub:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler {
public:
    void bind(ModelListener* listener);
    void tick();
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();

    // Time / date
    void getDate(uint8_t& month, uint8_t& day, uint8_t& weekday);
    void getTime(uint8_t& h, uint8_t& m, uint8_t& s);

    // Sensors & settings
    uint8_t getBatteryLevel() const;
    bool isUnitsImperial() const;
    const uint8_t* getHrThresholds() const;
    uint8_t getHrThresholdsCount() const;
    const Settings& getSettings() const;
    void saveSettings(const Settings& sett);
    bool hasGpsFix() const;

    // Track lifecycle
    void trackStart();
    bool isTrackActive() const;
    void trackPause();
    void trackResume();
    bool isTrackPaused() const;
    const Track::Data& getTrackData() const;
    void saveLap();
    void saveTrack();
    void discardTrack();

    // Summary
    bool isTrackSummaryAvailable() const;
    const ActivitySummary& getTrackSummary() const;
};
```

Key responsibilities:
- Lifecycle management (`onStart`, `onResume`, `onSuspend`, `onStop`)
- Message handling from service (`ICustomMessageHandler`)
- Idle timeout management
- Menu position tracking

### Screens (15 total)

**Entry & Main Menu**:
- `MainView` — App entry point; scroll-wheel menu (Start, Settings); GPS acquisition

**Tracking** (all face cycling happens inside `TrackView` via swipeable containers):
- `TrackView` — Active tracking screen; hosts `TrackFaceTotal`, `TrackFaceLap`, `TrackFaceOverview`, `TrackFaceStatus` (swipe L1/L2)
- `TrackActionView` — Pause menu: Resume / Summary / Save / Discard
- `TrackLapView` — Lap-saved notification (auto-dismisses after 3 s)

**Confirmations**:
- `TrackStartConfirmationView` — Prompt to start without GPS fix; idle timeout → `MainView`
- `TrackHoldConfirmationView` — Hold R1 to confirm finish (→ `TrackSavedView`) or discard (→ `TrackDiscardedView`); release early cancels; idle timeout → `TrackActionView`
- `TrackDiscardedView` — Discard feedback (auto-dismisses after 3 s → exits app)
- `TrackSavedView` — Save feedback (auto-dismisses after 3 s → `TrackSummaryView`)

**Summary** (all face cycling happens inside `TrackSummaryView`):
- `TrackSummaryView` — Post-activity summary; hosts `SummaryFaceMap`, `SummaryFaceOverview`, `SummaryFaceHeartRate`, `SummaryFaceLaps` (swipe L1/L2)
  - `SummaryFaceMap` — route map + total distance
  - `SummaryFaceOverview` — total distance, average speed, elevation, elapsed time
  - `SummaryFaceHeartRate` — max HR, average HR
  - `SummaryFaceLaps` — paginated lap list (5 rows visible, paged by 3 per L1/L2)

**Settings**:
- `MenuSettingsView` — Root settings wheel (Alerts, Phone Notifications; auto-pause hidden — see Auto-pause above)
- `MenuAlertsView` — Alert type selection (Distance, Time)
- `MenuDistanceView` — Distance alert value picker
- `MenuDistanceSavedView` — Save confirmation (auto-dismisses after 3 s → `MenuAlertsView`)
- `MenuTimeView` — Time alert value picker
- `MenuTimeSavedView` — Save confirmation (auto-dismisses after 3 s → `MenuAlertsView`)

### Message Handling System

The Model implements `ICustomMessageHandler` to receive asynchronous updates from the service. Each message is cast to its concrete type, stored in a Model field, and forwarded to the active presenter via `ModelListener`:

```cpp
bool Model::customMessageHandler(SDK::MessageBase *msg) {
    switch (msg->getType()) {
        case CustomMessage::TRACK_DATA_UPDATE: {
            auto *cmsg = static_cast<CustomMessage::TrackDataUpd*>(msg);
            mTrackData = cmsg->data;                      // store
            modelListener->onTrackData(mTrackData);       // notify presenter
        } break;

        // ... other message types follow the same pattern
        default: break;
    }
    return true;
}
```

Messages handled: `SETTINGS_UPDATE`, `LOCAL_TIME`, `BATTERY`, `GPS_FIX`, `TRACK_STATE_UPDATE`, `TRACK_DATA_UPDATE`, `LAP_END`, `SUMMARY`.

### Screen Navigation

All screen changes use `gotoXxxScreenNoTransition()` — there are no slide or animated screen transitions. Animations exist only within containers: `MainMenu` scroll-wheel and `ScrollIndicator` arc movement.

**Screen Flow**:
```
MainView ──[Start, GPS ok]──► TrackView (faces: Total ◄──► Lap ◄──► Overview ◄──► Status)
    │              │                         │
    │    [Start, no GPS fix]                [R1]
    │              │                    TrackActionView
    │    TrackStartConfirmationView   Resume Summary Save Discard
    │      │ [R1:start] → TrackView      │      │     └──┬──┘
    │      │ [idle/R2]  → MainView   TrackView   │   [set Finish/Discard mode]
    │                                            │            │
    │                                            │   TrackHoldConfirmationView
    │                                            │   hold R1 1.5s — arc grows 3→2→1:
    │                                            │   ├ release early → TrackActionView
    │                                            │   └ hold complete →
    │                                            │       [Finish]  TrackSavedView ──(3s)──► TrackSummaryView
    │                                            │       [Discard] TrackDiscardedView ──(3s)──► exitApp()
    │                                     TrackSummaryView
    │                         (faces: Map ◄──► Overview ◄──► HeartRate ◄──► Laps)
    │                                          │
    │                     [back]: paused ──► TrackActionView
    │                             not paused──► exitApp()
    │
    └──[Settings]──► MenuSettingsView
                     (items: Alerts, Phone Notifications toggle)
                          │[Alerts]
                     MenuAlertsView
                       │             │
                  [Distance]       [Time]
                      │               │
              MenuDistanceView   MenuTimeView
                  │ [R1:save]        │ [R1:save]
          MenuDistanceSavedView  MenuTimeSavedView
              │ (3s auto)            │ (3s auto)
          MenuAlertsView         MenuAlertsView
```

### Custom Containers

The app uses two categories of containers: ready-made **SDK widgets** from `Templates/TouchGFX-Widgets` (imported as `.tpkg` packages) and **app-specific containers** built for this app's data and layout.

#### SDK Widget Containers

Imported from `Templates/TouchGFX-Widgets` and used without modification:

- `Battery` — 4-segment battery level indicator
- `Buttons` — button arc indicators (L1/L2/R1/R2)
- `GpsIndicator` — blinking GPS fix status dot
- `HeartRateZone` — 5-zone HR bar visualization
- `ScrollIndicator` — arc position indicator for face and menu navigation
- `MainMenu` — scroll-wheel menu with bundled `ScrollIndicator` and `Toggle`
- `Map` — route map with start/end markers
- `PauseIndicator` — full-width pause overlay with elapsed pause time
- `Title` — screen title with underline
- `InfoCarousel` — auto-cycling multi-value info panel

#### App-Specific Containers

Built specifically for this app and found in `gui/include/gui/containers/`:

**Track watch faces** (swipeable inside `TrackView`):
```cpp
void TrackFaceTotal::setSpeed(float speed, bool isImperial);
void TrackFaceTotal::setDistance(float dist, bool isImperial);
void TrackFaceTotal::setTimer(std::time_t sec);

void TrackFaceLap::setSpeed(float speed, bool isImperial);
void TrackFaceLap::setDistance(float dist, bool isImperial);
void TrackFaceLap::setTimer(std::time_t sec);

void TrackFaceOverview::setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount);
void TrackFaceOverview::setAvgSpeed(float speed, bool isImperial);
void TrackFaceOverview::setElevation(float elevation, bool isImperial);

void TrackFaceStatus::setTime(uint8_t h, uint8_t m);
void TrackFaceStatus::setBatteryLevel(uint8_t level);
```

**Summary faces** (swipeable inside `TrackSummaryView`):
```cpp
void SummaryFaceMap::setDistance(float dist, bool isImperial);
void SummaryFaceMap::setMap(const SDK::TrackMapScreen& map);

void SummaryFaceOverview::setDistance(float dist, bool isImperial);
void SummaryFaceOverview::setAvgSpeed(float speed, bool isImperial);
void SummaryFaceOverview::setElevation(float elevation, bool isImperial);
void SummaryFaceOverview::setTimer(std::time_t sec);

void SummaryFaceHeartRate::setMaxHR(float hr);
void SummaryFaceHeartRate::setAvgHR(float hr);

// Paginated lap list: 5 rows visible, L1/L2 advance by 3 rows per press
void SummaryFaceLaps::setLaps(const std::vector<LapSummary>& laps, bool isImperial);
```

**Utility**:
- `CountdownTimer` — self-registering countdown used by auto-dismiss screens (`TrackLapView`, `TrackSavedView`, `TrackDiscardedView`, `MenuDistanceSavedView`, `MenuTimeSavedView`)

### Input Handling

User input is processed through a hierarchical system:

1. **Hardware Events**: Button presses detected by TouchGFX HAL
2. **Key Event Processing**: `Model::handleKeyEvent()` for global actions
3. **Screen-Specific Handling**: View classes handle context-specific input

**Button Mapping**:
- **L1**: Previous item/navigation left
- **L2**: Next item/navigation right
- **R1**: Primary action (menu access)
- **R2**: Secondary action (lap/manual trigger)

### Data Formatting and Units

The GUI handles unit conversions and formatting before passing values to containers:

- **Distance**: meters → km (metric) or miles (imperial)
- **Speed**: m/s → km/h or mi/h depending on unit setting
- **Pace**: seconds-per-metre → min/km or min/mile depending on unit setting
- **Elevation**: metres → feet in imperial mode
- **Time**: `std::time_t` seconds passed directly to containers; formatted as `H:MM:SS` by the container
- **HR**: raw bpm, no conversion required

### Idle Timeout

The app implements automatic screen timeout to conserve battery:

```cpp
void Model::decIdleTimer() {
    if (mIdleTimer > 0) {
        if (--mIdleTimer == 0) {
            modelListener->onIdleTimeout();
        }
    }
}

void Model::resetIdleTimer() {
    mIdleTimer = App::Config::kScreenTimeoutSteps;
}
```

Idle timer resets on any user interaction, preventing accidental timeouts during active use.

### TouchGFX Integration with UNA SDK

The integration between TouchGFX and UNA SDK is handled through several key components.

#### TouchGFXCommandProcessor

`SDK::TouchGFXCommandProcessor` is a singleton that bridges the SDK kernel and the TouchGFX event loop. It holds a `const SDK::Kernel&` reference internally and manages GUI lifecycle and incoming custom messages.

```cpp
namespace SDK {

class TouchGFXCommandProcessor {
public:
    static TouchGFXCommandProcessor& GetInstance();

    void setAppLifeCycleCallback(SDK::Interface::IGuiLifeCycleCallback* cb);
    void setCustomMessageHandler(SDK::Interface::ICustomMessageHandler* h);

    bool waitForFrameTick();
    bool getKeySample(uint8_t& key);
    void writeDisplayFrameBuffer(const uint8_t* data);

    // Called before Model::tick() on every frame
    void callCustomMessageHandler();
};

} // namespace SDK
```

`FrontendApplication::handleTickEvent()` calls `callCustomMessageHandler()` first, then `model.tick()`, ensuring pending service messages are dispatched to `ModelListener` before the presenter updates the view.

#### Kernel Provider Architecture

`SDK::KernelProviderGUI` is a singleton that stores a pointer to the `SDK::Kernel` object. It is initialised early in the program (before the GUI starts) and later retrieved by the `Model`:

```cpp
namespace SDK {

class KernelProviderGUI {
public:
    static KernelProviderGUI& CreateInstance(const SDK::Kernel* kernel);
    static KernelProviderGUI& GetInstance();

    const SDK::Kernel& getKernel();
};

} // namespace SDK
```

The `Model` constructor retrieves the kernel via `SDK::KernelProviderGUI::GetInstance().getKernel()`, ensuring the GUI has access to the same kernel instance used by the service.

#### Custom Message System

All service↔GUI messages are declared in `Commands.hpp` as plain `constexpr SDK::MessageType::Type` constants (not an enum) with fixed hex IDs. Every message struct inherits from `SDK::MessageBase`:

```cpp
namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type SETTINGS_UPDATE    = 0x00000001;
constexpr SDK::MessageType::Type LOCAL_TIME         = 0x00000002;
constexpr SDK::MessageType::Type BATTERY            = 0x00000003;
constexpr SDK::MessageType::Type GPS_FIX            = 0x00000004;
constexpr SDK::MessageType::Type TRACK_STATE_UPDATE = 0x00000005;
constexpr SDK::MessageType::Type TRACK_DATA_UPDATE  = 0x00000006;
constexpr SDK::MessageType::Type LAP_END            = 0x00000007;
constexpr SDK::MessageType::Type SUMMARY            = 0x00000008;

// GUI --> Service
constexpr SDK::MessageType::Type SETTINGS_SAVE  = 0x0000000A;
constexpr SDK::MessageType::Type TRACK_START    = 0x0000000B;
constexpr SDK::MessageType::Type TRACK_STOP     = 0x0000000C;
constexpr SDK::MessageType::Type TRACK_PAUSE    = 0x0000000D;
constexpr SDK::MessageType::Type TRACK_RESUME   = 0x0000000E;
constexpr SDK::MessageType::Type MANUAL_LAP     = 0x0000000F;

struct SettingsUpd : public SDK::MessageBase {
    Settings settings;
    bool     unitsImperial;
    bool     timeFormat12h;                     // true = 12-hour clock
    uint8_t  hrThresholds[kHrThresholdsCount];  // C-array of 6
    uint8_t  hrThresholdsCount;
};

struct TrackDataUpd : public SDK::MessageBase {
    Track::Data data;
};

// ... other structs follow the same pattern

} // namespace CustomMessage
```

#### Sending Messages

Every message struct keeps a default constructor (which sets its message type) plus, where it has fields, a field-filling constructor that delegates to it. Anything unusual — a `memcpy`, a clamp, a bounded string copy — lives in that constructor rather than at the call site. For `SettingsUpd`, whose fields are listed above:

```cpp
    SettingsUpd()
        : SDK::MessageBase(SETTINGS_UPDATE)
        , unitsImperial(false)
        , timeFormat12h(false)
        , hrThresholds {}
        , hrThresholdsCount(0)
    {}

    explicit SettingsUpd(Settings settings, bool units, bool timeFormat12h,
                         const uint8_t (&thresholds)[kHrThresholdsCount],
                         uint8_t thresholdCount)
        : SettingsUpd()
    {
        this->settings          = settings;
        this->unitsImperial     = units;
        this->timeFormat12h     = timeFormat12h;
        memcpy(this->hrThresholds, thresholds, sizeof(this->hrThresholds));
        this->hrThresholdsCount = thresholdCount;
    }
```

Sending is then one call in either direction. `SDK::send_msg<T>(kernel, args...)` allocates the message from the kernel pool, forwards `args...` to the constructor, sends it, and releases it, returning `false` if allocation or the send failed:

```cpp
// Service --> GUI
SDK::send_msg<CustomMessage::TrackDataUpd>(mKernel, mTrackData);
SDK::send_msg<CustomMessage::LapEnded>(mKernel, mTrackData.lapNum);
SDK::send_msg<CustomMessage::SettingsUpd>(mKernel, mSettings, mIsImperial,
                                          mTimeFormat12h, hrThresholds, hrThresholdsCount);

// GUI --> Service
SDK::send_msg<CustomMessage::TrackStop>(mKernel, /* discard */ true);
SDK::send_msg<CustomMessage::ManualLap>(mKernel);   // zero-field message
```

There is no per-app sender class to write: the message type owns its field filling and `send_msg` owns the allocate/send/release sequence.

`send_msg` is for fire-and-forget sends, and it posts with a zero timeout — if the queue has no room the message is dropped rather than waited for. Reach for `SDK::make_msg<T>()` when the reply matters *or* when you need to wait for queue space: it returns an RAII `MessageGuard` that releases on scope exit, so you can send with a timeout (`msg.send(100)`) and read the result back. `sendInitialInfoToGui()` does this to pick up the system units and clock format before forwarding them to the GUI:

```cpp
if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
    if (msg.send(100) && msg.ok()) {
        mIsImperial    = msg->imperialUnits;
        mTimeFormat12h = msg->timeFormat;
        // ... copy the HR zone thresholds ...
    }
}
```

### Asset Management

**Images**: PNG assets for backgrounds, buttons, icons
**Fonts**: Poppins family for various weights and styles
**Texts**: Localized strings in XML format

### Code Generation

TouchGFX Designer generates:
- Screen base classes (`TrackViewBase`, etc.)
- Container implementations
- Bitmap databases
- Font and text resources

Custom code extends base classes with app-specific logic.

### Simulator Support

The TouchGFX project includes simulator builds for development:
- Visual Studio project for Windows
- Mock sensor data for development
- Visual debugging capabilities

## Build and Setup

The Cycling app uses CMake for cross-platform builds targeting embedded hardware and simulation.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Cycling/Software/Apps/Cycling-CMake/`

```cmake
# App configuration
set(APP_NAME "Cycling")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "A1B7D4E29C8F105A")

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

**External Libraries**:
- TouchGFX framework
- Custom app libraries (ActivityWriter, etc.)

### Build Process

1. **CMake Configuration**: Sets up toolchain and paths
2. **Source Collection**: Gathers all source files
3. **Compilation**: Separate builds for service and GUI
4. **Linking**: Creates ELF executables
5. **Packaging**: Combines into deployable app package

### Development Setup

See [SDK Setup and Build Overview](../sdk-setup.md) for comprehensive development environment setup, build instructions, and toolchain requirements.

### Simulator Build

TouchGFX provides simulator builds for PC development:
- Visual Studio project for Windows
- Makefile for Linux
- Includes mock hardware interfaces

## Conclusion

The Cycling app demonstrates a sophisticated implementation of a cycling activity tracking application on wearable devices. Its modular architecture separates concerns effectively between service logic and user interface, enabling robust sensor integration and real-time data processing.

Key architectural strengths include:
- **Separation of Concerns**: Clear division between service and GUI
- **Message-Based Communication**: Loose coupling between components
- **Extensible Sensor Framework**: Easy addition of new sensors
- **Rich UI Framework**: TouchGFX provides professional user experience
- **Data Persistence**: FIT file format ensures interoperability
