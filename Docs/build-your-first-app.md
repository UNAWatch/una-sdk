# First App

This tutorial walks you through building process of an app. For SDK setup and build system reference, see [SDK Setup and Build Overview](sdk-setup.md).

## Overview: Heart Rate Monitor App

Our app will:

- Monitor heart rate using the watch's PPG sensor
- Display real-time BPM on the screen
- Store readings for trend analysis
- Show notifications for abnormal readings
- Sync data to companion apps via BLE

## Development Phases

### Phase 1: Project Setup and Architecture

#### Step 1.1: Choose App Type

Una-Watch supports four app types:

| Type | Use Case | Lifecycle | Memory |
|------|----------|-----------|---------|
| **Activity** | Full-featured apps (fitness, games) | Always running | 256KB |
| **Utility** | Tools (calculator, settings) | On-demand | 128KB |
| **Glance** | Notifications (weather, alerts) | Background | 64KB |
| **Clockface** | Watch faces | System-level | 192KB |

For our heart rate monitor, we'll use **Activity** type for continuous monitoring.

#### Step 1.2: Export SDK Environment

Expose the SDK root for tools and add `tools/` to your `PATH`.

```bash
# From the SDK root
./tools/una.py export
```

Add the printed `export` lines to your shell profile or run them in the current
terminal session.

#### Step 1.3: Initialize the Workspace

Generate or update the `.env` file for app builds.

```bash
# From the SDK root
./tools/una.py init
```

Update the `.env` entries to match your local paths:

| Variable | Description |
| --- | --- |
| `LIBS_PATH` | Absolute path to the SDK libraries (headers + sources). |
| `TOUCHGFX_GUI_PATH` | Absolute path to the TouchGFX GUI template used for GUI apps. |
| `OUTPUT_PATH` | Destination directory for built `.uapp` artifacts. |
| `RESOURCES_PATH` | Root directory for app resources (icons, assets, fonts). |
| `APP_PATH` | Root directory where app source trees live. |

#### Step 1.4: Create Project Structure

Create a new app with the new workflow. Choose **service-only** (background logic only) or **service+gui** (full UI app).

```bash
# Service + GUI app (default for Activity/Utility apps)
./tools/una.py create heart-monitor service+gui

# Service-only app (no GUI process, ideal for Glance or background services)
./tools/una.py create heart-monitor service-only
```

**Service + GUI structure (example):**

```
heart-monitor/
├── App/
│   ├── Service/
│   │   ├── Inc/
│   │   └── Src/
│   └── Gui/
│       ├── Inc/
│       └── Src/
├── Libs/
│   ├── Header/
│   └── Source/
├── Resources/
│   ├── icon_30x30.png
│   ├── icon_60x60.png
│   └── TouchGFX/
└── Output/
    └── Release/
```

**Service-only structure (example):**

```
heart-monitor/
├── App/
│   └── Service/
│       ├── Inc/
│       └── Src/
├── Libs/
│   ├── Header/
│   └── Source/
├── Resources/
│   ├── icon_30x30.png
│   └── icon_60x60.png
└── Output/
    └── Release/
```

#### Step 1.5: Understand Dual-Process Architecture

Una-Watch apps run as two separate processes:

**Service Process** (Background):
- Sensor data acquisition
- Data processing and storage
- BLE communication
- Runs continuously with low priority

**GUI Process** (Interface):
- User interface rendering
- Touch input handling
- Visual feedback
- Activated on user interaction

```cpp
// Service process entry point
void service_main() {
    HeartRateService service;
    service.run();  // Never returns - event loop
}

// GUI process entry point
void gui_main() {
    HeartRateGui gui;
    gui.run();     // TouchGFX application loop
}
```

### Phase 2: Core Implementation

#### Step 2.1: Sensor Integration (Service Process)

**Initialize Kernel Interfaces:**

```cpp
class HeartRateService : public SDK::Interface::IApp::Callback {
private:
    SDK::Kernel& kernel;

    // Core interfaces
    SDK::Interface::IAppComm* comm;
    SDK::Interface::ISystem* system;
    SDK::Interface::IFileSystem* storage;
    SDK::Interface::ILogger* logger;

    // Sensor connection
    SDK::Sensor::Connection hrSensor;

public:
    HeartRateService(SDK::Kernel& k)
        : kernel(k)
        , hrSensor(SDK::Sensor::Type::HEART_RATE, 1000.0f) { // 1000ms period

        // Get interface pointers from kernel facade
        comm = &kernel.comm;
        system = &kernel.sys;
        storage = &kernel.fs;
        logger = &kernel.log;
    }
};
```

