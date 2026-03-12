# Sensor Layer

## Overview

The SensorLayer provides a high-level C++ API for subscribing to and receiving data from various sensors via IPC messages through the SDK Kernel. It abstracts low-level driver interactions, allowing applications to connect to sensors by type, specify sampling periods and latency, and receive batched sensor data events.

Key components:
- [`SDK::Sensor::Connection`](Libs/Header/SDK/SensorLayer/SensorConnection.hpp): Manages subscription (RequestDefault) and connection (RequestConnect/Disconnect).
- Sensor data delivery via [`SDK::Message::Sensor::EventData`](Libs/Header/SDK/Messages/SensorLayerMessages.hpp) messages.
- Data processing with [`SDK::Sensor::DataBatch`](Libs/Header/SDK/SensorLayer/SensorDataBatch.hpp), [`SDK::Sensor::DataView`](Libs/Header/SDK/SensorLayer/SensorDataView.hpp), and type-specific parsers in DataParsers/.

## Sensor Types

All available sensor types are defined in [`SDK::Sensor::Type`](Libs/Header/SDK/SensorLayer/SensorTypes.hpp):

| Category | Type | Hex Value | Description | Parser Available | Fields |
|----------|------|-----------|-------------|------------------|--------|
| IMU | ACCELEROMETER | 0x10 | Acceleration (3-axis) | Yes | X,Y,Z (float g) - 3 |
| IMU | ACCELEROMETER_RAW | 0x11 | Acceleration raw | Yes | X,Y,Z (int16 raw) - 3 |
| IMU | GYROSCOPE | 0x20 | Angular rate (3-axis) | No | - |
| IMU | GYROSCOPE_RAW | 0x21 | Angular rate raw | No | - |
| IMU | MAGNETIC_FIELD | 0x30 | Magnetic field (3-axis) | No | - |
| Cardio | HEART_BEAT | 0x40 | Beat peak event | No | - |
| Cardio | HEART_RATE | 0x41 | Current heart rate (bpm) | Yes | BPM (float), TRUST_LEVEL (float) - 2 |
| Cardio | HEART_RATE_METRICS | 0x42 | Aggregated metrics (AHR, RHR) | Yes | AHR (float bpm), RHR (float bpm) - 2 |
| Pedometer | STEP_DETECTOR | 0x50 | Step event | Yes | STEP_DETECTED (u32=1) - 1 |
| Pedometer | STEP_COUNTER | 0x51 | Step count since reboot | Yes | STEP_COUNT (u32) - 1 |
| Pedometer | FLOOR_COUNTER | 0x60 | Floor counter | Yes | FLOORS_UP (i32), FLOORS_DOWN (i32) - 2 |
| Ambient | AMBIENT_TEMPERATURE | 0x70 | Ambient temperature | Yes | TEMP (float °C?) - 1 |
| Ambient | PRESSURE | 0x80 | Atmospheric pressure | Yes | PRESS (float Pa), PRESS_SEA_LEVEL (float Pa) - 2 |
| Ambient | ALTIMETER | 0x90 | Altimeter | Yes | ALTITUDE (float m) - 1 |
| Wrist | WRIST_MOTION | 0xA0 | Wrist-motion event | Yes | WRIST_MOTION (u32=1) - 1 |
| Motion | MOTION_DETECT | 0xB0 | Motion states (NO_MOTION, MOTION, SIG_MOTION) | Yes | ID (u32 enum) - 1 |
| Motion | ACTIVITY_RECOGNITION | 0xC0 | Activity classification (STILL, WALKING, RUNNING, UNKNOWN) | Yes | ID (u32 enum), CONFIDENCE (u8 %) - 2 |
| Motion | GESTURE_RECOGNITION | 0xD0 | Discrete gesture events | No | - |
| Daily | ACTIVITY | 0xE0 | Active minutes (minutes) | Yes | DURATION (u32 ms) - 1 |
| Optical | PPG | 0xF0 | Photoplethysmogram | No | - |
| Optical | ECG | 0x100 | Electrocardiogram | No | - |
| GPS | GPS_LOCATION | 0x110 | GNSS location | Yes | PRECISION (f m), COORDS_VALID (bool), LAT (f deg), LON (f deg), ALT (f m) - 5 |
| GPS | GPS_SPEED | 0x111 | GNSS speed | Yes | SPEED (f m/s) - 1 |
| GPS | GPS_DISTANCE | 0x112 | GNSS distance/odometer | Yes | DISTANCE (f m) - 1 |
| Battery | BATTERY_LEVEL | 0x120 | Charge level (%) | Yes | LEVEL (f 0-100) - 1 |
| Battery | BATTERY_CHARGING | 0x121 | Charging state | Yes | CONNECTED (u32 bool), CHARGING (u32 bool) - 2 |
| Battery | BATTERY_METRICS | 0x122 | Voltage/current/capacity | Yes | VOLTAGE (f V), CURRENT (f mA), AVG_CURRENT (f mA), CAPACITY (f mAh), DESIGN_CAPACITY (f mAh) - 5 |
| Fusion | FUSION | 0x130 | Fused IMU | No | - |
| Fusion | FUSION_RAW | 0x131 | Raw fusion inputs | No | - |
| Touch | TOUCH_DETECT | 0x140 | Touch/worn/unworn | Yes | TOUCH (u32 bool) - 1 |

