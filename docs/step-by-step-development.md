# Step-by-Step App Development Guide

This comprehensive guide walks through the complete Una-Watch app development process, from concept to deployment. We'll build a heart rate monitor app that demonstrates the platform's unique capabilities.

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

#### Step 1.2: Create Project Structure

```bash
# Use the SDK's project generator
./sdk-tools/create-activity.sh heart-monitor \
  --name "Heart Rate Monitor" \
  --autostart \
  --capabilities "sensor.heart_rate,notification.display,storage.write"

# Resulting structure:
heart-monitor/
├── Software/
│   ├── App/
│   │   └── HeartMonitor-CubeIDE/
│   │       ├── Core/
│   │       │   ├── Inc/
│   │       │   └── Src/
│   │       └── SDK/          # SDK headers
│   └── Libs/
│       ├── Header/
│       │   ├── Service.hpp   # Background logic
│       │   └── Gui.hpp       # UI logic
│       └── Source/
│           ├── Service.cpp
│           └── Gui.cpp
├── Resources/
│   ├── icon_30x30.png
│   ├── icon_60x60.png
│   └── TouchGFX/            # UI assets
└── Output/
    └── Release/
```

#### Step 1.3: Understand Dual-Process Architecture

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
    SDK::KernelProviderService kernel;

    // Core interfaces
    SDK::Interface::IAppComm* comm;
    SDK::Interface::ISystem* system;
    SDK::Interface::ISensorManager* sensors;
    SDK::Interface::IFileSystem* storage;
    SDK::Interface::ILogger* logger;

public:
    HeartRateService()
        : kernel(SDK::KernelProviderService::GetInstance()) {

        // Get interface pointers
        comm = kernel.comm;
        system = kernel.system;
        sensors = kernel.sensors;
        storage = kernel.storage;
        logger = kernel.logger;
    }
};
```

**Request Heart Rate Sensor:**

```cpp
bool initializeSensor() {
    // Request default heart rate sensor
    SDK::Sensor::Handle sensorHandle;
    auto result = sensors->requestDefaultSensor(
        SDK::Sensor::Type::HEART_RATE,
        sensorHandle
    );

    if (!result) {
        logger->log(SDK::LogLevel::ERROR,
                   "Failed to get heart rate sensor");
        return false;
    }

    // Configure sensor with 1Hz sampling
    result = sensors->connectSensor(sensorHandle, 1000, 0);
    if (!result) {
        logger->log(SDK::LogLevel::ERROR,
                   "Failed to connect heart rate sensor");
        return false;
    }

    return true;
}
```

**Main Service Loop:**

```cpp
void run() {
    if (!initializeSensor()) {
        return;  // Fatal error
    }

    while (!shouldTerminate()) {
        // Get sensor data
        SDK::Sensor::Data sensorData;
        if (sensors->getSensorData(sensorHandle, sensorData)) {
            processHeartRateReading(sensorData);
        }

        // Send updates to GUI every second
        sendUpdateToGui();

        // Sleep to conserve power
        system->delay(1000);
    }
}
```

#### Step 2.2: Data Processing

**Heart Rate Algorithm:**

```cpp
struct HeartRateReading {
    uint16_t bpm;
    uint8_t confidence;  // 0-100%
    uint32_t timestamp;
    bool isValid;
};