**Request Heart Rate Sensor:**

```cpp
bool initializeSensor() {
    // Connect to default heart rate sensor
    if (!hrSensor.connect()) {
        logger->printf("Failed to connect heart rate sensor\n");
        return false;
    }

    logger->printf("Heart rate sensor connected successfully\n");
    return true;
}
```

**Main Service Loop:**

```cpp
void run() {
    if (!initializeSensor()) {
        return;  // Fatal error
    }

    while (true) {
        // Process messages from kernel
        SDK::MessageBase* msg = nullptr;
        if (comm->getMessage(msg, 100)) {
            handleMessage(msg);
            comm->releaseMessage(msg);
        }
    }
}

void handleMessage(SDK::MessageBase* msg) {
    if (msg->getType() == SDK::MessageType::EVENT_SENSOR_LAYER_DATA) {
        auto* dataEvent = static_cast<SDK::Message::Sensor::EventData*>(msg);
        if (hrSensor.matchesDriver(dataEvent->handle)) {
            processHeartRateReading(dataEvent->data[0]);
        }
    }
}
```

#### Step 2.2: Data Processing

**Heart Rate Algorithm:**

```cpp
void processHeartRateReading(const SDK::Sensor::Data& rawData) {
    // Extract heart rate value (assuming HEART_RATE type)
    float bpm = rawData.value;

    // Store reading
    storeReading(bpm);
}
```

#### Step 2.3: Data Persistence

**Store Readings in Flash:**

```cpp
bool storeReading(float bpm) {
    // Create file object
    auto file = storage->file("0:/heart-data.bin");

    // Open for append
    if (!file->open(true, false)) {
        logger->printf("Failed to open heart rate data file\n");
        return false;
    }

    // Move to end for append
    file->seek(file->size());

    // Write reading
    size_t written;
    bool result = file->write(reinterpret_cast<const char*>(&bpm), sizeof(bpm), written);

    file->close();
    return result && (written == sizeof(bpm));
}
```

#### Step 2.4: Inter-Process Communication

**Send Updates to GUI Process:**

```cpp
void sendUpdateToGui() {
    // Allocate message using the modern make_msg helper
    auto msg = SDK::make_msg<SDK::Message::HeartRateUpdate>(kernel);

    if (!msg) {
        logger->printf("Failed to allocate GUI update message\n");
        return;
    }

    // Fill message data
    msg->currentBPM = currentBPM;
    msg->averageBPM = calculateAverage();

    // Send to GUI process with 100ms timeout
    if (!msg.send(100)) {
        logger->printf("Failed to send GUI update, queue full?\n");
    }
}
```

### Phase 3: User Interface Development

#### Step 3.1: TouchGFX Integration

**GUI Process Structure:**

```cpp
class HeartRateGui : public TouchGFX::Application {
private:
    SDK::Kernel& kernel;
    SDK::Interface::IAppComm* comm;

public:
    HeartRateGui()
        : kernel(SDK::KernelProviderGUI::GetInstance().getKernel()) {
        comm = &kernel.comm;
    }
};
```

#### Step 3.2: Message Handling

**Receive Updates from Service:**

```cpp
void handleMessages() {
    SDK::MessageBase* msg = nullptr;
    while (comm->getMessage(msg, 0)) { // Non-blocking
        switch (msg->getType()) {
            case SDK::MessageType::HEART_RATE_UPDATE: {
                auto* update = static_cast<SDK::Message::HeartRateUpdate*>(msg);
                updateDisplay(update);
                break;
            }
        }
        comm->releaseMessage(msg);
    }
}
```

#### Step 3.3: UI Updates

**Update Display with New Data:**

```cpp
void updateDisplay(const SDK::Message::HeartRateUpdate* update) {
    // Update BPM text
    char bpmStr[16];
    sprintf(bpmStr, "%d BPM", update->currentBPM);
    bpmText.setText(bpmStr);

    // Update confidence indicator
    confidenceBar.setValue(update->confidence);

    // Animate heart icon based on status
    if (update->status == SDK::SensorStatus::ACTIVE) {
        heartIcon.startAnimation();
    } else {
        heartIcon.stopAnimation();
    }

    // Trigger screen refresh
    invalidate();
}
```

### Phase 4: Advanced Features

#### Step 4.1: Notification System

**Abnormal Reading Alerts:**

