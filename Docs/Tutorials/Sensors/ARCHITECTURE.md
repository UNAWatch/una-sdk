(tutorials/images/architecture)=

# Sensors Tutorial

In this tutorial, we implement a sensors dashboard app that subscribes to all available sensors at **maximum frequency** (period=0, count=0), processes new message structures with **timestamps**, and displays data on a **single GUI screen**:

- **Top**: Battery level
- **Middle multiline text**: Sensor data with **L1/L2 verbosity levels** (Basic/Detailed/Full)
- **Bottom**: Stats (Service/GUI CPU%, TX/RX msg rates, bytes/sec)

## List of Implemented Sensors

| Sensor | Description | Notes |
|--------|-------------|-------|
| Heart Rate | Live BPM + Trust Level | BLE calibration required for accuracy |
| GPS Location | Lat/Long/Alt | Always on; add on/off toggle if needed |
| Altimeter | Elevation (m) | Barometric pressure |
| Accelerometer | X/Y/Z G-forces | Raw acceleration |
| Step Counter | Total steps | Cumulative |
| Floor Counter | Floors ascended | Cumulative |
| Magnetometer | X/Y/Z fields (for compass) | Parsing pending; heading computation |
| RTC | UTC time | Subscribe RTC sensor if available |

## Architecture Overview

```mermaid
graph LR
    Sensors[(Sensors HW)] --> SDL[Sensor Data Layer]
    SDL -->|EVENT_SENSOR_LAYER_DATA| Service[Service Thread]
    Service -->|Custom Msgs w/ timestamps| Kernel[(Kernel)]
    Kernel -->|Custom Msgs| Model[GUI Model]
    Model --> View[MainView]
    View --> Display[Single Screen:<br/>Battery Top<br/>Multiline Data (L1/L2)<br/>Bottom Stats]
    Buttons[L1/L2 Buttons] -.-> View
```

## Implementation Steps

### Step 1: Define Custom Messages

Create [`Commands.hpp`](Docs/Tutorials/Sensors/Software/Libs/Header/Commands.hpp) with message types and structs:

```cpp
namespace CustomMessage {
    constexpr SDK::MessageType::Type HR_VALUES = 0x00000001;
    // LOCATION_VALUES, ELEVATION_VALUES, etc.

    struct HRValues : public SDK::MessageBase {
        float heartRate;
        float trustLevel;
        HRValues() : SDK::MessageBase(HR_VALUES), heartRate(0), trustLevel(0) {}
    };
    // Similar for others: include uint64_t timestamp where available
}
```

Add `GUISender` class with `updateHeartRate(float bpm, float tl)` etc. using `SDK::make_msg`.

### Step 2: Service - Subscribe & Process Sensors

In [`Service.hpp`](Docs/Tutorials/Sensors/Software/Libs/Header/Service.hpp):

```cpp
SDK::Sensor::Connection mSensorHR{SDK::Sensor::Type::HEART_RATE, 0, 0};  // Max freq
// Repeat for GPS_LOCATION, ALTIMETER, ACCELEROMETER, STEP_COUNTER, FLOOR_COUNTER, MAGNETIC_FIELD
```

In `run()` [`Service.cpp`](Docs/Tutorials/Sensors/Software/Libs/Sources/Service.cpp): connect all, loop getMessage:

```cpp
case SDK::MessageType::EVENT_SENSOR_LAYER_DATA: {
    auto event = static_cast<SDK::Message::Sensor::EventData*>(msg);
    SDK::Sensor::DataBatch data(event->data, event->count, event->stride);
    onSdlNewData(event->handle, data);
} break;
```

In `onSdlNewData`:

```cpp
if (mSensorHR.matchesDriver(handle)) {
    SDK::SensorDataParser::HeartRate parser(data[0]);
    if (parser.isDataValid()) {
        mSender.updateHeartRate(parser.getBpm(), parser.getTrustLevel());
    }
}
// Similar parsers for each sensor, send with timestamp if avail (e.g. parser.getTimestamp())
```

Track stats (CPU, msg rates) every 1s, extend to send `STATS_VALUES`.

### Step 3: GUI Model - Receive Messages

In [`Model.cpp`](Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/gui/src/model/Model.cpp), implement `customMessageHandler`:

```cpp
case CustomMessage::HR_VALUES: {
    auto* m = static_cast<CustomMessage::HRValues*>(msg);
    modelListener->updateHR(m->heartRate, m->trustLevel);
} break;
// For all types
```

### Step 4: MainView - Display & Controls

In [`MainView.hpp`](Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/gui/include/gui/main_screen/MainView.hpp)/cpp:

Store data in members, `updateHR(float hr, float tl)` etc. call `refreshDisplay()`.

`handleKeyEvent`: L1++ verbosity, L2-- (BASIC/DETAILED/FULL)

`refreshDisplay()`: format multiline:

```cpp
// BASIC
"Time: %lu\nHR: %.0f BPM\nSteps: %lu\n",
// DETAILED + GPS etc.
// FULL + Mag
Unicode::strncpy(text_bodyBuffer, buffer, TEXT_BODY_SIZE);
```

Header: Battery

Stats: CPU S/G, Msg Tx/Rx, Bytes Tx/Rx

R2: presenter->exit();

### Additional Notes

- **BLE Calibration**: Send calibration command before HR subscription.
- **GPS On/Off**: Add R1 toggle, send command to service to connect/disconnect.
- **RTC UTC**: Use `SDK::Sensor::Type::RTC` or kernel time.
- **Max Frequency**: `Connection(type, 0 /*period*/, 0 /*count*/)` for continuous.
- Build with CMakeLists.txt in Apps/Sensors-CMake.
