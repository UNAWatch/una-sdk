# Running App Architecture

## Overview

The Running app is a comprehensive fitness tracking application designed for wearable devices, specifically targeting running activities. It provides real-time tracking of distance, speed, pace, heart rate, elevation, and other metrics essential for runners. The app integrates multiple sensors including GPS, heart rate monitor, barometric pressure sensor, and battery monitoring to deliver accurate and detailed activity data.

The application follows a modular architecture with separate service and GUI components, communicating through a custom message system. It supports features like automatic lap splitting based on distance or time, activity data persistence in FIT file format, and a rich TouchGFX-based user interface with multiple watch faces and screens.

Key features include:
- Real-time GPS tracking with position and speed data
- Heart rate monitoring with trust level assessment
- Elevation tracking using barometric pressure
- Automatic and manual lap recording
- Multiple watch face layouts
- Activity summary and history
- Battery level monitoring
- Settings for units, alerts, and notifications

## Architecture

The Running app follows a client-server architecture pattern where the service component handles all backend logic, sensor management, and data processing, while the GUI component manages user interaction and display. Communication between these components occurs through a message-based system using the UNA SDK's kernel infrastructure.

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

The main service class inherits from no base class but implements several interfaces for lifecycle management and messaging.

```cpp
class Service {
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service();
    void run();
private:
    // Implementation details
};
```

#### Key Data Structures

**GPS Data Structure:**
```cpp
struct {
    bool fix;
    float latitude, longitude, altitude;
    uint32_t timestamp;
} mGps;
```

**Track Data Structure:**
```cpp
struct Track::Data {
    std::time_t totalTime;
    std::time_t lapTime;
    float distance;
    float lapDistance;
    float speed;
    float avgSpeed;
    float maxSpeed;
    // ... additional fields
};
```

**Battery Management:**
```cpp
struct {
    SDK::Timer timer;
    float soc, voltage;
    bool isLevelValid, isVoltageValid;
    bool saveRequest;
    void setLevel(float v);
    void setVoltage(float v);
    bool readyToSave();
    void reset();
} mBatteryLevel;
```

### Sensor Integration

The service manages multiple sensor connections:

- **GPS Location**: For position and altitude data
- **GPS Speed**: For instantaneous speed measurements
- **GPS Distance**: For distance calculations
- **Pressure**: For barometric altitude
- **Heart Rate**: For cardiac monitoring
- **Battery Level/Metrics**: For power management
- **Wrist Motion**: For backlight activation

Each sensor is represented by an `SDK::Sensor::Connection` object with specific sampling periods and latencies.

### Data Processing Pipeline

The data processing pipeline is the heart of the service backend, transforming raw sensor data into meaningful fitness metrics.

#### 1. Sensor Data Reception