## Connection API

Use [`SDK::Sensor::Connection`](Libs/Header/SDK/SensorLayer/SensorConnection.hpp):

Constructors:
- `Connection(Type id, float period = 0, uint32_t latency = 0)`: Lazy resolve handle on connect.
- `Connection(uint8_t handle, float period = 0, uint32_t latency = 0)`: Use existing handle.

Methods:
- `bool connect()`: Subscribe if needed, connect with stored params.
- `bool connect(float period, uint32_t latency)`: Update params, connect.
- `bool isValid()`: Handle != 0.
- `bool isConnected()`: Active connection.
- `void disconnect()`: Send RequestDisconnect.
- `bool matchesDriver(uint16_t handle)`: Compare handles.

Destructor auto-disconnects.

Implementation in [SensorConnection.cpp](Libs/Source/SensorLayer/SensorConnection.cpp).

## Data Reception & Processing

**Reception**: Poll kernel IPC:

```cpp
SDK::Kernel& kernel = SDK::KernelProviderService::GetInstance().getKernel();
SDK::MessageBase* msg;
while (kernel.comm.getMessage(msg, 1000)) {
    if (msg->getType() == SDK::MessageType::EVENT_SENSOR_LAYER_DATA) {
        SDK::Message::Sensor::EventData* event = static_cast<SDK::Message::Sensor::EventData*>(msg);
        SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
        processBatch(event->handle, batch);
    }
    kernel.comm.releaseMessage(msg);
}
```

**Processing**:
- `DataBatch`: Iterable views (`batch.size()`, `batch[i]`).
- `DataView`: Timestamps (`getTimestamp()`, `getTimestampUs()`), fields (`f[0]`, `u[0]`, `i[0]` float/u32/i32).

See [SensorData.hpp](Libs/Header/SDK/SensorLayer/SensorData.hpp), [DataView.hpp](Libs/Header/SDK/SensorLayer/SensorDataView.hpp).

## Sensors

Detailed usage for each supported sensor (with parser). For others, use DataView directly.

### ACCELEROMETER (0x10)

**Parser**: `SDK::SensorDataParser::Accelerometer`

**Fields**:
| Index | Name | Type | Unit |
|-------|------|------|------|
| 0 | X | float | g |
| 1 | Y | float | g |
| 2 | Z | float | g |

**Code Snippet**:
```cpp
SDK::Sensor::Connection conn(SDK::Sensor::Type::ACCELEROMETER, 0.1f);
conn.connect();

void processBatch(uint16_t handle, SDK::Sensor::DataBatch& batch) {
    if (conn.matchesDriver(handle)) {
        for (uint16_t i = 0; i < batch.size(); ++i) {
            SDK::SensorDataParser::Accelerometer p(batch[i]);
            if (p.isDataValid()) {
                float x = p.getX(), y = p.getY(), z = p.getZ();
                uint64_t ts = p.getTimestampUs();
                // Use data
            }
        }
    }
}
```

### ACCELEROMETER_RAW (0x11)

**Parser**: `SDK::SensorDataParser::AccelerometerRaw`

**Fields**: X/Y/Z (int16_t raw)

**Code Snippet**:
Similar, `int16_t x = p.getX();` etc.

### HEART_RATE (0x41)

**Parser**: `SDK::SensorDataParser::HeartRate`

**Fields**:
| Index | Name | Type | Unit |
|-------|------|------|------|
| 0 | BPM | float | bpm |
| 1 | TRUST_LEVEL | float | - |

**Code Snippet**:
As above, `float bpm = p.getBpm(); float trust = p.getTrustLevel();`

### HEART_RATE_METRICS (0x42)

**Parser**: `SDK::SensorDataParser::HeartRateMetrics`

**Fields**:
| Index | Name | Type | Unit |
|-------|------|------|------|
| 0 | AHR | float | bpm |
| 1 | RHR | float | bpm |

### STEP_DETECTOR (0x50)

**Parser**: `SDK::SensorDataParser::StepDetector`

**Fields**: STEP_DETECTED (u32 =1)

**Code**: `if (p.isStepDetected()) { /* step */ }`

### STEP_COUNTER (0x51)

**Parser**: `SDK::SensorDataParser::StepCounter`

**Fields**: STEP_COUNT (u32)

### FLOOR_COUNTER (0x60)

**Parser**: `SDK::SensorDataParser::FloorCounter`

**Fields**: FLOORS_UP (i32), FLOORS_DOWN (i32)