```cpp
void checkAbnormalReading(const HeartRateReading& reading) {
    static const uint16_t MAX_NORMAL_BPM = 100;
    static const uint16_t MIN_NORMAL_BPM = 50;

    if (reading.bpm > MAX_NORMAL_BPM || reading.bpm < MIN_NORMAL_BPM) {
        // Show glance notification
        kernel.glance->showNotification(
            "Heart Rate Alert",
            reading.bpm > MAX_NORMAL_BPM ? "High heart rate detected" :
                                         "Low heart rate detected",
            5000  // 5 second display
        );

        // Vibrate for attention
        kernel.vibro->vibrate(SDK::VibrationPattern::ALERT);
    }
}
```

#### Step 4.2: BLE Data Synchronization

**Send Data to Companion App:**

```cpp
void syncDataToPhone() {
    // Prepare data batch
    std::vector<HeartRateReading> recentReadings = getRecentReadings();

    // Create BLE message
    SDK::Message::BLE::DataSync* msg = nullptr;
    comm->allocateMessage(msg, SDK::Message::Type::BLE_DATA_SYNC);

    if (msg) {
        // Fill with heart rate data
        msg->dataType = SDK::BLE::DataType::HEART_RATE;
        msg->data = serializeReadings(recentReadings);
        msg->size = msg->data.size();

        comm->sendMessage(msg, 1000);
    }
}
```

### Phase 5: Testing and Deployment

#### Step 5.1: Build and Simulator Testing

```bash
# From the app directory
./tools/una.py build

# Optional: build a simulator target (if provided by the app CMake)
./tools/una.py build --target simulator

# Run the resulting .uapp in your simulator tooling
# (use the .uapp generated in OUTPUT_PATH/Release)
```

#### Step 5.2: Hardware Testing

```bash
# Build a release artifact
./tools/una.py build --target release

# Install the .uapp onto the watch via USB or BLE tooling
# (use the .uapp generated in OUTPUT_PATH/Release)
```

#### Step 5.3: Performance Optimization

**Memory Usage Analysis:**

```cpp
// Check memory usage
void logMemoryStats() {
    auto heapUsed = kernel.system->getHeapUsed();
    auto heapFree = kernel.system->getHeapFree();

    logger->log(SDK::LogLevel::INFO,
               "Memory: %d used, %d free (%d%%)",
               heapUsed, heapFree,
               (heapUsed * 100) / (heapUsed + heapFree));
}
```

**Power Consumption Optimization:**

```cpp
// Use appropriate sensor sampling rates
void optimizeForPower() {
    if (kernel.power->isLowBattery()) {
        // Reduce sampling frequency
        sensors->connectSensor(sensorHandle, 5000, 0);  // 5Hz -> 1Hz
    }
}
```

### Phase 6: Packaging and Distribution

#### Step 6.1: App Packaging

Packaging is handled by the build pipeline. After a successful build, the `.uapp`
artifact is written to `OUTPUT_PATH/Release`. Update your app metadata and icons
in `Resources/` as needed before building.

#### Step 6.2: OTA Deployment

Upload the `.uapp` from `OUTPUT_PATH/Release` using the companion app or your
OTA tooling pipeline.

## Key Technical Concepts Learned

### Position-Independent Code (PIC)
- Apps compiled without absolute memory addresses
- Enables kernel abstraction and process isolation
- Allows multiple apps to coexist safely

### Shared libc Architecture
- All apps share the same standard library implementation
- Reduces memory footprint per app
- Enables code reuse across applications

### Dual-Process Model
- Service process: continuous background operation
- GUI process: user interface and interaction
- IPC via message passing for data exchange

### Memory-Constrained Development
- 256KB total RAM for Activity apps
- Pool-based memory allocation
- Careful stack usage monitoring

### Power-Aware Programming
- Event-driven design over polling
- Appropriate sensor sampling rates
- Sleep mode utilization

## Next Steps

With your heart rate monitor complete, explore:

1. **Multi-Sensor Fusion**: Combine heart rate with accelerometer for activity detection
2. **Advanced UI**: Custom animations and transitions with TouchGFX
3. **Data Visualization**: Charts and graphs for historical data
4. **BLE Integration**: Real-time sync with fitness apps
5. **Cloud Connectivity**: Upload data to health platforms

## Resources

- [API Reference](api-reference.rst) - Complete SDK documentation
- [SDK Overview](sdk-overview.md) - Core concepts and tools
- [Development Workflow](development-workflow.md) - Framework details
- [Architecture Deep Dive](architecture-deep-dive.md) - Internal details
