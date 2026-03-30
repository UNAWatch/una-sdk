# GlanceSteps - Step Counter Glance

## Overview

The GlanceSteps app is a simple wearable application designed to display real-time step count data in a compact glance view. It provides continuous pedometer monitoring with a clean, minimal interface that shows the current step count alongside an icon. The app integrates step counter sensor data and presents it through the UNA SDK's glance framework, making it easily accessible from the watch face.

The application follows a service-only architecture with no separate GUI component, communicating directly with the glance system through the UNA SDK's kernel infrastructure. It supports real-time step count updates and handles sensor connection management automatically.

Key features include:
- Real-time step count monitoring with live display
- Automatic sensor connection and data processing
- Clean glance interface with step counter icon and value
- Graceful handling of sensor availability and data validity
- Low-power operation suitable for continuous monitoring

## Architecture

The GlanceSteps app follows a service-only architecture pattern where the service component handles all functionality including sensor integration, data processing, and glance UI management. Communication occurs through the UNA SDK's kernel infrastructure and glance messaging system.

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

The service runs as a separate process, continuously processing step counter sensor data and updating the glance display. The glance framework handles the actual rendering and user interaction.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`, providing the core functionality for step count monitoring and glance display.

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

**Step Counter Data:**
```cpp
SDK::Sensor::Connection mSensorPedo;  // Step counter sensor connection
uint32_t                mStepsValue;   // Current step count value
bool                    mDataReceived; // Data validity flag
```

**Glance UI Components:**
```cpp
SDK::Glance::Form        mGlanceUI;      // Main glance form
SDK::Glance::ControlText mGlanceTitle;   // "Steps" title
SDK::Glance::ControlText mGlanceValue;   // Step count value display
```

### Sensor Integration

The service manages a single step counter sensor connection:

- **Step Counter Sensor** (`SDK::Sensor::Type::STEP_COUNTER`): Provides cumulative step count measurements

The sensor is represented by an `SDK::Sensor::Connection` object with appropriate sampling periods and latencies.

### Data Processing Pipeline

The data processing pipeline is streamlined for step count display, focusing on real-time updates and validity checking.

#### 1. Sensor Data Reception

Step counter data arrives through the kernel's message system. The `handleSensorsData()` method processes the sensor data:

```cpp
void Service::handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data) {
    if (mSensorPedo.matchesDriver(handle)) {
        SDK::SensorDataParser::StepCounter p(data[0]);

        if (p.isDataValid()) {
            LOG_DEBUG("steps = %d\n", p.getStepCount());
            uint32_t newValue = p.getStepCount();
            if (mStepsValue != newValue) {
                mStepsValue = newValue;
                mGlanceValue.print("%u", mStepsValue);
            }

            if (!mDataReceived && mStepsValue == 0) {
                mGlanceValue.print("%u", mStepsValue);
            }

            mDataReceived = true;
        }
    }
}
```

Each sensor connection has a `matchesDriver()` method to identify the source of the data batch.

#### 2. Data Validation and Filtering

Raw step counter data undergoes validation to ensure accuracy:

**Step Count Validation:**
```cpp
if (p.isDataValid()) {
    uint32_t newValue = p.getStepCount();
    if (mStepsValue != newValue) {
        mStepsValue = newValue;
        // Update display with new value
    }
}
```

The app handles initial state (zero steps) and incremental updates appropriately.

#### 3. Glance Update Logic

The glance display is updated directly when step count changes, using the UNA SDK's glance framework for efficient rendering.

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
    // Step counter icon (60x60 pixels)
    mGlanceUI.createImage().init({20, 0}, {60, 60}, ICON_60X60_ABGR2222);

    // Title text "Steps"
    mGlanceTitle = mGlanceUI.createText();
    mGlanceTitle.pos({ 70, 0 }, { 100, 25 })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_20)
        .color(GlanceColor_t::GLANCE_COLOR_TEAL)
        .setText("Steps")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);

    // Step count value display
    mGlanceValue = mGlanceUI.createText();
    mGlanceValue.pos({ 80, 28 }, { 80, 34 })
        .font(GlanceFont_t::GLANCE_FONT_POPPINS_SEMIBOLD_30)
        .color(GlanceColor_t::GLANCE_COLOR_WHITE)
        .setText("")
        .alignment(GlanceAlignH_t::GLANCE_ALIGN_H_CENTER);
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

The GlanceSteps app integrates a single motion sensor through the UNA SDK's sensor layer.

### Step Counter Sensor

**Step Counter Sensor** (`SDK::Sensor::Type::STEP_COUNTER`):
- Provides cumulative step count measurements
- Used for real-time step count display
- Maintains count across app sessions
- Automatic reset handling for new days/periods

### Sensor Data Processing Architecture

The sensor data processing system uses specialized parsers for step count data extraction and validation.

#### Parser Classes

**Step Counter Parser:**
```cpp
SDK::SensorDataParser::StepCounter parser(data[0]);
if (parser.isDataValid()) {
    uint32_t stepCount = parser.getStepCount();
    // Process step count data
}
```

#### Data Validation

**Step Count Validity Checking:**
```cpp
bool isValidStepCount = (stepCount >= 0 && stepCount <= 100000);  // Reasonable daily step range
if (isValidStepCount) {
    // Display step count
} else {
    // Handle invalid data
}
```

#### Sensor Sampling Strategy

**Adaptive Sampling Rates:**
- **Step Counter**: Event-driven updates when steps are detected
- **Latency**: 1000ms latency balancing responsiveness and power consumption

#### Error Handling and Data Validation

**Sensor Availability Management:**
```cpp
void Service::connect() {
    if (!mSensorPedo.isConnected()) {
        LOG_DEBUG("Connect to sensors...\n");
        mSensorPedo.connect();
    }
}

void Service::disconnect() {
    if (mSensorPedo.isConnected()) {
        LOG_DEBUG("Disconnect from sensors...\n");
        mSensorPedo.disconnect();
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
- Title control with fixed "Steps" text
- Value control with dynamic step count display
- Font and color customization through SDK enums

**Image Controls:**
- Step counter icon display using pre-compiled bitmap data
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

The GlanceSteps app uses CMake for cross-platform builds targeting embedded hardware.

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `GlanceSteps-CMake/`

```cmake
# App configuration
set(APP_NAME "GlanceSteps")
set(APP_USER_NAME "Steps")
set(APP_TYPE "Glance")
set(DEV_ID "UNA")
set(APP_ID "8899AABBCCDDEEFF")

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

The GlanceSteps app demonstrates a focused implementation of a step counter glance for wearable devices. Its service-only architecture provides efficient real-time step count display with minimal resource usage.

Key architectural strengths include:
- **Simplicity**: Focused on single responsibility of step count display
- **Efficiency**: Low-power operation suitable for continuous monitoring
- **Integration**: Seamless integration with UNA SDK glance framework
- **Reliability**: Robust sensor connection and data validation
- **User Experience**: Clean, intuitive interface for quick step count checks

The implementation showcases effective use of the UNA SDK's glance system and sensor layer for building lightweight, purpose-driven wearable applications. The app successfully provides real-time step count monitoring while maintaining the performance characteristics expected of glance interfaces.