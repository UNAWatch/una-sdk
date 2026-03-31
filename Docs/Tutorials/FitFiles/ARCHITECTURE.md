# FitFiles - FIT File Creation and Sensor Logging Tutorial

## Overview

The FitFiles app is a focused tutorial demonstrating FIT (Flexible and Interoperable Data Transfer) file creation and sensor data logging on wearable devices. This app showcases how to collect heart rate and step counter data during glance sessions and store it in the industry-standard FIT format using the UNA SDK's FitHelper components.

The application implements a glance-triggered recording service that monitors heart rate and steps data during active glance sessions, accumulates step counts, and writes them to FIT files with custom developer fields. It demonstrates core concepts of session-based recording, event-driven data collection, and FIT file structure creation with a simplified, tutorial-friendly approach.

Key features include:
- Real-time heart rate and step counting during glance sessions
- FIT file creation with proper headers, definitions, and CRC validation
- Custom developer fields for extended data types
- Activity session management triggered by glance start/stop events
- Session-based data persistence
- Glance UI displaying both current heart rate and step count
- Automatic data persistence on session completion

## Architecture

The FitFiles app follows a service-only architecture pattern typical of glance applications, where all functionality resides in the service component with a minimal UI for data display. The service handles sensor integration, data processing, FIT file management, and communication with the glance interface. Unlike continuous monitoring apps, this application is event-driven, activating recording only during active glance sessions.

### High-Level Components

1. **Service Layer**: Core business logic, sensor integration, FIT file creation, data persistence
2. **Glance UI**: Simple display interface showing current heart rate and step count
3. **SDK Integration**: Kernel, sensor layer, file system, FIT helper utilities
4. **Data Persistence**: FIT file format with custom developer fields

### Component Interaction

```
[Hardware Sensors] <-> [Sensor Layer] <-> [Service]
                                      ^
                                      |
                            [Glance Interface]
```

The service runs as a separate process/thread, processing sensor data only during active glance sessions and maintaining session-based activity data. The glance UI provides a simple display for the current heart rate and accumulated step data during the session.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for heart rate and step tracking during glance sessions and FIT file management.

### Core Classes and Structures

#### Service Class

The main service class handles all backend logic for heart rate and step counting during glance sessions and FIT file creation. It manages sensor connections, processes heart rate and step data, maintains activity state, and handles file I/O operations through the UNA SDK's kernel interface.

```cpp
class Service : public SDK::Interface::ISensorDataListener
{
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service();
    void run();

private:
    // ===== SENSOR MANAGEMENT =====
    void connect();
    void disconnect();

    // ISensorDataListener implementation
    void onSdlNewData(uint16_t handle, const SDK::Sensor::Data* data, uint16_t count, uint16_t stride) override;

    // ===== GLANCE UI =====
    void onGlanceTick();
    bool configGui();
    void createGuiControls();

    // ===== FIT FILE MANAGEMENT =====
    void saveFit(bool finalize);
    void appendPendingRecords(SDK::Interface::IFile* fp);
    void writeFitDefinitions(SDK::Interface::IFile* fp, std::time_t timestamp);
    void writeFitSessionSummary(SDK::Interface::IFile* fp, std::time_t timestamp);

    // ===== SESSION MANAGEMENT =====
    void startSession();
    void finalizeSession();

    // ... member variables
};
```

#### Key Data Structures

**FitRecord Structure:**
```cpp
struct FitRecord {
    std::time_t timestamp;
    uint8_t     heartRate;
    uint32_t    steps;
};
```

This structure represents individual data points containing timestamp, heart rate, and accumulated step count for FIT file recording during glance sessions.

### Sensor Integration

The service integrates with heart rate and step counter sensors to collect biometric data during glance sessions:

- **Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`): Provides real-time heart rate measurements in BPM
- **Step Counter Sensor** (`SDK::Sensor::Type::STEP_COUNTER`): Provides incremental step count measurements
- **Sampling Rate**: 5-second intervals for efficient power management
- **Data Processing**: Accumulates step deltas, captures heart rate values, and maintains session-based totals

### Data Processing Pipeline

#### 1. Sensor Data Reception

Heart rate and step data arrive through the kernel's message system. The `onSdlNewData()` method processes both sensor types during active glance sessions:

```cpp
void Service::onSdlNewData(uint16_t handle, const SDK::Sensor::Data* data, uint16_t count, uint16_t stride) {
    if (!mSessionOpen) return;  // Only process data during active sessions

    std::time_t now = std::time(nullptr);
    SDK::Sensor::DataBatch batch(data, count, stride);

    // Process step counter data
    if (mSensorSteps.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::StepCounter p(batch[i]);
            if (!p.isDataValid()) continue;
            uint32_t steps = p.getStepCount();
            if (steps > mLastSteps) {
                uint32_t delta = steps - mLastSteps;
                mTotalSteps += delta;
                mLastSteps = steps;
                mSampleCount++;
            }
        }
    }
    // Process heart rate data
    else if (mSensorHR.matchesDriver(handle)) {
        for (uint16_t i = 0; i < count; ++i) {
            SDK::SensorDataParser::HeartRate p(batch[i]);
            if (!p.isDataValid()) continue;
            mCurrentHR = static_cast<uint8_t>(p.getBpm());
        }
    }

    // Accumulate records for FIT file if we have data
    if (mSampleCount > 0 || mCurrentHR > 0) {
        mPendingRecords.push_back({now, mCurrentHR, mTotalSteps});
        LOG_DEBUG("Recorded data point: HR=%u, steps=%u\n", mCurrentHR, mTotalSteps);
    }
}
```

#### 2. Session Management

The app manages glance sessions with simple start/stop logic:

```cpp
void Service::startSession() {
    std::time_t now = std::time(nullptr);
    mSessionStart = now;
    mSessionOpen = true;
    mFitFileInitialized = false;  // Will initialize on first save
    mTotalSteps = 0;
    mLastSteps = 0;
    mSampleCount = 0;
    mCurrentHR = 0;
    mPendingRecords.clear();
    LOG_INFO("FIT session started at %ld\n", now);
}

void Service::finalizeSession() {
    if (mSessionOpen) {
        saveFit(true);
        mSessionOpen = false;
        LOG_INFO("FIT session finalized\n");
    }
}
```

#### 3. FIT File Creation and Management

The app creates properly formatted FIT files with headers, definitions, and data records:

**FIT File Structure:**
- File Header (protocol version, data size, CRC)
- File ID Message (manufacturer, product, timestamp)
- Developer Data ID Message (developer/app identification)
- Field Descriptions (custom developer fields)
- Data Records (timestamped heart rate and step counts)
- Session Messages (activity summaries)
- Activity Messages (overall activity data)
- File CRC

**FIT Helper Components:**
```cpp
SDK::Component::FitHelper mFitFileID;
SDK::Component::FitHelper mFitDeveloper;
SDK::Component::FitHelper mFitRecord;
SDK::Component::FitHelper mFitEvent;
SDK::Component::FitHelper mFitSession;
SDK::Component::FitHelper mFitActivity;
SDK::Component::FitHelper mFitStepsField;
```

Each FitHelper manages a specific FIT message type and handles serialization.

#### 4. Custom Developer Fields

The app demonstrates custom developer fields for extended data types:

```cpp
mFitStepsField.init({FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_NAME,
                    FIT_FIELD_DESCRIPTION_FIELD_NUM_UNITS,
                    FIT_FIELD_DESCRIPTION_FIELD_NUM_DEVELOPER_DATA_INDEX,
                    FIT_FIELD_DESCRIPTION_FIELD_NUM_FIELD_DEFINITION_NUMBER,
                    FIT_FIELD_DESCRIPTION_FIELD_NUM_FIT_BASE_TYPE_ID});