Sensor data arrives through the kernel's message system. The `handleSensorsData()` method processes each sensor type:

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
            LOG_DEBUG("Location: fix %u, lat %f, lon %f\n", mGps.fix, mGps.latitude, mGps.longitude);
        }
    }
    // ... additional sensor handlers
}
```

Each sensor connection has a `matchesDriver()` method to identify the source of the data batch.

#### 2. Data Filtering and Validation

Raw sensor data undergoes filtering to reduce noise and improve accuracy:

**Altitude Filtering**:
```cpp
SDK::SensorDataParser::Pressure parser(data[0]);
if (parser.isDataValid()) {
    if (!mAltitudeCounter.isValid()) {
        mSeaLevelPressure = parser.getP0();  // Initial calibration
    }
    float altitude = parser.getAltitude(parser.getPressure(), mSeaLevelPressure);
    float filtered = mAltitudeFilter.execute(altitude);  // Low-pass filter
    mAltitudeCounter.add(filtered);
}
```

The `SimpleLPF` uses a configurable alpha value (0.8f) for smoothing altitude changes.

#### 3. Counter System Architecture

The app uses specialized counter classes for different types of measurements:

**MonotonicCounter**: For continuously increasing values (distance, time)
```cpp
SDK::MonotonicCounter<std::time_t> mTimeCounter;
SDK::MonotonicCounter<float> mDistanceCounter;
```

**VariableCounter**: For values that can vary with min/max tracking
```cpp
SDK::VariableCounter mSpeedCounter;  // Tracks current, average, maximum
SDK::VariableCounter mHrCounter;
```

**DeltaCounter**: For elevation changes with ascent/descent calculation
```cpp
SDK::DeltaCounter mAltitudeCounter;
```

Each counter provides methods like `add()`, `getCurrent()`, `getAverage()`, `getMaximum()`, and lap-specific variants.

#### 4. Track Processing Logic

The `processTrack()` method runs every second during active tracking:

```cpp
void Service::processTrack() {
    // GPS map building
    if (mGps.fix) {
        mTrackMapBuilder.addPoint({mGps.latitude, mGps.longitude});
    }

    // Aggregate data for GUI
    mTrackData.totalTime = mTimeCounter.getValueActive();
    mTrackData.distance = mDistanceCounter.getValueActive();
    mTrackData.speed = mSpeedCounter.getCurrent();

    // Calculate pace (min/km or min/mile)
    mTrackData.pace = getPace(mTrackData.speed, mSpeedCounter.getMinValid());

    // Update GUI
    mGuiSender.trackData(mTrackData);

    // FIT file recording
    if (mTrackState == Track::State::ACTIVE) {
        ActivityWriter::RecordData fitRecord = prepareRecordData();
        mActivityWriter.addRecord(fitRecord);
    }
}
```

#### 5. FIT File Recording

Activity data is recorded in FIT (Flexible and Interoperable Data Transfer) format, the standard for fitness devices:

```cpp
ActivityWriter::RecordData Service::prepareRecordData() {
    ActivityWriter::RecordData fitRecord{};

    fitRecord.timestamp = mTimeCounter.getCurrent();
    fitRecord.set(ActivityWriter::RecordData::Field::COORDS, mGps.fix);
    fitRecord.latitude = mGps.latitude;
    fitRecord.longitude = mGps.longitude;

    fitRecord.set(ActivityWriter::RecordData::Field::SPEED, mSpeedCounter.isValid());
    fitRecord.speed = mSpeedCounter.getCurrent();

    fitRecord.set(ActivityWriter::RecordData::Field::ALTITUDE, mAltitudeCounter.isValid());
    fitRecord.altitude = mAltitudeCounter.getCurrent();

    bool hasHeartRate = (mHrCounter.getCurrent() > 20 && mTrackData.hrTrustLevel >= 1);
    fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
    fitRecord.heartRate = mHrCounter.getCurrent();

    fitRecord.set(ActivityWriter::RecordData::Field::BATTERY, mBatteryLevel.readyToSave());
    fitRecord.batteryLevel = static_cast<uint8_t>(mBatteryLevel.getLevel());

    return fitRecord;
}
```

FIT records include optional fields that are only written when valid data is available.

### Activity State Management

The service maintains track state through `Track::State` enum:
- `INACTIVE`: No active tracking
- `ACTIVE`: Currently recording activity
- `PAUSED`: Tracking suspended

State transitions are handled by methods like `startTrack()`, `stopTrack()`, `pauseTrack()`.

### Lap Management

Laps can be triggered automatically or manually:
- **Distance-based**: Configurable distance thresholds
- **Time-based**: Configurable time intervals
- **Manual**: User-initiated via GUI

Lap data includes timing, distance, speed averages, and elevation changes.

### Settings and Persistence

Settings are stored in JSON format and include:
- Unit preferences (imperial/metric)
- Alert configurations (distance/time thresholds)
- Notification settings
- Phone notification enablement

Activity summaries are persisted for historical data.

### Activity Data Management and Persistence

The Running app implements comprehensive data persistence using multiple storage mechanisms.

#### FIT File Format Implementation

**ActivityWriter Class**:
```cpp
class ActivityWriter {
public:
    ActivityWriter(const SDK::Kernel& kernel, const char* activityDir);