void processHeartRateReading(const SDK::Sensor::Data& rawData) {
    // Extract PPG sensor data
    auto& ppgData = static_cast<const SDK::Sensor::PPGData&>(rawData);

    // Apply heart rate algorithm
    HeartRateReading reading = heartRateAlgorithm.process(ppgData);

    // Validate reading quality
    if (reading.confidence < 70) {
        logger->log(SDK::LogLevel::WARN,
                   "Low confidence heart rate reading: %d%%",
                   reading.confidence);
        return;
    }

    // Store reading
    storeReading(reading);

    // Check for abnormal readings
    checkAbnormalReading(reading);
}
```

#### Step 2.3: Data Persistence

**Store Readings in Flash:**

```cpp
bool storeReading(const HeartRateReading& reading) {
    // Open storage file (internal flash: 0:/heart-data.bin)
    SDK::FileHandle file;
    auto result = storage->open(file, "0:/heart-data.bin",
                               SDK::FileMode::APPEND);

    if (!result) {
        logger->log(SDK::LogLevel::ERROR,
                   "Failed to open heart rate data file");
        return false;
    }

    // Write reading as CBOR
    SDK::CBOR::Writer writer;
    writer.write("bpm", reading.bpm);
    writer.write("confidence", reading.confidence);
    writer.write("timestamp", reading.timestamp);

    size_t written;
    result = storage->write(file, writer.getBuffer(),
                           writer.getSize(), written);

    storage->close(file);
    return result && (written == writer.getSize());
}
```

#### Step 2.4: Inter-Process Communication

**Send Updates to GUI Process:**

```cpp
void sendUpdateToGui() {
    // Allocate message from kernel pool
    SDK::Message::HeartRateUpdate* msg = nullptr;
    auto result = comm->allocateMessage(msg,
                                       SDK::Message::Type::HEART_RATE_UPDATE);

    if (!result || !msg) {
        logger->log(SDK::LogLevel::ERROR,
                   "Failed to allocate GUI update message");
        return;
    }

    // Fill message data
    msg->currentBPM = currentBPM;
    msg->averageBPM = calculateAverage();
    msg->status = sensorStatus;

    // Send to GUI process
    result = comm->sendMessage(msg, 100);  // 100ms timeout

    if (!result) {
        logger->log(SDK::LogLevel::WARN,
                   "Failed to send GUI update, queue full?");
    }
}
```

### Phase 3: User Interface Development

#### Step 3.1: TouchGFX Integration

**GUI Process Structure:**

```cpp
class HeartRateGui : public TouchGFX::Application {
private:
    SDK::KernelProviderGUI kernel;
    SDK::Interface::IAppComm* comm;

    // UI Components
    TouchGFX::TextArea bpmText;
    TouchGFX::Image heartIcon;
    TouchGFX::ProgressBar confidenceBar;

public:
    HeartRateGui()
        : kernel(SDK::KernelProviderGUI::GetInstance()) {
        comm = kernel.comm;
    }

    void setupScreen() {
        // Initialize UI components
        add(bpmText);
        add(heartIcon);
        add(confidenceBar);

        // Set initial values
        bpmText.setText("--- BPM");
        confidenceBar.setValue(0);
    }
};
```

#### Step 3.2: Message Handling

**Receive Updates from Service:**

```cpp
bool getMessage(SDK::Message::Base*& msg, uint32_t timeout) {
    return comm->getMessage(msg, timeout);
}

void handleMessage(const SDK::Message::Base* msg) {
    switch (msg->type) {
        case SDK::Message::Type::HEART_RATE_UPDATE: {
            auto* update = static_cast<const SDK::Message::HeartRateUpdate*>(msg);
            updateDisplay(update);
            break;
        }

        case SDK::Message::Type::SENSOR_STATUS_CHANGE: {
            auto* status = static_cast<const SDK::Message::SensorStatus*>(msg);
            updateSensorStatus(status);
            break;
        }
    }

    // Send response if required
    if (msg->expectsResponse()) {
        comm->sendResponse(nullptr);  // Ack message
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

#### Step 5.1: Simulator Testing

```bash
# Build for simulator
make simulator

# Run in simulator with mock sensor data
./simulator heart-monitor.uapp --mock-sensors

# Test different scenarios:
# - Normal heart rate readings
# - Abnormal readings (notifications)
# - Sensor disconnection
# - Low battery conditions
```

#### Step 5.2: Hardware Testing

```bash
# Build for device
make release

# Flash to connected watch
make flash

# Monitor logs via serial
make monitor
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

```bash
# Package app with metadata
./sdk-tools/package-app.sh \
  --input heart-monitor.elf \
  --output heart-monitor.uapp \
  --name "Heart Rate Monitor" \
  --version "1.0.0" \
  --type activity \
  --capabilities "sensor.heart_rate,notification.display,storage.write" \
  --icons icon_30x30.png icon_60x60.png
```

#### Step 6.2: OTA Deployment

**Upload to App Store:**

```bash
# Submit to Una-Watch app store
./sdk-tools/submit-app.sh \
  --app heart-monitor.uapp \
  --metadata app-metadata.json \
  --screenshots screenshot1.png screenshot2.png \
  --description "Professional heart rate monitoring with trend analysis"
```

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

- [API Reference](index.rst) - Complete SDK documentation
- [SDK Overview](sdk-overview.md) - Core concepts and tools
- [App Development](app-development.md) - Framework details
- [Kernel Architecture](kernel-architecture.md) - Internal details

---

**Development Time**: 2-3 hours
**Concepts Covered**: PIC execution, shared libc, dual-process architecture, sensor integration, IPC, UI development, BLE communication, power optimization
**Skills Gained**: Full-stack Una-Watch development, performance tuning, cross-platform deployment