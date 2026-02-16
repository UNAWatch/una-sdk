# HelloWorld Architecture

## Problem Statement

Provide a simple heart rate monitoring tutorial that demonstrates basic sensor integration, FIT file logging for activity data, and real-time TouchGFX GUI updates. The app connects to the heart rate sensor, displays live HR values on the screen, and logs activity records to a FIT file while the GUI is active.

## Program Architecture

### 1) Kernel entry and service lifetime

- `Service` is constructed with a reference to the kernel (`SDK::Kernel`) and runs as the app's main service entry point.
- `Service::run()` is the main loop. It blocks on `mKernel.comm.getMessage()` and reacts to GUI start/stop notifications and sensor data messages.
- The service connects to the heart rate sensor on startup and disconnects only on `COMMAND_APP_STOP`.
- FIT logging occurs only while the GUI is active (`mGUIStarted = true`), with records written every second.

### 2) Kernel message loop

`Service::run()` handles the following message types in order:

- `COMMAND_APP_NOTIF_GUI_RUN`
  - Sets `mGUIStarted = true` and sends initial HR values (0.0) to the GUI.
- `COMMAND_APP_NOTIF_GUI_STOP`
  - Sets `mGUIStarted = false`, stopping FIT logging.
- `EVENT_SENSOR_LAYER_DATA`
  - Dispatches heart rate sensor batches into `onSdlNewData()`.
- `COMMAND_APP_STOP`
  - Writes final lap and track data to FIT, disconnects sensor, and returns from `run()`.

### 3) UI layer (TouchGFX GUI)

- The GUI is built with TouchGFX and displays heart rate values in real-time.
- `Model` receives custom messages from the service via `customMessageHandler()` and updates the view through the presenter.
- The main screen shows current heart rate and trust level.
- GUI lifecycle is managed via `onStart()`, `onResume()`, `onStop()`, `onSuspend()` callbacks.

### 4) Sensor layer

- `SDK::Sensor::Connection` subscribes to `HEART_RATE` sensor.
- Configured with 1000ms sample period and 2000ms timeout.
- Incoming data is parsed using `SDK::SensorDataParser::HeartRate` to extract BPM and trust level.
- Sensor data is only processed when the GUI is active.

### 5) Core heart rate logic

- Heart rate samples are validated using `parser.isDataValid()`.
- Valid HR values (BPM and trust level) are stored in `mHR` and `mHRTL`.
- Every second while GUI is active, a FIT record is created with current HR data.
- Aggregates are maintained for average and max HR over the session.

### 6) Persistence (FIT)

- A single FIT file is created for the activity session.
- `ActivityWriter` handles FIT file creation, record addition, lap data, and track summary.
- FIT file structure includes file ID, activity records, lap summary, and track summary using `SDK::Component::FitHelper`.
- On app exit, lap and track data are written with session statistics (avg HR, max HR, duration).

## Implementation Walkthrough

1. **App layout**
   - Service logic lives in [`Software/Libs/Header/Service.hpp`](Docs/Tutorials/HelloWorld/Software/Libs/Header/Service.hpp:1) and [`Software/Libs/Sources/Service.cpp`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:1).
   - ActivityWriter for FIT serialization in [`Software/Libs/Header/ActivityWriter.hpp`](Docs/Tutorials/HelloWorld/Software/Libs/Header/ActivityWriter.hpp:1) and [`Software/Libs/Sources/ActivityWriter.cpp`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/ActivityWriter.cpp:1).
   - GUI code under [`Software/Apps/TouchGFX-GUI/`](Docs/Tutorials/HelloWorld/Software/Apps/TouchGFX-GUI:1).
   - App build wiring in [`Software/Apps/HelloWorld-CMake/CMakeLists.txt`](Docs/Tutorials/HelloWorld/Software/Apps/HelloWorld-CMake/CMakeLists.txt:1).

2. **Entry point**
   - `Service::run()` in [`Service.cpp`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:20) is the main loop called by the kernel.
   - The loop blocks on `mKernel.comm.getMessage()` and switches on `SDK::MessageType` values (GUI notifications, sensor data, app stop).

3. **GUI lifecycle**
   - `COMMAND_APP_NOTIF_GUI_RUN` calls `onStartGUI()` in [`Service::onStartGUI()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:135), setting `mGUIStarted = true` and sending initial values.
   - `COMMAND_APP_NOTIF_GUI_STOP` calls `onStopGUI()` in [`Service::onStopGUI()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:142), stopping logging.
   - GUI receives updates via custom messages handled in [`Model::customMessageHandler()`](Docs/Tutorials/HelloWorld/Software/Apps/TouchGFX-GUI/gui/src/model/Model.cpp:104).