    void start(const AppInfo& info);
    void addRecord(const RecordData& record);
    void addLap(const LapData& lap);
    void pause(std::time_t timestamp);
    void resume(std::time_t timestamp);
    void stop(const TrackData& track);
    void discard();

private:
    // FIT file writing implementation
};
```

**Record Data Structure**:
```cpp
struct RecordData {
    enum class Field {
        COORDS = 0,
        SPEED,
        ALTITUDE,
        HEART_RATE,
        BATTERY,
        // ... additional fields
    };

    std::time_t timestamp;
    double latitude, longitude;
    float speed, altitude;
    uint8_t heartRate, batteryLevel;
    uint16_t batteryVoltage;

    void set(Field field, bool enabled) {
        mFields |= (1 << static_cast<uint8_t>(field));
    }

    bool isSet(Field field) const {
        return mFields & (1 << static_cast<uint8_t>(field));
    }

private:
    uint32_t mFields = 0;
};
```

#### Activity Summary Persistence

**ActivitySummarySerializer** handles JSON-based summary storage:

```cpp
class ActivitySummarySerializer {
public:
    ActivitySummarySerializer(const SDK::Kernel& kernel, const char* filename);

    bool load(ActivitySummary& summary);
    bool save(const ActivitySummary& summary);

private:
    const SDK::Kernel& mKernel;
    std::string mFilename;
};
```

**ActivitySummary Structure**:
```cpp
struct ActivitySummary {
    std::time_t utc = 0;
    std::time_t time = 0;        // Active time in seconds
    float distance = 0.0f;       // Total distance in meters
    float speedAvg = 0.0f;       // Average speed m/s
    float elevation = 0.0f;      // Current elevation
    float paceAvg = 0.0f;        // Average pace
    uint8_t hrMax = 0;           // Maximum heart rate
    float hrAvg = 0.0f;          // Average heart rate
    std::vector<uint8_t> map;    // Track map data
};
```

#### Settings Persistence

**SettingsSerializer** manages application configuration:

```cpp
class SettingsSerializer {
public:
    SettingsSerializer(const SDK::Kernel& kernel, const char* filename);

    bool load(Settings& settings);
    bool save(const Settings& settings);

private:
    // JSON serialization implementation
};
```

**Settings Structure**:
```cpp
struct Settings {
    bool phoneNotifEn = true;        // Phone notifications
    float alertDistance = 1.0f;      // Lap distance in km
    uint8_t alertTime = 10;          // Lap time in minutes
    // ... additional settings
};
```

#### File System Integration

The app uses the UNA SDK's file system abstraction:

```cpp
// File operations through kernel
auto file = mKernel.fs.open("Activity/summary.json", SDK::FS::Mode::READ);
if (file) {
    // Read JSON data
    file.close();
}
```

Files are stored in app-specific directories with automatic cleanup and space management.

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

## GUI Implementation

The GUI is built using TouchGFX framework, providing a rich, animated interface for the running app.

### Model-View-Presenter Pattern

The GUI follows MVP architecture:

- **Model**: `Model.hpp/cpp` - Data management and service communication
- **View**: Various view classes (TrackView, etc.) - UI rendering
- **Presenter**: Presenter classes - Logic binding model and view

### Key GUI Components

#### Model Class

The Model class (`gui/model/Model.hpp`) serves as the central data hub:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler {
public:
    void bind(ModelListener *listener);
    void tick();
    void handleKeyEvent(uint8_t c);
    // Track management methods
    void trackStart();
    bool trackIsActive();
    // ... additional methods
};
```

Key responsibilities:
- Lifecycle management (onStart, onResume, onSuspend, onStop)
- Message handling from service
- Idle timeout management
- Menu position tracking

#### View Classes

**TrackView**: Main tracking screen with multiple faces
- TrackFace1: Pace, distance, total time
- TrackFace2: HR, lap pace, lap distance, lap time
- TrackFace3: Time, battery level

