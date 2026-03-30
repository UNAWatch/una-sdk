# GlanceHR - Heart Rate Glance

## Overview

The GlanceHR app is a simple wearable application designed to display real-time heart rate data in a compact glance view. It provides continuous heart rate monitoring with a clean, minimal interface that shows the current heart rate value alongside an icon. The app integrates heart rate sensor data and presents it through the UNA SDK's glance framework, making it easily accessible from the watch face.

The application follows a service-only architecture with no separate GUI component, communicating directly with the glance system through the UNA SDK's kernel infrastructure. It supports real-time heart rate updates and handles sensor connection management automatically.

Key features include:
- Real-time heart rate monitoring with live display
- Automatic sensor connection and data processing
- Clean glance interface with heart rate icon and value
- Graceful handling of sensor availability and data validity
- Low-power operation suitable for continuous monitoring

## Architecture

The GlanceHR app follows a service-only architecture pattern where the service component handles all functionality including sensor integration, data processing, and glance UI management. Communication occurs through the UNA SDK's kernel infrastructure and glance messaging system.

### High-Level Components

1. **Service Layer**: Core business logic, sensor integration, data processing, and glance UI management
2. **SDK Integration**: Kernel, sensor layer, glance framework, messaging
3. **Glance Framework**: UNA SDK's glance system for compact UI display

### Component Interaction

```
[Hardware Sensors] <-> [Sensor Layer] <-> [Service] <-> [Glance Framework]
        ^                    ^                ^
        |                    |                |
[Kernel Messages] <-- [Message System] --> [Glance Updates]
```

The service runs as a separate process, continuously processing heart rate sensor data and updating the glance display. The glance framework handles the actual rendering and user interaction.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for heart rate monitoring and glance display.

### Core Classes and Structures

#### Service Class

The main service class inherits from no base class but implements lifecycle management and messaging through the kernel.

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

**Heart Rate Data:**
```cpp
SDK::Sensor::Connection mSensorHR;  // Heart rate sensor connection
float                   mHrValue;    // Current heart rate value
bool                    mIsValid;    // Data validity flag
```

**Glance UI Components:**
```cpp
SDK::Glance::Form        mGlanceUI;      // Main glance form
SDK::Glance::ControlText mGlanceTitle;   // "Live HR" title
SDK::Glance::ControlText mGlanceValue;   // Heart rate value display
```

### Sensor Integration

The service manages a single heart rate sensor connection:

- **Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`): Provides BPM measurements with trust levels

The sensor is represented by an `SDK::Sensor::Connection` object with appropriate sampling periods and latencies.

### Data Processing Pipeline

The data processing pipeline is streamlined for heart rate display, focusing on real-time updates and validity checking.

#### 1. Sensor Data Reception

Heart rate data arrives through the kernel's message system. The `handleSensorsData()` method processes the sensor data:

```cpp
void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data) {
    if (mSensorHR.matchesDriver(handle)) {
        SDK::SensorDataParser::HeartRate p(data[0]);
        float newValue = p.getBpm();
        LOG_DEBUG("hr = %.2f\n", newValue);
        if (static_cast<uint32_t>(newValue) != static_cast<uint32_t>(mHrValue)) {
            mHrValue = newValue;
            glanceUpdate();
        }
    }
}
```

Each sensor connection has a `matchesDriver()` method to identify the source of the data batch.

#### 2. Data Validation and Filtering

Raw heart rate data undergoes validation to ensure accuracy:

**Heart Rate Validation:**
```cpp
if (mHrValue > 1.0 && !mIsValid) {
    mIsValid = true;
    // Switch to valid display mode
} else if (mHrValue <= 1.0 && mIsValid) {
    mIsValid = false;
    // Switch to calculating display mode
}
```

The app distinguishes between valid heart rate readings (> 1.0 BPM) and invalid/calculating states.

#### 3. Glance Update Logic

The `glanceUpdate()` method manages the display state based on data validity:

```cpp
void Service::glanceUpdate() {
    if (mHrValue > 1.0 && !mIsValid) {
        mIsValid = true;
        mGlanceValue.pos({ 80, 28 }, { 160, 34 })
                .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
                .setText("")
                .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);
    } else if (mHrValue <= 1.0 && mIsValid) {
        mIsValid = false;
        mGlanceValue.pos({ 81, 34 }, { 130, 23 })
                .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_18)
                .setText(skTextCalculating)
                .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_LEFT);
    }

    if (mIsValid) {
        mGlanceValue.print("%.0f          ", mHrValue);
    }
}
```

### Activity State Management

The service maintains glance lifecycle states:
- **Glance Start**: Initialize UI and connect sensors
- **Glance Stop**: Disconnect sensors and cleanup
- **Glance Tick**: Periodic UI updates and refresh

State transitions are handled by message processing in the main run loop.

### Glance UI Management

The app creates a simple glance interface with three components:

**Glance Form Layout:**
```cpp
void Service::createGuiControls() {
    // Heart rate icon (60x60 pixels)
    mGlanceUI.createImage().init({20, 0}, {60, 60}, ICON_60X60_ABGR2222);

    // Title text "Live HR"
    mGlanceTitle = mGlanceUI.createText();
    mGlanceTitle.pos({ 70, 0 }, { 100, 25 })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_TEAL)
        .setText("Live HR")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    // Heart rate value display
    mGlanceValue = mGlanceUI.createText();
    mGlanceValue.pos({ 81, 34 }, { 172, 23 })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_18)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .setText(skTextCalculating)
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_LEFT);
}
```

### Settings and Configuration

The app configures the glance display through the UNA SDK's glance configuration system:

```cpp
bool Service::configGui() {
    bool status = false;
    if (auto gc = SDK::make_msg<SDK::Message::RequestGlanceConfig>(mKernel)) {
        if (gc.send(100) && gc.ok()) {
            if (gc->maxControls >= 3) {
                mGlanceUI.setWidth(gc->width);
                mGlanceUI.setHeight(gc->height);
                status = true;
            }
        }
    }
    return status;
}
```

The configuration ensures the glance has sufficient space for the three UI controls (icon, title, value).

## Sensor Integration

The GlanceHR app integrates a single physiological sensor through the UNA SDK's sensor layer.

### Heart Rate Sensor

**Heart Rate Sensor** (`SDK::Sensor::Type::HEART_RATE`):
- Provides BPM measurements with trust levels
- Used for real-time heart rate display
- Trust levels: 1-3 (higher is better)
- Only valid data is displayed

### Sensor Data Processing Architecture

The sensor data processing system uses specialized parsers for heart rate data extraction and validation.

#### Parser Classes

**Heart Rate Parser:**
```cpp
SDK::SensorDataParser::HeartRate parser(data[0]);
if (parser.isDataValid()) {
    float bpm = parser.getBpm();
    uint8_t trustLevel = parser.getTrustLevel();
    // Process heart rate data
}
```

#### Data Validation

**Heart Rate Validity Checking:**
```cpp
bool isValidHeartRate = (mHrValue > 1.0 && mHrValue <= 300.0);  // Reasonable BPM range
if (isValidHeartRate) {
    // Display heart rate
} else {
    // Show calculating state
}
```

#### Sensor Sampling Strategy

**Adaptive Sampling Rates:**
- **Heart Rate**: 1 Hz sampling for real-time display
- **Latency**: 1000ms latency balancing responsiveness and power consumption

#### Error Handling and Data Validation

**Sensor Availability Management:**
```cpp
void Service::connect() {
    if (!mSensorHR.isConnected()) {
        LOG_DEBUG("Connect to sensors...\n");
        mSensorHR.connect();
    }
}

