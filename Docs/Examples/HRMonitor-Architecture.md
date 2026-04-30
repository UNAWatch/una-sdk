# HRMonitor - Heart Rate Monitor

## Overview

The HRMonitor app is a dedicated heart rate monitoring application designed for wearable devices, specifically targeting continuous cardiac monitoring. It provides real-time display of heart rate measurements with trust level assessment, persistent data recording in FIT file format, and a clean TouchGFX-based user interface optimized for wearable screens.

The application follows a modular architecture with separate service and GUI components, communicating through a custom message system using the UNA SDK's kernel infrastructure. It supports continuous heart rate tracking with data export capabilities and automatic activity session management.

Key features include:
- Real-time heart rate monitoring with trust level display
- Continuous data recording in FIT file format
- Activity session tracking with summary statistics
- Simple, focused user interface for wearable devices
- Automatic sensor connection and data validation

## Architecture

The HRMonitor app follows a client-server architecture pattern where the service component handles all backend logic, sensor integration, and data processing, while the GUI component manages user interaction and display. Communication between these components occurs through a message-based system using the UNA SDK's kernel infrastructure.

### High-Level Components

1. **Service Layer**: Core business logic, heart rate sensor integration, data processing
2. **GUI Layer**: TouchGFX-based user interface, heart rate display
3. **SDK Integration**: Kernel, sensor layer, file system, messaging
4. **Data Persistence**: FIT file format for heart rate data

### Component Interaction

```
[Heart Rate Sensor] <-> [Sensor Layer] <-> [Service]
        ^                    ^                ^
        |                    |                |
[File System] <-- [Activity Writer] --> [FIT Recording]
        ^                    ^                ^
        |                    |                |
[Kernel Messages] <-- [Message System] --> [GUI]
```

The service runs as a separate process/thread, continuously processing heart rate sensor data and maintaining activity state. The GUI runs on the TouchGFX framework, handling user input and displaying heart rate information received from the service.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for heart rate monitoring and data recording.

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

**Heart Rate Data Structure:**
```cpp
struct {
    float mHR;       // Current heart rate value
    float mHRTL;     // Trust level (1-3 scale)
} mHeartRateData;
```

**Activity Writer Integration:**
```cpp
ActivityWriter mActivityWriter;  // Handles FIT file recording
```

### Sensor Integration

The service manages heart rate sensor connection:

- **Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`):
  - Provides BPM measurements with trust levels
  - Trust levels: 1-3 (higher is better)
  - 1-second sampling period with 2-second latency

Each sensor is represented by an `SDK::Sensor::Connection` object with specific sampling periods and latencies.

### Data Processing Pipeline

The data processing pipeline handles heart rate data reception, validation, and recording:

#### 1. Sensor Data Reception

Heart rate data arrives through the kernel's message system. The `onSdlNewData()` method processes the sensor data:

```cpp
void Service::onSdlNewData(uint16_t handle, SDK::Sensor::DataBatch& data) {
    if (mSensorHR.matchesDriver(handle)) {
        if (mGUIStarted) {
            SDK::SensorDataParser::HeartRate parser(data[0]);
            if (parser.isDataValid()) {
                mHR = parser.getBpm();
                mHRTL = parser.getTrustLevel();

                mSender.updateHeartRate(mHR, mHRTL);
            }
        }
    }
}
```

Each sensor connection has a `matchesDriver()` method to identify the source of the data batch.

#### 2. Data Validation and Filtering

Raw heart rate data undergoes validation to ensure quality:

**Trust Level Filtering:**
```cpp
if (parser.isDataValid()) {
    mHR = parser.getBpm();
    mHRTL = parser.getTrustLevel();  // 1-3 scale
    // Only send valid data to GUI
    mSender.updateHeartRate(mHR, mHRTL);
}
```

#### 3. FIT File Recording

Heart rate data is recorded in FIT (Flexible and Interoperable Data Transfer) format every second during active monitoring:

```cpp
ActivityWriter::RecordData fitRecord {};
fitRecord.timestamp = utc;
fitRecord.heartRate = static_cast<uint8_t>(mHR);
fitRecord.trustLevel = static_cast<uint8_t>(mHRTL);
mActivityWriter.addRecord(fitRecord);
```

FIT records include heart rate and trust level data for interoperability with fitness applications.

### Activity State Management

The service maintains activity state and handles lifecycle events:

- **Active Monitoring**: Continuously collects heart rate data when GUI is active
- **Data Recording**: Writes FIT records every second
- **Session Management**: Tracks activity start/end times
- **Statistics Calculation**: Computes average and maximum heart rates

### Settings and Persistence

Activity data is persisted in FIT format with session summaries:

- Heart rate records with timestamps
- Trust level information
- Session start/end times
- Average and maximum heart rate calculations

## GUI Implementation

The GUI is built using TouchGFX framework, providing a simple, focused interface for heart rate monitoring.

### Model-View-Presenter Pattern

The GUI follows MVP architecture:

- **Model**: `Model.hpp/cpp` - Data management and service communication
- **View**: MainView - UI rendering
- **Presenter**: MainPresenter - Logic binding model and view

### Key GUI Components

#### Model Class

The Model class serves as the central data hub:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler {
public:
    void bind(ModelListener *listener);
    void tick();
    void handleKeyEvent(uint8_t c);
    // Heart rate management methods
    void exitApp();
};
```

Key responsibilities:
- Lifecycle management (onStart, onResume, onSuspend, onStop)
- Message handling from service
- Application exit coordination