**Other Screens**:
- EnterMenu: App entry point
- TrackAction: Pause/resume/stop controls
- TrackSummary: Activity summary display
- Settings screens for configuration

### Message Handling System

The Model implements `ICustomMessageHandler` to receive asynchronous updates from the service:

```cpp
bool Model::customMessageHandler(SDK::MessageBase *msg) {
    switch (msg->getType()) {
        case CustomMessage::SETTINGS_UPDATE: {
            LOG_DEBUG("SETTINGS_UPDATE\n");
            auto *cmsg = static_cast<CustomMessage::SettingsUpd*>(msg);
            mSettings = cmsg->settings;
            mUnitsImperial = cmsg->unitsImperial;
            mHrThresholds = cmsg->hrThresholds;
            modelListener->onSettingsChanged();
        } break;

        case CustomMessage::LOCAL_TIME: {
            auto *cmsg = static_cast<CustomMessage::Time*>(msg);
            std::tm newTime = cmsg->localTime;
            bool dateChanged = (newTime.tm_mday != mTime.tm_mday);
            bool timeChanged = (newTime.tm_hour != mTime.tm_hour ||
                               newTime.tm_min != mTime.tm_min ||
                               newTime.tm_sec != mTime.tm_sec);
            mTime = newTime;
            if (dateChanged) {
                modelListener->onDate(mTime.tm_year + 1900, mTime.tm_mon + 1, mTime.tm_mday, mTime.tm_wday);
            }
            if (timeChanged) {
                modelListener->onTime(mTime.tm_hour, mTime.tm_min, mTime.tm_sec);
            }
        } break;

        case CustomMessage::BATTERY: {
            auto *cmsg = static_cast<CustomMessage::Battery*>(msg);
            if (mBatteryLevel != cmsg->level) {
                mBatteryLevel = cmsg->level;
                modelListener->onBatteryLevel(mBatteryLevel);
            }
        } break;

        case CustomMessage::GPS_FIX: {
            auto *cmsg = static_cast<CustomMessage::GpsFix*>(msg);
            if (mGpsFix != cmsg->state) {
                mGpsFix = cmsg->state;
                modelListener->onGpsFix(mGpsFix);
            }
        } break;

        case CustomMessage::TRACK_STATE_UPDATE: {
            auto *cmsg = static_cast<CustomMessage::TrackStateUpd*>(msg);
            if (mTrackState != cmsg->state) {
                mTrackState = cmsg->state;
                modelListener->onTrackState(mTrackState);
            }
        } break;

        case CustomMessage::TRACK_DATA_UPDATE: {
            auto *cmsg = static_cast<CustomMessage::TrackDataUpd*>(msg);
            mTrackData = cmsg->data;
            modelListener->onTrackData(mTrackData);
        } break;

        case CustomMessage::LAP_END: {
            auto *cmsg = static_cast<CustomMessage::LapEnded*>(msg);
            modelListener->onLapChanged(cmsg->lapNum);
        } break;

        case CustomMessage::SUMMARY: {
            auto *cmsg = static_cast<CustomMessage::Summary*>(msg);
            mpActivitySummary = cmsg->summary;
            modelListener->onActivitySummary(*mpActivitySummary);
        } break;

        default:
            break;
    }
    return true;
}
```

Each message type triggers specific UI updates through the ModelListener interface.

### Screen Navigation and State Management

The app uses TouchGFX's screen management system with custom transitions:

**Screen Flow**:
```
EnterMenu -> TrackView (tracking screens)
    ↓
TrackAction -> TrackSummary
    ↓
Settings screens
```

**Transition Types**:
- **Slide transitions**: Smooth animated screen changes
- **No-transition calls**: Instant updates for data refreshes
- **Modal screens**: Overlays for confirmations and actions

### Custom Containers Implementation

The app uses custom containers for reusable UI components:

**TrackFace1 Container**:
```cpp
// Displays pace, distance, total time
void TrackFace1::setPace(float pace, bool imperial, bool gpsFix);
void TrackFace1::setDistance(float distance, bool imperial, bool gpsFix);
void TrackFace1::setTimer(std::time_t time);
```

**TrackFace2 Container**:
```cpp
// Displays HR, lap metrics
void TrackFace2::setHR(float hr, uint8_t trustLevel, const std::array<uint8_t, 4>& thresholds);
void TrackFace2::setLapPace(float pace, bool imperial, bool gpsFix);
void TrackFace2::setLapDistance(float distance, bool imperial, bool gpsFix);
void TrackFace2::setLapTimer(std::time_t time);
```

**TrackFace3 Container**:
```cpp
// Displays time and battery
void TrackFace3::setTime(uint8_t h, uint8_t m);
void TrackFace3::setBatteryLevel(uint8_t level);
```

### Input Handling Architecture

User input is processed through a hierarchical system:

1. **Hardware Events**: Button presses detected by TouchGFX HAL
2. **Key Event Processing**: Model::handleKeyEvent() for global actions
3. **Screen-Specific Handling**: View classes handle context-specific input
4. **Gesture Recognition**: TouchGFX handles swipe gestures for navigation

**Button Mapping**:
- **L1**: Previous item/navigation left
- **L2**: Next item/navigation right
- **R1**: Primary action (menu access)
- **R2**: Secondary action (lap/manual trigger)

### Data Formatting and Units

The GUI handles unit conversions and formatting:

**Distance Units**:
```cpp
if (isImperial) {
    // Convert meters to miles/feet
    float miles = distance * 0.000621371f;
    // Format display string
} else {
    // Display in kilometers
    float km = distance / 1000.0f;
}
```

**Speed/Pace Calculations**:
```cpp
float pace = (speed > 0.1f) ? (1.0f / speed) : 0.0f;  // min per unit
// Convert to appropriate units based on system setting
```

**Time Formatting**:
```cpp
// Convert seconds to HH:MM:SS or MM:SS
std::string formatTime(std::time_t seconds) {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        return std::format("{}:{:02d}:{:02d}", hours, minutes, secs);
    } else {
        return std::format("{}:{:02d}", minutes, secs);
    }
}
```

### Idle Timeout Management

The app implements automatic screen timeout to conserve battery:

```cpp
void Model::decIdleTimer() {
    if (mIdleTimer > 0) {
        mIdleTimer--;
        if (mIdleTimer == 0) {
            modelListener->onIdleTimeout();
            // Trigger screen off or return to home
        }
    }
}

void Model::resetIdleTimer() {
    mIdleTimer = Gui::Config::kScreenTimeoutSteps;  // Configurable timeout
}
```

Idle timer resets on any user interaction, preventing accidental timeouts during active use.

### User Interaction

- **L1/L2 buttons**: Navigate between watch faces or menu items
- **R1 button**: Access action menus
- **R2 button**: Manual lap recording
- **Wrist motion**: Backlight activation

### Data Display

The GUI displays real-time data with appropriate formatting:
- Speed/pace in current units
- Distance with GPS fix indicators
- Heart rate with trust level visualization
- Battery level with color coding

## Sensor Integration

The Running app integrates multiple sensors through the UNA SDK's sensor layer.

### GPS Integration

**Location Sensor** (`SDK::Sensor::Type::GPS_LOCATION`):
- Provides latitude, longitude, altitude
- Used for position tracking and map building
- Altitude data filtered for stability

**Speed Sensor** (`SDK::Sensor::Type::GPS_SPEED`):
- Instantaneous speed measurements
- Fed into VariableCounter for averaging

**Distance Sensor** (`SDK::Sensor::Type::GPS_DISTANCE`):
- Incremental distance measurements
- Accumulated in MonotonicCounter

### Physiological Sensors

**Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`):
- BPM measurements with trust levels
- Trust levels: 1-3 (higher is better)
- Only valid data (>20 BPM) used for calculations

**Pressure Sensor** (`SDK::Sensor::Type::PRESSURE`):
- Barometric pressure for altitude calculation
- Sea-level pressure calibration
- Filtered altitude data

### System Sensors

**Battery Sensors**:
- Level: State of charge percentage
- Metrics: Voltage measurements
- Periodic logging to FIT files

**Wrist Motion Sensor**:
- Detects movement for backlight activation
- 300ms sampling period

### Sensor Data Processing Architecture

The sensor data processing system is built around specialized parsers and filtering chains.

#### Parser Classes

Each sensor type has a corresponding parser class that handles data extraction and validation:

**GPS Location Parser**:
```cpp
SDK::SensorDataParser::GpsLocation parser(data[0]);
if (parser.isDataValid()) {
    mGps.timestamp = parser.getTimestamp();
    mGps.fix = parser.isCoordinatesValid();
    if (mGps.fix) {
        parser.getCoordinates(mGps.latitude, mGps.longitude, mGps.altitude);
    }
}
```

**GPS Speed Parser**:
```cpp
SDK::SensorDataParser::GpsSpeed parser(data[0]);
if (parser.isDataValid()) {
    mSpeedCounter.add(parser.getSpeed());
}
```

**Pressure Parser with Altitude Calculation**:
```cpp
SDK::SensorDataParser::Pressure parser(data[0]);
if (parser.isDataValid()) {
    if (!mAltitudeCounter.isValid()) {
        mSeaLevelPressure = parser.getP0();  // Initial pressure at sea level
    }
    float pressure = parser.getPressure();
    float altitude = parser.getAltitude(pressure, mSeaLevelPressure);
    float filteredAltitude = mAltitudeFilter.execute(altitude);
    mAltitudeCounter.add(filteredAltitude);
}
```

**Heart Rate Parser**:
```cpp
SDK::SensorDataParser::HeartRate parser(data[0]);
if (parser.isDataValid()) {
    mHrCounter.add(parser.getBpm());
    mTrackData.hrTrustLevel = parser.getTrustLevel();  // 1-3 scale
}
```

#### Advanced Filtering Techniques

**Low-Pass Filtering for Altitude**:
```cpp
class SimpleLPF {
public:
    SimpleLPF(float alpha) : mAlpha(alpha), mFilteredValue(0.0f), mInitialized(false) {}

    float execute(float input) {
        if (!mInitialized) {
            mFilteredValue = input;
            mInitialized = true;
            return input;
        }
        mFilteredValue = mAlpha * input + (1.0f - mAlpha) * mFilteredValue;
        return mFilteredValue;
    }

    void reset() {
        mInitialized = false;
    }

private:
    float mAlpha;
    float mFilteredValue;
    bool mInitialized;
};
```

The altitude filter uses α = 0.8, providing significant smoothing while maintaining responsiveness to actual elevation changes.

#### Counter System Implementation

**MonotonicCounter Template**:
```cpp
template<typename T>
class MonotonicCounter {
public:
    void init() { /* initialization */ }
    void add(T value) { mTotal += value; }
    T getValueActive() const { return mActive; }
    T getValueTotal() const { return mTotal; }
    T getLapValueActive() const { return mLapActive; }

    void resetLap() {
        mLapStart = mTotal;
        mLapActive = 0;
    }

private:
    T mTotal = 0;
    T mActive = 0;
    T mLapStart = 0;
    T mLapActive = 0;
};
```

**VariableCounter for Statistical Tracking**:
```cpp
class VariableCounter {
public:
    void init(float minValid, float maxValid) {
        mMinValid = minValid;
        mMaxValid = maxValid;
    }

    void add(float value) {
        if (value >= mMinValid && value <= mMaxValid) {
            mCurrent = value;
            mSum += value;
            mCount++;
            if (value > mMaximum) mMaximum = value;
            if (value < mMinimum || mMinimum == 0) mMinimum = value;
        }
    }