### AMBIENT_TEMPERATURE (0x70)

**Parser**: `SDK::SensorDataParser::Temperature`

**Fields**: TEMP (float)

### PRESSURE (0x80)

**Parser**: `SDK::SensorDataParser::Pressure`

**Fields**: PRESS (float Pa), PRESS_SEA_LEVEL (float Pa)

### ALTIMETER (0x90)

**Parser**: `SDK::SensorDataParser::Altimeter`

**Fields**: ALTITUDE (float m)

### WRIST_MOTION (0xA0)

**Parser**: `SDK::SensorDataParser::WristMotion`

**Fields**: WRIST_MOTION (u32=1)

`if (p.isWristMotion()) { /* raise wrist */ }`

### MOTION_DETECT (0xB0)

**Parser**: `SDK::SensorDataParser::MotionDetect`

**Fields**: ID (u32: NO_MOTION=0, MOTION, SIG_MOTION)

`MotionDetect::Motion m = p.getID();`

### ACTIVITY_RECOGNITION (0xC0)

**Parser**: `SDK::SensorDataParser::ActivityRecognition`

**Fields**: ID (STILL=0, WALKING, RUNNING, UNKNOWN), CONFIDENCE (u8 %)

### ACTIVITY (0xE0)

**Parser**: `SDK::SensorDataParser::Activity`

**Fields**: DURATION (u32 ms)

### GPS_LOCATION (0x110)

**Parser**: `SDK::SensorDataParser::GpsLocation`

**Fields**: PRECISION (f m), COORDS_VALID (bool), LAT/LON (f deg), ALT (f m)

`p.getLatitude()`, `p.isCoordinatesValid()`

### GPS_SPEED (0x111)

**Parser**: `SDK::SensorDataParser::GpsSpeed`

**Fields**: SPEED (f m/s)

### GPS_DISTANCE (0x112)

**Parser**: `SDK::SensorDataParser::GpsDistance`

**Fields**: DISTANCE (f m)

### BATTERY_LEVEL (0x120)

**Parser**: `SDK::SensorDataParser::BatteryLevel`

**Fields**: LEVEL (f 0-100 %)

### BATTERY_CHARGING (0x121)

**Parser**: `SDK::SensorDataParser::BatteryCharging`

**Fields**: CONNECTED (bool), CHARGING (bool)

### BATTERY_METRICS (0x122)

**Parser**: `SDK::SensorDataParser::BatteryMetrics`

**Fields**: VOLTAGE (f V), CURRENT (f mA), AVERAGE_CURRENT (f mA), CAPACITY (f mAh), DESIGN_CAPACITY (f mAh)

### TOUCH_DETECT (0x140)

**Parser**: `SDK::SensorDataParser::Touch`

**Fields**: TOUCH (bool)

For sensors without parsers (e.g. GYROSCOPE), use DataView:
```cpp
SDK::Sensor::DataView view = batch[0];
float gyroX = view.f[0]; // Assume layout known from driver docs
```

## Workflow Diagram

```mermaid
graph TD
    A[Connection(Type, period)] --> B[conn.connect()]
    B --> C[Kernel: EventData(handle, data[], count, stride)]
    C --> D[app.getMessage() -> EventData]
    D --> E[DataBatch batch]
    E --> F[DataView view = batch[0]]
    F --> G[Parser p(view)]
    G --> H{isDataValid?}
    H -->|Yes| I[Process fields]
```

## Example: Multi-Sensor Service

Adapted from tutorial [`Service.cpp`](Docs/Tutorials/Sensors/Software/Libs/Sources/Service.cpp):

```cpp
// In app service loop
SDK::Sensor::Connection hr(SDK::Sensor::Type::HEART_RATE);
hr.connect(); // Auto-subscribe & connect

while (true) {
    SDK::MessageBase* msg;
    if (kernel.comm.getMessage(msg, 1000)) {
        if (msg->getType() == SDK::MessageType::EVENT_SENSOR_LAYER_DATA) {
            auto* event = static_cast<SDK::Message::Sensor::EventData*>(msg);
            SDK::Sensor::DataBatch batch(event->data, event->count, event->stride);
            if (hr.matchesDriver(event->handle)) {
                auto parser = SDK::SensorDataParser::HeartRate(batch[0]);
                if (parser.isDataValid()) {
                    float bpm = parser.getBpm();
                    // Send to GUI or process
                }
            }
        }
        kernel.comm.releaseMessage(msg);
    }
}
hr.disconnect();
```

Full tutorial in Docs/Tutorials/Sensors/.

## Additional Messages

- RequestList: List handles for type.
- RequestGetDesc: Sensor descriptor string.

See [`SensorLayerMessages.hpp`](Libs/Header/SDK/Messages/SensorLayerMessages.hpp).
## Full Example

See tutorial [Service.hpp/cpp](Docs/Tutorials/Sensors/Software/Libs/Header/Service.hpp) for multi-sensor handling with GUI comm.