void Service::disconnect() {
    if (mSensorHR.isConnected()) {
        LOG_DEBUG("Disconnect from sensors...\n");
        mSensorHR.disconnect();
    }
}
```

Sensor connections are managed based on glance lifecycle events.

### Glance Framework Integration

The app uses the UNA SDK's glance framework for compact UI display:

#### Glance Message System

**Glance Update Messages:**
```cpp
void Service::onGlanceTick() {
    if (mGlanceUI.isInvalid()) {
        if (auto upd = SDK::make_msg<SDK::Message::RequestGlanceUpdate>(mKernel)) {
            upd->name           = APP_NAME;
            upd->controls       = mGlanceUI.data();
            upd->controlsNumber = static_cast<uint32_t>(mGlanceUI.size());
            upd.send(100);
        }
        mGlanceUI.setValid();
    }
}
```

The glance system handles periodic updates and invalidation management.

#### Control Types

**Text Controls:**
- Title control with fixed "Live HR" text
- Value control with dynamic heart rate display
- Font and color customization through SDK enums

**Image Controls:**
- Heart rate icon display using pre-compiled bitmap data
- Positioned alongside text controls

### Message Handling System

The service implements message-based communication for lifecycle and data events:

```cpp
switch (msg->getType()) {
    case SDK::MessageType::EVENT_GLANCE_START:
        // Initialize glance and connect sensors
        break;
    case SDK::MessageType::EVENT_GLANCE_STOP:
        // Disconnect sensors and cleanup
        break;
    case SDK::MessageType::EVENT_GLANCE_TICK:
        // Periodic UI refresh
        break;
    case SDK::MessageType::EVENT_SENSOR_LAYER_DATA:
        // Process sensor data
        break;
}
```

## Build and Setup

The GlanceHR app uses CMake for cross-platform builds targeting embedded hardware.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `GlanceHR-CMake/`

```cmake
# App configuration
set(APP_NAME "GlanceHR")
set(APP_USER_NAME "Live HR")
set(APP_TYPE "Glance")
set(DEV_ID "UNA")
set(APP_ID "A1358F7C2E9D4BA6")

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

**Complete App**:
```cmake
una_app_build_app()
```

### Dependencies

**SDK Components**:
- UNA SDK common and service sources
- Sensor layer interfaces
- Kernel and messaging systems
- Glance framework components

### Build Process

1. **CMake Configuration**: Sets up toolchain and paths
2. **Source Collection**: Gathers service source files
3. **Compilation**: Builds service executable
4. **Packaging**: Combines into deployable glance app package

### Development Setup

See [SDK Setup and Build Overview](sdk-setup.md) for comprehensive development environment setup, build instructions, and toolchain requirements.

## Conclusion

The GlanceHR app demonstrates a focused implementation of a heart rate monitoring glance for wearable devices. Its service-only architecture provides efficient real-time heart rate display with minimal resource usage.

Key architectural strengths include:
- **Simplicity**: Focused on single responsibility of heart rate display
- **Efficiency**: Low-power operation suitable for continuous monitoring
- **Integration**: Seamless integration with UNA SDK glance framework
- **Reliability**: Robust sensor connection and data validation
- **User Experience**: Clean, intuitive interface for quick heart rate checks

The implementation showcases effective use of the UNA SDK's glance system and sensor layer for building lightweight, purpose-driven wearable applications. The app successfully provides real-time heart rate monitoring while maintaining the performance characteristics expected of glance interfaces.