FIT_FIELD_DESCRIPTION_MESG stepsField{};
std::strncpy(stepsField.field_name, "steps", FIT_FIELD_DESCRIPTION_MESG_FIELD_NAME_COUNT);
std::strncpy(stepsField.units, "count", FIT_FIELD_DESCRIPTION_MESG_UNITS_COUNT);
stepsField.developer_data_index = 0;
stepsField.field_definition_number = 0;
stepsField.fit_base_type_id = FIT_BASE_TYPE_UINT32;
mFitStepsField.writeMessage(&stepsField, fp);
```

#### 4. Session and Activity Management

The app manages activity sessions with proper start/stop events and summaries:

**Session Creation:**
```cpp
void Service::startSession() {
    std::time_t now = std::time(nullptr);
    mSessionStart = now;
    mSessionOpen = true;
    mFitFileInitialized = false;  // Will initialize on first save
    mTotalSteps = 0;
    mLastSteps = 0;
    mSampleCount = 0;
    mCurrentHR = 0;
    mPendingRecords.clear();
    LOG_INFO("FIT session started at %ld\n", now);
}
```

**Session Summary:**
```cpp
void Service::writeFitSessionSummary(SDK::Interface::IFile* fp, std::time_t timestamp) {
    // Stop session event
    FIT_EVENT_MESG stop_event{};
    stop_event.timestamp = unixToFitTimestamp(timestamp);
    stop_event.event = FIT_EVENT_TIMER;
    stop_event.event_type = FIT_EVENT_TYPE_STOP;
    mFitEvent.writeMessage(&stop_event, fp);

    // Session message with timing and sport data
    FIT_SESSION_MESG session_mesg{};
    session_mesg.message_index = 0;
    session_mesg.sport = FIT_SPORT_GENERIC;
    session_mesg.sub_sport = FIT_SUB_SPORT_GENERIC;
    session_mesg.timestamp = unixToFitTimestamp(timestamp);
    session_mesg.start_time = unixToFitTimestamp(mSessionStart);
    session_mesg.total_elapsed_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    session_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    mFitSession.writeMessage(&session_mesg, fp);

    // Activity summary
    FIT_ACTIVITY_MESG activity_mesg{};
    activity_mesg.timestamp = unixToFitTimestamp(timestamp);
    activity_mesg.local_timestamp = unixToFitTimestamp(timestamp);  // Simplified
    activity_mesg.total_timer_time = static_cast<FIT_UINT32>((timestamp - mSessionStart) * 1000);
    activity_mesg.num_sessions = 1;
    mFitActivity.writeMessage(&activity_mesg, fp);
}
```

### Data Persistence Strategy

#### FIT File Writing Process

1. **File Opening**: Open existing file or create new one
2. **Header Management**: Update file header with correct data size
3. **Definition Writing**: Write message definitions on first use
4. **Data Append**: Add new records to existing file
5. **Session Management**: Handle session start/stop events
6. **CRC Calculation**: Compute and append file CRC

#### Data Persistence Strategy

The app saves data on session completion:

- **Session-based**: Automatic save when glance session ends
- **Event-based**: App termination triggers session finalization
- **Simple approach**: One FIT file per session with complete data

#### FIT File Writing Process

1. **File Creation**: Create new FIT file on session start
2. **Header Management**: Write placeholder header (updated with final size)
3. **Definition Writing**: Write message definitions once
4. **Data Append**: Add records during session
5. **Session Finalization**: Write session summary on completion
6. **CRC Calculation**: Compute and append file CRC

## Glance UI Implementation

The glance UI provides a simple display for the current heart rate and step count during active sessions:

### UI Components

**Glance Controls:**
- Icon display (60x60 pixel icon)
- Title text: "Steps"
- Value text: Current step count
- Heart rate text: Current heart rate in BPM

**Layout:**
```
+-------------------+
|        Icon       |
|                   |
|      Steps        |
|     1,234         |
|   HR: 72          |
+-------------------+
```

### Message Handling

The service updates the glance display on each tick event with current sensor data:

```cpp
void Service::onGlanceTick() {
    mGlanceValue.print("%u", mTotalSteps);
    mGlanceHR.print("HR: %u", mCurrentHR);
    // Send update to glance interface
}
```

## Sensor Integration

The FitFiles app integrates with the UNA SDK's sensor layer for heart rate and step counting during glance sessions:

### Heart Rate and Step Counter Sensors

**Heart Rate Sensor Configuration:**
- Type: `SDK::Sensor::Type::HEART_RATE`
- Sample Period: 5 seconds (300,000 ms)
- Latency: 1,000 ms

**Step Counter Sensor Configuration:**
- Type: `SDK::Sensor::Type::STEP_COUNTER`
- Sample Period: 5 seconds (300,000 ms)
- Latency: 1,000 ms

**Data Processing:**
- Real-time heart rate measurements in BPM
- Incremental step counts from hardware steps
- Delta calculation to track new steps
- Session-based accumulation of biometric data
- Timestamp association for FIT recording

### Sensor Data Flow

```
Hardware Sensors -> Sensor Drivers -> SDK Parsers -> Service Accumulator -> FIT File
    (HR + Steps)      (HR + Steps)     (HR + Steps)       (Session Data)
