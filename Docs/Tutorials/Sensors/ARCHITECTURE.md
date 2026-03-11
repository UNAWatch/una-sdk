(tutorials/images/architecture)=

# Sensors Tutorial

In this tutorial, we implement a sensors dashboard app that subscribes to available sensors at **maximum frequency** (period=0, count=0 except Accelerometer), processes new message structures with **timestamps** where available, and displays data on a **single GUI screen**:

- **Top**: Battery level (TODO: implement)
- **Middle multiline text**: Sensor data with **L1/L2 verbosity levels** (BASIC/DETAILED/FULL)
- **Bottom**: Stats (Service/GUI CPU%, TX/RX msg rates, bytes/sec)

## List of Implemented Sensors

| Sensor | Description | Notes |
|--------|-------------|-------|
| Heart Rate | Live BPM + Trust Level | BLE calibration TODO |
| GPS Location | Lat/Long/Alt (float) | Always on; toggle TODO |
| Altimeter | Elevation (m) | Barometric pressure parsing TODO |
| Accelerometer | X/Y/Z G-forces | connect(period=10, count=10); sender throttled ~100ms |
| Step Counter | Total steps | Cumulative |
| Floor Counter | Floors ascended | Cumulative; parser.getFloorsUp() |
| Magnetometer | X/Y/Z fields (for compass) | Parsing & connection pending; heading computation TODO |
| RTC | Time (sec since boot) | From kernel sys.getTimeMs()/1000; not sensor |

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
    constexpr SDK::MessageType::Type LOCATION_VALUES = 0x00000002;
    constexpr SDK::MessageType::Type ELEVATION_VALUES = 0x00000003;
    constexpr SDK::MessageType::Type ACCELEROMETER_VALUES = 0x00000004;
    constexpr SDK::MessageType::Type STEP_COUNTER_VALUES = 0x00000005;
    constexpr SDK::MessageType::Type FLOORS_VALUES = 0x00000006;
    constexpr SDK::MessageType::Type COMPASS_VALUES = 0x00000007; // pending
    constexpr SDK::MessageType::Type STATS_VALUES = 0x00000008;
    constexpr SDK::MessageType::Type RTC_VALUES = 0x00000009;

    struct HRValues : public SDK::MessageBase {
        float heartRate;
        float trustLevel;
        HRValues() : SDK::MessageBase(HR_VALUES), heartRate(0), trustLevel(0) {}
    };
    // Similar for others:
    // LocationValues: uint64_t timestamp; double latitude, longitude, altitude;
    // ElevationValues: uint64_t timestamp; float elevation;
    // AccelerometerValues: uint64_t timestamp; float x,y,z;
    // StepCounterValues: uint64_t timestamp; uint32_t steps;
    // FloorsValues: uint64_t timestamp; uint32_t floors;
    // CompassValues: uint64_t timestamp; float heading;
    // StatsValues: float serviceCpuPct, guiCpuPct, txMsgRate, rxMsgRate, txByteRate, rxByteRate;
    // RtcValues: uint32_t time;
}