    float getCurrent() const { return mCurrent; }
    float getAverage() const { return (mCount > 0) ? mSum / mCount : 0; }
    float getMaximum() const { return mMaximum; }
    float getMinimum() const { return mMinimum; }

private:
    float mMinValid, mMaxValid;
    float mCurrent = 0;
    float mSum = 0;
    int mCount = 0;
    float mMaximum = 0;
    float mMinimum = 0;
};
```

#### Sensor Sampling Strategy

**Adaptive Sampling Rates**:
- **GPS Sensors**: 1 Hz sampling for real-time tracking
- **Heart Rate**: 1 Hz for continuous monitoring
- **Pressure**: 1 Hz for altitude tracking
- **Battery**: 1 Hz with periodic FIT logging
- **Wrist Motion**: 3.33 Hz (300ms intervals) for gesture detection

**Sampling Latency**:
- All sensors use 1000ms latency to balance power consumption and responsiveness
- Lower latency (300ms) for wrist motion to ensure immediate backlight activation

#### Error Handling and Data Validation

**GPS Fix State Management**:
```cpp
bool previousFixState = mGps.fix;
mGps.fix = parser.isCoordinatesValid();

if (mPreviousGpsFixState != mGps.fix) {
    mPreviousGpsFixState = mGps.fix;
    if (!firstFix) {
        notifyFirstFix();  // Buzzer, vibration, backlight
        firstFix = true;
    }
    mGuiSender.fix(mGps.fix);
}
```

**Heart Rate Trust Level Filtering**:
```cpp
bool hasHeartRate = (mHrCounter.getCurrent() > 20 &&
                    mTrackData.hrTrustLevel >= 1 &&
                    mTrackData.hrTrustLevel <= 3);
fitRecord.set(ActivityWriter::RecordData::Field::HEART_RATE, hasHeartRate);
```

Only heart rate data with trust levels 1-3 (best to good) are considered valid for recording.

#### Power Management Integration

**Wrist Motion Backlight Activation**:
```cpp
SDK::SensorDataParser::WristMotion parser(data[0]);
if (parser.isDataValid()) {
    auto bl = SDK::make_msg<SDK::Message::RequestBacklightSet>(mKernel);
    if (bl) {
        bl->brightness = 100;
        bl->autoOffTimeoutMs = 5000;
        bl.send();
    }
}
```

Motion detection immediately activates the display with 5-second timeout.

### Sensor Connection Management

Sensors are connected/disconnected based on app state:
- GPS connected on GUI start for fix acquisition
- All sensors connected when tracking starts
- Sensors disconnected when tracking stops

## TouchGFX Port

The Running app uses TouchGFX for its graphical user interface, providing smooth animations and professional UI design.

### TouchGFX Project Structure

```
TouchGFX-GUI/
├── application.config    # Application configuration
├── target.config         # Target hardware settings
├── touchgfx.cmake        # CMake integration
├── gui/                  # Generated and custom GUI code
├── assets/               # Images, fonts, texts
└── generated/            # Auto-generated code
```

### GUI Architecture

**Screens and Transitions**:
- Multiple screens for different app states
- Smooth transitions between screens
- No-transition calls for quick updates

**Custom Containers**:
- TrackFace1, TrackFace2, TrackFace3: Different watch layouts
- BatteryBig: Battery indicator
- HrBar: Heart rate visualization
- Menu components for navigation

### TouchGFX Integration with UNA SDK

The integration between TouchGFX and UNA SDK is handled through several key components:

#### TouchGFXCommandProcessor

This singleton class bridges TouchGFX and the SDK messaging system:

```cpp
class TouchGFXCommandProcessor {
public:
    static TouchGFXCommandProcessor& GetInstance();

    void setAppLifeCycleCallback(SDK::Interface::IGuiLifeCycleCallback* callback);
    void setCustomMessageHandler(SDK::Interface::ICustomMessageHandler* handler);