```

## FIT File Format Implementation

### FIT Protocol Overview

FIT (Flexible and Interoperable Data Transfer) is Garmin's binary file format for fitness data. Key characteristics:

- Binary format for efficient storage
- Self-describing with message definitions
- Extensible through developer fields
- CRC validation for data integrity
- Timestamp-based (seconds since FIT epoch: 1989-12-31 00:00:00 UTC)

### Message Types Used

1. **File ID** (`FIT_MESG_FILE_ID`): File metadata
2. **Developer Data ID** (`FIT_MESG_DEVELOPER_DATA_ID`): Developer identification
3. **Field Description** (`FIT_MESG_FIELD_DESCRIPTION`): Custom field definitions
4. **Record** (`FIT_MESG_RECORD`): Data points with timestamps
5. **Event** (`FIT_MESG_EVENT`): Session start/stop markers
6. **Session** (`FIT_MESG_SESSION`): Activity segment summaries
7. **Activity** (`FIT_MESG_ACTIVITY`): Overall activity summary

### Custom Developer Fields

The app demonstrates developer fields for heart rate and step data:

**Heart Rate Field (Standard FIT):**
- Built-in FIT field: `FIT_RECORD_FIELD_NUM_HEART_RATE`
- Units: BPM
- Base Type: `FIT_BASE_TYPE_UINT8`

**Steps Field (Developer Field):**
- Field Name: "steps"
- Units: "count"
- Base Type: `FIT_BASE_TYPE_UINT32`
- Developer Index: 0
- Field Number: 0

**Data Recording:**
```cpp
FIT_RECORD_MESG record_mesg{};
record_mesg.timestamp = unixToFitTimestamp(rec.timestamp);
record_mesg.heart_rate = rec.heartRate;
mFitRecord.writeMessage(&record_mesg, fp);
uint32_t steps = rec.steps;
mFitRecord.writeFieldMessage(0, &steps, fp);
```

## Build and Setup

The FitFiles app uses CMake for cross-platform builds:

### Build Configuration

**Primary Build File**: `CMakeLists.txt` in `FitFiles/Software/App/FitFiles-CMake/`

```cmake
set(APP_NAME "FitFiles")
set(APP_TYPE "Glance")
set(DEV_ID "UNA")
set(APP_ID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
```

### Build Targets

**Service Build**:
```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_SERVICE}
    ${UNA_SDK_SOURCES_SENSOR}
    ${UNA_SDK_SOURCES_FIT}
)
una_app_build_service(${APP_NAME}Service.elf)
```

### Dependencies

**SDK Components**:
- UNA SDK common, service, and sensor sources
- FIT helper utilities
- File system interfaces
- Kernel messaging system

## Key Concepts Demonstrated

### FIT File Creation

1. **File Structure**: Proper FIT file format with headers, definitions, and data
2. **Message Definitions**: Dynamic definition writing for message types
3. **Developer Fields**: Custom field creation for extended data types
4. **CRC Validation**: File integrity through cyclic redundancy checks

### Sensor Data Logging

1. **Event-Driven Processing**: Sensor data collection triggered by glance events
2. **Multi-Sensor Integration**: Simultaneous heart rate and step counter handling
3. **Session-Based Accumulation**: Data collection limited to active glance sessions
4. **Timestamp Management**: Proper time handling for session-based activity data
5. **Power Efficiency**: Optimized sampling rates and processing

### Session Management

1. **Glance Sessions**: Start/stop event handling based on user interaction
2. **Session Boundaries**: Automatic session management with FIT file creation
3. **Data Persistence**: Reliable storage with session completion
4. **Simplified Approach**: Clean session lifecycle without complex state recovery

### File System Integration

1. **File Operations**: Create, read, write, and truncate operations
2. **Path Management**: Organized file naming and directory structure
3. **Data Integrity**: Header updates and CRC calculations
4. **Resource Management**: Proper file handle lifecycle

## Next Steps

This tutorial provides a foundation for more advanced glance-based fitness applications. The simplified approach focuses on core FIT file creation concepts:

1. **Additional Sensors**: Add GPS, altitude, or other biometric sensors
2. **Advanced FIT Features**: Implement laps, events, and complex activities
3. **Data Synchronization**: Add cloud upload capabilities
4. **Enhanced UI**: Extend to full TouchGFX applications with charts and trends
5. **Performance Optimization**: Implement data compression and efficient storage
6. **Health Metrics**: Calculate calories, distance, and activity intensity
7. **Session Analytics**: Add post-session summary and statistics

The tutorial demonstrates essential FIT file creation using UNA SDK's FitHelper components with a clean, tutorial-friendly implementation.

The FitFiles tutorial demonstrates essential concepts for building robust, data-persistent wearable applications using the UNA SDK's FIT file capabilities and sensor integration features.