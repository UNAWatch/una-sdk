# GlanceStrain Architecture

## Problem Statement

Provide a glance-friendly strain summary that logs heart-rate-derived strain whenever sensor data arrives and the watch is on-hand. Logging must be resilient to off-hand transitions and recover some daily activity from persisted FIT data.

## Program Architecture

### 1) Kernel entry and service lifetime

- `Service` is constructed with a reference to the kernel (`SDK::Kernel`) and runs as the glance service entry point.
- `Service::run()` is the main loop. It blocks on `mKernel.comm.getMessage()` and reacts to glance/tick/sensor messages. `EVENT_GLANCE_TICK` / `onGlanceTick()` only occur while the glance is active, and are used only for UI refresh.
- The service does **not** exit on `EVENT_GLANCE_STOP`; it keeps sensors connected and continues acquiring data in the background. Persistence is driven by sensor events, and background saves are allowed when the device is on-hand. The service exits only on `COMMAND_APP_STOP`.

### 2) Kernel message loop

`Service::run()` handles the following message types in order:

- `EVENT_GLANCE_START`
  - Configure glance UI bounds (`configGui()`), build controls (`createGuiControls()`), mark `mGlanceActive = true`.
  - Connect to sensors (`connect()`), handle day rollover (`checkDayRollover()`), and trigger an initial FIT save (`saveFit(true, false)`).
- `EVENT_GLANCE_TICK`
  - Refresh display (`onGlanceTick()`). These ticks only occur while the glance is active; they do not fire in the background.
- `EVENT_SENSOR_LAYER_DATA`
  - Dispatch batched sensor samples into `onSdlNewData()`.
- `EVENT_GLANCE_STOP`
  - Mark the glance inactive and keep sensors connected; sensor-driven saves can continue when on-hand.
- `COMMAND_APP_STOP`
  - Finalize the FIT session for the day, disconnect, and return from `run()`.

### 3) UI layer (Glance)

- `SDK::Glance::Form` (`mGlanceUI`) owns controls.
- `createGuiControls()` builds the UI layout:
  - Icon image
  - Title text ("Strain Score")
  - Value text for current strain score
- `onGlanceTick()` updates the value text and issues a `RequestGlanceUpdate` when the UI is invalid; it only runs while the glance is active.

### 4) Sensor layer

- `SDK::Sensor::Connection` instances subscribe to:
  - `HEART_RATE` → `SensorDataParserHeartRate`
  - `ACTIVITY` → `SensorDataParserActivity`
  - `TOUCH_DETECT` → `SensorDataParserTouch`
- All three sensor connections are configured with the same sample period (`skSamplePeriodSec`, expressed in ms).
- `onSdlNewData()` routes incoming batches by sensor handle:
  - Touch updates `mIsOnHand` and triggers `saveFit(true, false)` on off-hand transitions.
  - Activity updates active minutes (`mActiveMin`).
  - Heart rate updates strain accumulators and the most recent valid HR value.
- Sensor events enforce record cadence via the activity sensor connection interval, day rollover checks, and time-gated FIT saves (including in the background when on-hand).

### 5) Core strain logic

- Samples are accepted only when HR is in the valid range [50, 220].
- Each HR sample contributes a normalized delta:
  - `norm = (hr - 60) / 120`
  - `delta = max(0, norm) * 0.75`
- Running aggregates:
  - `mTotalStrain`, `mSumHR`, `mMaxHR`, `mSampleCount`, `mLastHr`
- On each activity sensor event, if the watch is on-hand, a pending FIT record is captured at the activity sensor cadence (5 seconds). This is independent of glance activity.

### 6) Persistence (FIT)

- A daily FIT file is created with name `strain_YYYY-MM-DD.fit` in the app filesystem.
- `saveFit(force, finalizeDay)` appends pending records, and optionally adds session summary if `finalizeDay` is true.
- FIT file structure is built using `SDK::Component::FitHelper` definitions for file ID, developer data, events, records, sessions, and activity summaries.
- The file header is rewritten after appends and CRC is recomputed on each save.