    // Message processing
    void processMessage(SDK::MessageBase* msg);

private:
    SDK::Interface::IGuiLifeCycleCallback* mLifeCycleCallback = nullptr;
    SDK::Interface::ICustomMessageHandler* mCustomHandler = nullptr;
};
```

The processor receives messages from the SDK kernel and forwards them to the appropriate GUI components.

#### Kernel Provider Architecture

**KernelProviderGUI** provides GUI-specific kernel access:

```cpp
class KernelProviderGUI {
public:
    static KernelProviderGUI& GetInstance();
    const SDK::Kernel& getKernel() const;

private:
    SDK::Kernel mKernel;
};
```

This ensures the GUI has controlled access to kernel services without direct coupling.

#### Custom Message System

The app defines custom message types for service-GUI communication:

```cpp
namespace CustomMessage {
    enum Type {
        SETTINGS_UPDATE = SDK::MessageType::CUSTOM_GUI_MESSAGE_START,
        LOCAL_TIME,
        BATTERY,
        GPS_FIX,
        TRACK_STATE_UPDATE,
        TRACK_DATA_UPDATE,
        LAP_END,
        SUMMARY,
        // ... additional types
    };

    // Message structures
    struct SettingsUpd : SDK::Message::CustomMessageBase {
        Settings settings;
        bool unitsImperial;
        std::array<uint8_t, 4> hrThresholds;
    };

    struct TrackDataUpd : SDK::Message::CustomMessageBase {
        Track::Data data;
    };
}
```

#### Message Sender Classes

**CustomMessage::Sender** provides type-safe message sending:

```cpp
class CustomMessage::Sender {
public:
    Sender(const SDK::Kernel& kernel) : mKernel(kernel) {}

    void settingsUpd(const Settings& s, bool imperial, const std::array<uint8_t, 4>& thresholds) {
        auto msg = SDK::make_msg<SettingsUpd>(mKernel);
        if (msg) {
            msg->settings = s;
            msg->unitsImperial = imperial;
            msg->hrThresholds = thresholds;
            msg.send();
        }
    }

    // ... additional sender methods
};
```

This abstraction ensures proper message allocation and sending.

### Asset Management

**Images**: PNG assets for backgrounds, buttons, icons
**Fonts**: Poppins family for various weights and styles
**Texts**: Localized strings in XML format

### Code Generation

TouchGFX Designer generates:
- Screen base classes (TrackViewBase)
- Container implementations
- Bitmap databases
- Font and text resources

Custom code extends base classes with app-specific logic.

### Simulator Support

The TouchGFX project includes simulator builds for development:
- Windows executable for testing
- Mock sensor data for development
- Visual debugging capabilities

## Build and Setup

The Running app uses CMake for cross-platform builds targeting embedded hardware and simulation.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Running-CMake/`

```cmake
# App configuration
set(APP_NAME "Running")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "A12E9F4C8B7D3A65")

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

**Prerequisites**:
- UNA SDK environment
- CMake 3.21+
- TouchGFX Designer
- Cross-compilation toolchain

**Build Commands**:
```bash
mkdir build
cd build
cmake ..
make
```

### Simulator Build

TouchGFX provides simulator builds for PC development:
- Visual Studio project for Windows
- Makefile for Linux
- Includes mock hardware interfaces

## Conclusion

The Running app demonstrates a sophisticated implementation of a fitness tracking application on wearable devices. Its modular architecture separates concerns effectively between service logic and user interface, enabling robust sensor integration and real-time data processing.

Key architectural strengths include:
- **Separation of Concerns**: Clear division between service and GUI
- **Message-Based Communication**: Loose coupling between components
- **Extensible Sensor Framework**: Easy addition of new sensors
- **Rich UI Framework**: TouchGFX provides professional user experience
- **Data Persistence**: FIT file format ensures interoperability

The implementation showcases advanced C++ patterns, real-time systems programming, and embedded GUI development. The app successfully integrates multiple sensor types, manages complex state transitions, and provides a user-friendly interface for fitness tracking.

Future enhancements could include additional activity types, enhanced sensor fusion algorithms, and expanded social features while maintaining the core architectural principles established in this implementation.