#### View Classes

**MainView**: Primary heart rate display screen
- Shows current heart rate value
- Displays trust level indicator
- Minimal button layout with exit functionality

### Message Handling System

The Model implements `ICustomMessageHandler` to receive heart rate updates from the service:

```cpp
bool Model::customMessageHandler(SDK::MessageBase *msg) {
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

### Screen Navigation and State Management

The app uses a single-screen design optimized for continuous monitoring:

**Screen Flow**:
```
Main Screen (Heart Rate Display)
    |
    v
Exit Application
```

**Transition Types**:
- No transitions needed for single-screen design
- Direct exit on button press

### Custom Containers Implementation

The app uses minimal custom containers for the heart rate display:

**Main Screen Layout**:
- Large heart rate number display
- Trust level indicator
- Exit button (R2)

### Input Handling Architecture

User input is minimal, focused on application exit:

**Button Mapping**:
- **R2**: Exit application

### Data Formatting and Units

The GUI handles heart rate display formatting:

**Heart Rate Display**:
```cpp
Unicode::snprintfFloat(textHRBuffer, TEXTHR_SIZE, "%.0f", hr);
textHR.invalidate();
```

**Trust Level Display**:
```cpp
Unicode::snprintfFloat(textTRLBuffer, TEXTTRL_SIZE, "%.0f", tl);
textTRL.invalidate();
```

## Sensor Integration

The HRMonitor app integrates heart rate sensor through the UNA SDK's sensor layer.

### Heart Rate Sensor

**Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`):
- Provides BPM measurements with trust levels
- Trust levels indicate signal quality (1-3 scale)
- Continuous monitoring during app active state

### Sensor Data Processing Architecture

The sensor data processing system uses specialized parsers for heart rate data extraction:

#### Parser Classes

**Heart Rate Parser**:
```cpp
SDK::SensorDataParser::HeartRate parser(data[0]);
if (parser.isDataValid()) {
    mHR = parser.getBpm();
    mHRTL = parser.getTrustLevel();
}
```

#### Data Validation

**Trust Level Assessment**:
- Trust levels 1-3 indicate signal quality
- Higher trust levels represent better signal quality
- Data is always sent to GUI regardless of trust level

#### Sensor Sampling Strategy

**Heart Rate Sampling**:
- 1 Hz sampling for real-time display
- 2-second latency for power optimization
- Continuous sampling during active monitoring

## TouchGFX Port

The HRMonitor app uses TouchGFX for its graphical user interface, providing a clean, minimal design for wearable devices.

### TouchGFX Project Structure

```
TouchGFX-GUI/
├── application.config    # Application configuration
├── target.config         # Target hardware settings
├── touchgfx.cmake        # CMake integration
├── gui/                  # Generated and custom GUI code
├── assets/               # Images, fonts, texts (minimal)
└── generated/            # Auto-generated code
```

### GUI Architecture

**Screens and Transitions**:
- Single main screen for heart rate display
- No screen transitions required
- Direct exit functionality

**Custom Containers**:
- Minimal container usage
- Direct text and button widgets

### TouchGFX Integration with UNA SDK

The integration uses standard TouchGFXCommandProcessor and KernelProviderGUI patterns.

#### Custom Message System

The app defines custom message types for service-GUI communication:

```cpp
namespace CustomMessage {
    constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;

    struct HRValues : public SDK::MessageBase {
        float heartRate;
        float trustLevel;
    };
}
```

#### Message Sender Classes

**GUISender** provides type-safe message sending:

```cpp
class GUISender {
public:
    bool updateHeartRate(float value, float trustLevel) {
        if (auto req = SDK::make_msg<CustomMessage::HRValues>(mKernel)) {
            req->heartRate = value;
            req->trustLevel = trustLevel;
            return req.send();
        }
        return false;
    }
};
```

### Asset Management

**Images**: Minimal icon assets for the application
**Fonts**: Standard TouchGFX fonts for text display
**Texts**: Localized strings (minimal text content)

## Build and Setup

The HRMonitor app uses CMake for cross-platform builds targeting embedded hardware and simulation.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `HRMonitor-CMake/`

```cmake
# App configuration
set(APP_NAME "HRMonitor")
set(APP_TYPE "Activity")
set(DEV_ID "UNA")
set(APP_ID "F1E2D3C448669780")

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
- Custom app libraries (ActivityWriter)

### Development Setup

See [SDK Setup and Build Overview](../sdk-setup.md) for comprehensive development environment setup, build instructions, and toolchain requirements.

### Simulator Build

TouchGFX provides simulator builds for PC development:
- Visual Studio project for Windows
- Includes mock hardware interfaces

## Conclusion

The HRMonitor app demonstrates a focused implementation of a heart rate monitoring application on wearable devices. Its modular architecture separates concerns effectively between service logic and user interface, enabling reliable sensor integration and data recording.

Key architectural strengths include:
- **Separation of Concerns**: Clear division between service and GUI
- **Message-Based Communication**: Loose coupling between components
- **Sensor Integration**: Robust heart rate sensor handling
- **Data Persistence**: FIT file format ensures interoperability
- **Minimal UI**: Focused design optimized for wearable use

The implementation showcases real-time systems programming, embedded GUI development, and sensor data processing. The app successfully integrates heart rate monitoring with persistent storage while maintaining a clean, focused user experience.

Future enhancements could include additional physiological metrics, enhanced trust level visualization, and expanded activity tracking features while maintaining the core architectural principles established in this implementation.