## Implementation Walktrought

1. **App layout**
   - Service entry point and logic live in [`Software/Libs/Header/Service.hpp`] and [`Software/Libs/Source/Service.cpp`].
   - App build wiring is under [`Software/App/GlanceStrain-CMake/CMakeLists.txt`].
   - Glance assets are compiled from headers like [`Software/Libs/Header/icon_60x60.h`] and the PNGs under [`Resources/`].

2. **Entry point**
   - `Service::run()` in [`Service.cpp`] is the main loop and the only entry point called by the kernel.
   - The loop blocks on `mKernel.comm.getMessage()` and switches on `SDK::MessageType` values (glance start/stop, tick, sensor data, app stop).

3. **UI**
   - `EVENT_GLANCE_START` calls `configGui()` to fetch sizing via `SDK::Message::RequestGlanceConfig` and initializes [`mGlanceUI`].
   - `createGuiControls()` builds the icon, title, and value text controls and stores them in `mGlanceUI` (see [`Service::createGuiControls()`]).
   - `onGlanceTick()` updates `mGlanceValue` and dispatches `RequestGlanceUpdate` when the form is invalid; it only runs while the glance is active (see [`Service::onGlanceTick()`]).

4. **Sensor connections**
   - The service owns `SDK::Sensor::Connection` members for heart rate, activity, and touch (see [`mSensorHR`], [`mSensorActivity`], and [`mSensorTouch`]).
   - `connect()` is called on glance start; `disconnect()` is only called on `COMMAND_APP_STOP` (see [`Service::connect()`] and [`Service::disconnect()`]).

5. **Handle incoming sensor messages**
   - `EVENT_SENSOR_LAYER_DATA` forwards sensor batches into `Service::onSdlNewData()` (see [`Service::run()`]).
   - `onSdlNewData()` uses `SDK::Sensor::DataBatch` and the parser types `SensorDataParser::Touch`, `::Activity`, and `::HeartRate` to validate and decode frames (see [`Service::onSdlNewData()`]).
   - Touch updates `mIsOnHand` and forces an immediate FIT save when the watch is removed (off-hand transition).
   - Activity updates `mActiveMin` from the parsed duration.

6. **Apply the strain calculation and accumulate state**
   - Heart-rate samples are accepted only in [50, 220]. For each valid sample, `norm = (hr - 60) / 120` and `delta = max(0, norm) * 0.75` (see [`Service::onSdlNewData()`]).
   - Running totals are stored in `mTotalStrain`, `mSumHR`, `mMaxHR`, `mSampleCount`, and `mLastHr` (see [`Service.hpp`]).

7. **Emit samples and refresh the UI on ticks**
- `onSdlNewData()` appends pending records to `mPendingRecords` on each activity sensor event, which runs at the `skSamplePeriodSec` cadence (5 seconds), whenever the watch is on-hand, regardless of glance activity (see [`Service::onSdlNewData()`]).
   - `onGlanceTick()` only updates the UI and emits `RequestGlanceUpdate` when invalid (see [`Service::onGlanceTick()`]).
   - `saveFit(false, false)` is invoked from sensor events to persist at most once per `skSaveIntervalSec` (3600 seconds), and can run in the background when on-hand.

8. **Persist FIT data and handle day rollover**
   - `checkDayRollover()` updates `mCurrentDate`, rebuilds `mFitPath` as `strain_YYYY-MM-DD.fit`, and resets accumulators when the date changes (see [`Service::checkDayRollover()`]).
   - `saveFit(force, finalizeDay)` opens or creates the FIT file, writes definitions when needed, appends pending records, and optionally emits a session summary (see [`Service::saveFit()`]).
   - FIT message/field helpers are initialized in the constructor (`mFitFileID`, `mFitRecord`, `mFitSession`, `mFitStrainField`, `mFitActiveField`) and used by `writeFitDefinitions()`, `appendPendingRecords()`, and `writeFitSessionSummary()` (see [`Service::Service()`] and helper methods nearby).
   - `saveFit()` is triggered by sensor events and can run in the background when the watch is on-hand; it is still time-gated by `skSaveIntervalSec`.