class GUISender {
public:
    GUISender(const SDK::Kernel& kernel) : mKernel(kernel) {}
    bool updateHeartRate(float bpm, float tl) {
        if (auto req = SDK::make_msg<CustomMessage::HRValues>(mKernel)) {
            req->heartRate = bpm;
            req->trustLevel = tl;
            return req.send();
        }
        return false;
    }
    // Similar updateLocation(uint64_t ts, double lat, double lon, double alt), etc.
};
```

### Step 2: Service - Subscribe & Process Sensors

In [`Service.hpp`](Docs/Tutorials/Sensors/Software/Libs/Header/Service.hpp):

```cpp
SDK::Sensor::Connection mSensorHR{SDK::Sensor::Type::HEART_RATE, 0, 0};
SDK::Sensor::Connection mSensorGPS{SDK::Sensor::Type::GPS_LOCATION, 0, 0};
SDK::Sensor::Connection mSensorAltimeter{SDK::Sensor::Type::ALTIMETER, 0, 0};
SDK::Sensor::Connection mSensorAccelerometer{SDK::Sensor::Type::ACCELEROMETER, 0, 0};
SDK::Sensor::Connection mSensorStepCounter{SDK::Sensor::Type::STEP_COUNTER, 0, 0};
SDK::Sensor::Connection mSensorFloorCounter{SDK::Sensor::Type::FLOOR_COUNTER, 0, 0};
// CustomMessage::GUISender mSender;
```

In `run()` [`Service.cpp`](Docs/Tutorials/Sensors/Software/Libs/Sources/Service.cpp): connect all (acc.connect(10,10)), loop getMessage:

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
// Similar for GPS: parser.getLatitude() etc (float), updateLocation(ts, lat,lon,alt);
// Altimeter: updateElevation(ts, parser.getAltitude());
// Accel: if (nowMs - mLastAccTimeMs >= 100) updateAccelerometer(ts, x,y,z);
// Steps: updateStepCounter(ts, parser.getStepCount());
// Floors: updateFloors(ts, parser.getFloorsUp());
```

Track stats every 1s (simplistic CPU% = ms/10, rates=counts/sec), `mSender.updateStats(...)`, `updateRtc(timeMs/1000)`.

### Step 3: GUI Model - Receive Messages

In [`Model.cpp`](Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/gui/src/model/Model.cpp), implement `customMessageHandler`:

```cpp
case CustomMessage::HR_VALUES: {
    auto* m = static_cast<CustomMessage::HRValues*>(msg);
    modelListener->updateHR(m->heartRate, m->trustLevel);
} break;
// Similar cases for all types: LOCATION_VALUES -> updateGPS(m->latitude, m->longitude, m->altitude);
// etc. for Elevation, Accelerometer, StepCounter, Floors, Compass, Stats, RTC
```

### Step 4: MainView - Display & Controls

In [`MainView.hpp`](Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/gui/include/gui/main_screen/MainView.hpp)[`MainView.cpp`](Docs/Tutorials/Sensors/Software/Apps/TouchGFX-GUI/gui/src/main_screen/MainView.cpp):

Store data in members, `updateHR(float hr, float tl)` etc. store values.

`handleKeyEvent`: L1: verbosity++ %3, L2: verbosity-- %3 (BASIC/DETAILED/FULL), R2: presenter->exit()

`handleTickEvent()` every tick: `refreshDisplay()` `refreshStats()` `refreshBattery()`

`refreshDisplay()`: format multiline in `text_body`:

```
# %lu
Time: %lu
HR: %.0f BPM
Steps: %lu
[DETAILED]
GPS: %.2f, %.2f, %.0f
Alt: %.1f Pa, %.1f m  // TODO pressure
Acc: %.2f, %.2f, %.2f
Floors: %lu
[FULL]
Mag: %.2f, %.2f, %.2f  // TODO
```
Unicode::strncpy(text_bodyBuffer, buffer, TEXT_BODY_SIZE); invalidate

Header: `refreshBattery()` "Battery: %.1f%%" `text_header`  // TODO impl

Stats: `refreshStats()` "CPU S: %.1f%% G: %.1f%%\nMsg Tx: %.0f Rx: %.0f\nBytes Tx: %.0f Rx: %.0f" `text_stats`

### Additional Notes

- **BLE Calibration**: TODO send before HR subscription.
- **GPS On/Off**: TODO R1 toggle, service connect/disconnect.
- **Battery**: updateBattery() exists but not called; TODO from kernel BatteryLevel?
- **Altimeter Pressure**: TODO parser.getPressure()?
- **Magnetometer/Compass**: TODO subscribe, parse fields, compute heading.
- **Max Frequency**: period=0,count=0 except Accel connect(10,10); sender Accel throttle 100ms.
- **RTC**: Kernel time, not SDK::Sensor::RTC.
- Build with [`CMakeLists.txt`](Docs/Tutorials/Sensors/Software/Apps/Sensors-CMake/CMakeLists.txt).