4. **Sensor connection**
   - The service owns `SDK::Sensor::Connection mSensorHR` for heart rate (see [`Service.hpp`](Docs/Tutorials/HelloWorld/Software/Libs/Header/Service.hpp:25)).
   - `connect()` is called on service start; `disconnect()` only on `COMMAND_APP_STOP` (see [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:24) and final disconnect).

5. **Handle incoming sensor messages**
   - `EVENT_SENSOR_LAYER_DATA` forwards sensor batches into `Service::onSdlNewData()` (see [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:66)).
   - `onSdlNewData()` validates the handle matches HR sensor, parses data if GUI is active, and sends updates to GUI (see [`Service::onSdlNewData()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:148)).

6. **Update heart rate state**
   - Valid HR data updates `mHR` and `mHRTL` (see [`Service::onSdlNewData()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:148)).
   - `mSender.updateHeartRate()` sends values to GUI via custom message (see [`Service::onSdlNewData()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:157)).

7. **Emit samples and refresh the UI**
   - While `mGUIStarted`, every second a FIT record is added with current HR (see [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:80)).
   - GUI updates occur via custom messages, triggering view refresh (see [`Model::customMessageHandler()`](Docs/Tutorials/HelloWorld/Software/Apps/TouchGFX-GUI/gui/src/model/Model.cpp:104)).

8. **Persist FIT data on exit**
   - On `COMMAND_APP_STOP`, lap and track data are written to FIT (see [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:110)).
   - `ActivityWriter::addLap()` and `ActivityWriter::stop()` finalize the file (see [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:111) and [`Service::run()`](Docs/Tutorials/HelloWorld/Software/Libs/Sources/Service.cpp:128)).

9. **Logs behavior while iterating**
   - Use log output from `Service.cpp` and `Model.cpp` (`LOG_INFO`/`LOG_DEBUG`) to verify message sequencing and sensor data flow.
   - Confirm FIT file creation and record writing by checking filesystem after app exit.
   - Verify GUI updates by observing screen changes with simulated or real sensor data.

## Common vs App-Specific Components

### Common services and patterns (shared across apps)

- Kernel integration via `SDK::Kernel` and `mKernel.comm` message loop.
- TouchGFX GUI framework with `Model`, `Presenter`, `View` pattern.
- Sensor subscription via `SDK::Sensor::Connection` and `SDK::Interface::ISensorDataListener`.
- File access via `SDK::Interface::IFileSystem` and `SDK::Interface::IFile`.
- FIT helper/definitions via `SDK::Component::FitHelper`.
- Custom messaging via `SDK::MessageBase` and message types.

### HelloWorld-specific services and logic

- Simple HR monitoring without complex calculations or multiple sensors.
- FIT logging gated by GUI activity state.
- Custom message `HR_VALUES` for service-to-GUI communication.
- Session-based FIT file (one per app run) with lap and track summaries.

## App ↔ Kernel Interaction / Execution Path

1. **Kernel starts service** → `Service::run()` → sensor connected, ActivityWriter initialized.
2. **Kernel notifies GUI start** → `COMMAND_APP_NOTIF_GUI_RUN` → `onStartGUI()` → FIT logging begins, initial HR sent to GUI.
3. **Kernel delivers sensor batches** → `EVENT_SENSOR_LAYER_DATA` → `onSdlNewData()` → HR parsed, sent to GUI.
4. **Kernel ticks GUI** → TouchGFX tick → `Model::tick()` → screen updates if invalidated.
5. **Kernel notifies GUI stop** → `COMMAND_APP_NOTIF_GUI_STOP` → `onStopGUI()` → FIT logging stops.
6. **Kernel stops app** → `COMMAND_APP_STOP` → finalize FIT with lap/track, disconnect sensor, exit.

## Key Interfaces and Data Structures

- `SDK::Kernel` → kernel handle for messaging (`comm`) and filesystem (`fs`).
- `SDK::Interface::ISensorDataListener` → callback interface implemented by `Service` (`onSdlNewData`).
- `SDK::Sensor::Connection` → connects to HR sensor driver and matches incoming handles.
- `SDK::SensorDataParser::HeartRate` → typed decoding of HR sensor frames.
- `SDK::MessageBase` and custom `CustomMessage::HRValues` → inter-thread communication.
- `ActivityWriter` → FIT file management and serialization.
- `SDK::Component::FitHelper` → FIT message definitions and writing.

## Behavior Details

### Logging cadence

- FIT records are written every second while the GUI is active, capturing current HR and trust level.

### Persistence and recovery

- FIT file created per app session with filename based on activity type ("Activity").
- No recovery from previous sessions; each run starts fresh.

### Gating rules

- HR sensor data is only processed and logged when GUI is running.
- FIT file is finalized on app exit with session statistics.