9. **Logs behavior while iterating**
   - Use log output from `Service.cpp` (`LOG_INFO`/`LOG_DEBUG`) to verify event sequencing, day rollover, and save cadence (see [`Service::run()`] and [`Service::checkDayRollover()`]).
   - Confirm that off-hand transitions force an immediate save by simulating `TOUCH_DETECT` events and watching for `saveFit(true, false)` calls in logs (see [`Service::onSdlNewData()`]).
   - Verify the FIT output location and daily file naming using `mFitPath` after a glance start/rollover (see [`Service::checkDayRollover()`]).

## Common vs App-Specific Components

### Common services and patterns (shared across glance apps)

- Kernel integration via `SDK::Kernel` and `mKernel.comm` message loop.
- Glance UI primitives: `SDK::Glance::Form`, `ControlText`, `RequestGlanceConfig`, `RequestGlanceUpdate`.
- Sensor subscription via `SDK::Sensor::Connection` and `SDK::Interface::ISensorDataListener`.
- File access via `SDK::Interface::IFileSystem` and `SDK::Interface::IFile`.
- FIT helper/definitions via `SDK::Component::FitHelper`.

### GlanceStrain-specific services and logic

- Strain scoring formula and gating by valid HR range.
- Off-hand detection behavior (touch sensor drives immediate save).
- Daily file naming and per-day session summary emission.
- Developer fields for FIT (`strain` and `active_min`) via `mFitStrainField` and `mFitActiveField`.

## App ↔ Kernel Interaction / Execution Path

1. **Kernel starts glance** → `EVENT_GLANCE_START` → UI configured and rendered, sensors connected, daily FIT context initialized.
2. **Kernel delivers sensor batches** → `EVENT_SENSOR_LAYER_DATA` → `onSdlNewData()` updates HR/active minutes/on-hand state.
3. **Kernel ticks glance** → `EVENT_GLANCE_TICK` → `onGlanceTick()` updates the UI only. This only happens while the glance is active.
4. **Kernel stops glance** → `EVENT_GLANCE_STOP` → mark glance inactive; sensors stay connected and background acquisition continues while sensor-driven persistence can continue when on-hand.
5. **No glance ticks while inactive** → no UI refresh occurs, but sensor events continue to drive record emission, rollover checks, and saves when on-hand.
6. **Kernel stops app** → `COMMAND_APP_STOP` → finalize session, disconnect, exit.

## Key Interfaces and Data Structures

- `SDK::Kernel` → kernel handle for messaging (`comm`) and filesystem (`fs`).
- `SDK::Interface::ISensorDataListener` → callback interface implemented by `Service` (`onSdlNewData`).
- `SDK::Sensor::Connection` → connects to HR/activity/touch drivers and matches incoming handles.
- `SDK::SensorDataParser::*` → typed decoding of incoming sensor frames.
- `SDK::Glance::Form` and `SDK::Glance::ControlText` → glance UI layout and updates.
- `SDK::Interface::IFile` / `IFileSystem` → FIT persistence.
- `SDK::Component::FitHelper` → FIT message definitions and writing.

## Behavior Details

### Logging cadence

- Record strain samples on a 5-second cadence while the watch is on-hand; cadence is driven by the activity sensor connection interval, not glance ticks.

### Persistence and recovery

- Persist to a daily FIT file (one file per day).
- Recover state from the daily FIT file on glance start and on day rollover so totals continue from the last saved data.

### Gating rules

- If `TOUCH_DETECT` reports unworn, treat the state as off-hand, suppress logging, and force an immediate save.
- Allow disk I/O in the background when the watch is on-hand; sensor events trigger time-gated saves.
- Save on glance start and on sensor events, but no more often than every 60 minutes unless an off-hand transition triggers an immediate save.
