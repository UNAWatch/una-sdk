# Alarm - Alarm Clock

## Overview

The Alarm app is a **Utility**-type application designed for wearable devices. It allows the user to create, edit, and delete alarms with configurable time, repeat schedule, and alert effect. When an alarm fires, the app wakes the screen, plays the configured buzzer or vibration pattern, and presents a ringing screen from which the user can stop or snooze the alarm.

The app does not use any sensors. All logic is time-based: the service runs continuously in the background, checking the current local time against the alarm list once per minute and triggering the corresponding effect when a match is found.

Key features include:
- Up to 20 alarms stored in persistent JSON storage
- Per-alarm configuration: time (HH:MM), repeat schedule, and alert effect
- Repeat options: once, every day, weekdays, weekends, or a specific day of the week
- Alert effects: buzzer only, vibration only, or buzzer + vibration
- Snooze: re-triggers after 5 minutes, up to 5 times
- The app can be launched on-demand (user opens from menu) or automatically when an alarm fires
- If no active alarms exist and the GUI never starts within 5 seconds, the service exits without launching the display

## Architecture

The app is structured as two independent components: a service that handles alarm persistence and time-based triggering, and a GUI that manages the alarm list and user interaction. Communication between them occurs through a message-based system using the UNA SDK kernel infrastructure.

### High-Level Components

1. **Service Layer**: Alarm persistence, time-based checking, effect playback
2. **GUI Layer**: TouchGFX-based user interface, alarm CRUD operations
3. **SDK Integration**: Kernel, file system, messaging, backlight, buzzer, vibration
4. **Data Persistence**: JSON file for the alarm list

### Component Interaction

```
[System Clock] ──► [AlarmManager] ──► [Service]
                                           ^
                                           |
                    [Message System] ──► [GUI]
```

The service runs as a separate thread, checking the alarm list each minute and dispatching effects through the kernel. The GUI runs on the TouchGFX framework, handling user input and displaying the alarm list and ringing screen.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`. It owns an `AlarmManager` instance and acts as its observer, forwarding alarm events to the GUI through the messaging system.

### Core Classes and Structures

#### Service Class

```cpp
class Service : public AlarmManager::AlarmCallback
{
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service();
    void run();

private:
    SDK::Kernel& mKernel;
    bool         mGuiStarted;
    AlarmManager mAlarmManager;
    Alarm        mActiveAlarm;
    // ...
};
```

The service implements `AlarmManager::AlarmCallback` to receive two events:
- `onAlarm(alarm)` — called when a scheduled alarm fires
- `onListChanged(list)` — called whenever the alarm list changes (load or save)

#### Alarm Data Structure

`Alarm` is a plain struct shared between GUI and Service (defined in `Libs/Header/Alarm.hpp`):

```cpp
struct Alarm {
    bool    on;           // Whether the alarm is enabled
    uint8_t timeHours;    // Hour (0–23)
    uint8_t timeMinutes;  // Minute (0–59)
    Repeat  repeat;       // Repetition schedule
    Effect  effect;       // Alert effect
};
```

**`Alarm::Repeat`** (11 options):

| Value | JSON key | Meaning |
|-------|----------|---------|
| `REPEAT_NO` | `"no"` | Fires once, then disabled automatically |
| `REPEAT_EVERY_DAY` | `"every_day"` | Every day |
| `REPEAT_WEEK_DAYS` | `"week_days"` | Monday – Friday |
| `REPEAT_WEEKENDS` | `"weekends"` | Saturday and Sunday |
| `REPEAT_MONDAY` … `REPEAT_SUNDAY` | `"monday"` … `"sunday"` | Specific day of the week |

**`Alarm::Effect`** (3 options):

| Value | JSON key | Behavior |
|-------|----------|---------|
| `EFFECT_BEEP_AND_VIBRO` | `"beep_vibro"` | Buzzer + vibration |
| `EFFECT_VIBRO` | `"vibro"` | Vibration only |
| `EFFECT_BEEP` | `"beep"` | Buzzer only |

**Identity**: Two `Alarm` objects are considered equal (for snooze tracking and duplicate detection) if `timeHours`, `timeMinutes`, and `repeat` match. The `on` flag and `effect` are excluded from the comparison.

#### AlarmManager Class

`AlarmManager` handles all alarm logic independently of the service loop. It is responsible for:
- Loading and saving the alarm list from a JSON file (`alarms.json`)
- Checking once per minute whether any enabled alarm is due
- Tracking snoozed alarms and re-triggering them after a 5-minute interval
- Notifying an observer (`AlarmCallback`) on each alarm fire and on list changes

```cpp
class AlarmManager {
public:
    class AlarmCallback {
    public:
        virtual void onAlarm(const Alarm& alarm) {}
        virtual void onListChanged(const std::vector<Alarm>& list) {}
    };

    void     load();
    void     attachCallback(AlarmCallback* pCallback);
    uint32_t execute(const std::tm& tmNow);
    bool     saveAlarmList(const std::vector<Alarm>& list);
    void     disableAlarm(const Alarm& alarm);
    void     disableAllActiveAlarm();
    void     snoozeAlarm(const Alarm& alarm);
    void     snoozeAllActiveAlarm();
    bool     hasActiveAlarms() const;
};
```

**Constants**:

| Constant | Value | Meaning |
|----------|-------|---------|
| `kSnoozedTimeMinutes` | 5 | Minutes between snooze re-triggers |
| `kMaxSnoozeCount` | 5 | Maximum snooze re-trigger count per alarm |
| `kInitialCount` | 20 | Initial vector capacity (also the maximum for message transfer) |

### Alarm Checking Logic

`AlarmManager::execute()` is called once per iteration of the service loop. It delegates to `checkAlarms()` and returns the milliseconds until the next full minute:

```cpp
uint32_t AlarmManager::execute(const std::tm& tmNow)
{
    checkAlarms(static_cast<uint8_t>(tmNow.tm_hour),
                static_cast<uint8_t>(tmNow.tm_min),
                static_cast<uint8_t>(tmNow.tm_wday),
                tmNow);
    uint32_t nextCheckMs = (60 - tmNow.tm_sec) * 1000;
    return nextCheckMs;
}
```

`checkAlarms()` performs two passes per call:

**Pass 1 — regular alarms**: For each enabled alarm that matches the current hour, minute, and day schedule, and has not been snoozed yet:
1. Calls `mObserver->onAlarm(alarm)`
2. Adds the alarm to the snoozed-alarm tracking list (`addSnoozedAlarm`)
3. For one-time alarms (`REPEAT_NO`): sets `alarm.on = false` and saves to file immediately

**Pass 2 — snoozed alarms**: Checks each entry in `mSnoozedAlarms`. When `nextTriggerHour:nextTriggerMinute` matches the current time:
- Decrements `snoozeCount`
- If `snoozeCount > 0`: re-triggers `onAlarm` and advances the next trigger time by `kSnoozedTimeMinutes`
- If `snoozeCount == 0`: removes the entry (snooze exhausted)

**Snooze design note**: `snoozeAlarm()` and `snoozeAllActiveAlarm()` are intentional no-ops. The alarm is already inserted into `mSnoozedAlarms` when it first fires in `checkAlarms`. Re-triggering after the snooze interval is handled automatically by `execute()`. The GUI only needs to call these methods to communicate intent; no additional action is required on the service side.

### Alarm Persistence

Alarms are stored in `alarms.json` in the application's file system. The JSON schema is:

```json
{
  "alarms": [
    { "on": true,  "time_h": 7, "time_m": 30, "repeat": "week_days", "effect": "beep_vibro" },
    { "on": false, "time_h": 9, "time_m": 0,  "repeat": "weekends",  "effect": "vibro"      }
  ]
}
```

The internal serialisation buffer is 2048 bytes. A worst-case file with 20 alarms (all using the longest field values: `false`, `23:59`, `"wednesday"`, `"beep_vibro"`) produces ~1612 bytes of compact JSON, which fits comfortably within this limit. If the file exceeds the buffer, loading fails gracefully with an error log.

### Service Lifecycle and App Startup

The Alarm service is configured with `APP_AUTOSTART On`, which instructs the kernel to launch the service automatically at boot. This is essential for time-based alarm detection: the service must be running in the background at all times so it can check the alarm list each minute, even when the GUI is not open.

The service's `run()` method handles two startup scenarios:

**Scenario A — User opens app from menu** (GUI already starting):
1. Service starts, loads alarms from `alarms.json`
2. Receives `COMMAND_APP_NOTIF_GUI_RUN` → calls `onStartGUI()`:
   - Sets `mGuiStarted = true`
   - If a pending active alarm exists (`mActiveAlarm.on`), sends it to GUI via `ACTIVATED_ALARM`
   - Sends the full alarm list to GUI via `ALARM_LIST`
3. Enters the minute-loop, checking for alarms and processing GUI messages

**Scenario B — Alarm fires automatically** (GUI not yet running):
1. Service starts, loads alarms
2. First `execute()` call detects a due alarm → `onAlarm(alarm)`:
   - Sends `RequestAppRunGui` to kernel (kernel launches GUI)
   - Saves `mActiveAlarm = alarm`
3. Kernel starts GUI; `COMMAND_APP_NOTIF_GUI_RUN` arrives → `onStartGUI()` sends `ACTIVATED_ALARM` + `ALARM_LIST` to GUI
4. GUI navigates directly to `RingingView`

**Early exit**: If `mGuiStarted` is still `false` after 5 seconds and `hasActiveAlarms()` returns `false`, the service exits without displaying anything.

### Effect Playback

When the GUI sends `ACTIVATED_EFFECT`, the service plays the configured pattern:

**Backlight**: always on (brightness 100%, auto-off after 4000 ms).

**Vibration pattern** (for `EFFECT_VIBRO` and `EFFECT_BEEP_AND_VIBRO`):
```
ALERT_750MS_100 — 250ms pause — ALERT_750MS_100 — 250ms pause — ALERT_750MS_100
```

**Buzzer pattern** (for `EFFECT_BEEP` and `EFFECT_BEEP_AND_VIBRO`):
```
750ms@100% — 250ms@0% — 750ms@100% — 250ms@0% — 750ms@100%
```

Stopping the effect (`stopRinging()`) sends empty `RequestBuzzerPlay` and `RequestVibroPlay` messages, which cancels any active pattern.

## GUI

The GUI is built using the TouchGFX framework and follows the Model-View-Presenter pattern.

### Project Structure

```
Alarm/Software/Apps/TouchGFX-GUI/
├── AlarmGUI.touchgfx     # TouchGFX Designer project
├── application.config    # Application configuration
├── target.config         # Target hardware settings
├── touchgfx.cmake        # CMake integration
├── gui/                  # Custom GUI code
├── assets/               # Images, fonts, texts
├── generated/            # Auto-generated TouchGFX code
└── simulator/            # Simulator builds
```

### Model-View-Presenter Pattern

- **Model**: `Model.hpp/cpp` — Alarm state, service communication, lifecycle
- **View**: Screen view classes — UI rendering and input
- **Presenter**: Screen presenter classes — Logic binding model and view

### Model

The Model class serves as the central data hub and gateway to the service:

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    void tick();
    void handleKeyEvent(uint8_t key);
    void resetIdleTimer();
    void exitApp();
    void switchToNextPriorityScreen();

    // Alarm access
    const Alarm&        getActiveAlarm() const;
    void                playAlarm();
    void                stopAlarm();
    void                snoozeAlarm();
    std::vector<Alarm>& getAlarmList();
    void                setAlarmEditId(size_t id);
    size_t              getAlarmEditId();
    void                saveAlarm(size_t id, Alarm alarm);
    void                deleteAlarm(size_t id);
};
```

**Key Model behaviours**:

- **`switchToNextPriorityScreen()`**: determines where to navigate after completing an action. Priority order: (1) alarm ringing → `RingingView`; (2) `mStayInApp` flag set → `MainView`; (3) otherwise → `exitApp()`. `mStayInApp` is set to `true` when the user presses any key while no alarm is ringing, ensuring the app stays visible after the user completes an edit, save, or delete flow.

- **`saveAlarm(id, alarm)`**: If `id < list.size()`, overwrites in-place. If `id == list.size()` (new alarm), first checks for duplicates by identity (time + repeat); if a match is found, overwrites it instead of appending. Otherwise appends.

- **`setCapabilities()`**: Called in the constructor. Sends `RequestSetCapabilities` to the kernel with `enMusicControl = true` and `enUsbChargingScreen = true`, enabling the system to overlay music controls and USB charging status while this app is active.

- **Idle timer**: Counts down at the frame rate; fires `modelListener->onIdleTimeout()` after `App::Config::kScreenTimeoutSteps` (30 seconds). Resets on any button press.

### Screens (6 total)

| Screen | Class | Purpose |
|--------|-------|---------|
| Main | `MainView` | Alarm list with scroll-wheel selection |
| Menu | `MenuView` | Action menu for a selected alarm (Toggle / Edit / Delete) |
| Edit | `EditView` | Time and options editor (time wheel + option wheels) |
| Saved | `SavedView` | Confirmation after save (auto-dismisses → next priority screen) |
| Deleted | `DeletedView` | Confirmation after delete (auto-dismisses → next priority screen) |
| Ringing | `RingingView` | Active alarm: shows alarm time; buttons to Stop or Snooze |

### Message Handling System

The Model implements `ICustomMessageHandler` to receive asynchronous messages from the service:

```cpp
bool Model::customMessageHandler(SDK::MessageBase* msg)
{
    switch (msg->getType()) {
        case CustomMessage::ALARM_LIST: {
            auto* m = static_cast<CustomMessage::AlarmList*>(msg);
            mAlarmList.assign(m->alarms, m->alarms + m->count);
            mTimeFormat12h = m->timeFormat12h;
            modelListener->onAlarmListUpdated(mAlarmList);
        } break;

        case CustomMessage::ACTIVATED_ALARM: {
            auto* m  = static_cast<CustomMessage::ActivatedAlarm*>(msg);
            mActiveAlarm = m->alarm;
            modelListener->onAlarmActivated(mActiveAlarm);
        } break;

        default: break;
    }
    return true;
}
```

`ModelListener::onAlarmActivated` has a default implementation that unconditionally navigates to `RingingView` — no presenter needs to override it. The active alarm data is available via `model->getActiveAlarm()`.

### Screen Navigation

All screen changes use `gotoXxxScreenNoTransition()` — no animated transitions. Animations exist only within containers: `TimeWheel` and `OptionWheel` scroll movement.

**Screen Flow**:
```
MainView ──[R1: open menu]──► MenuView
    │                             │           │           │
    │                         [Toggle]      [Edit]     [Delete]
    │                             │           │           │
    │                         MainView    EditView    DeletedView
    │                                         │       (auto → switchToNextPriorityScreen)
    │                                   [R1: save]
    │                                     SavedView
    │                                  (auto → switchToNextPriorityScreen)
    │
    └──[R2: exit]──► exitApp()

RingingView ──[R1: Stop]──►   stopAlarm()   → switchToNextPriorityScreen()
             ──[R2: Snooze]──► snoozeAlarm() → switchToNextPriorityScreen()

switchToNextPriorityScreen():
    alarm ringing  ──► RingingView
    mStayInApp     ──► MainView
    else           ──► exitApp()
```

When an alarm fires, `ModelListener::onAlarmActivated` navigates from any active screen directly to `RingingView`. If the user is currently in `EditView`, `RingingPresenter::deactivate()` calls `model->stopAlarm()` to ensure the effect is cleaned up if the user dismisses without pressing Stop.

### Custom Containers

The app uses two categories of containers: ready-made **SDK widgets** from `Templates/TouchGFX-Widgets` (imported as `.tpkg` packages) and **app-specific containers** built for alarm editing.

#### SDK Widget Containers

Imported from `Templates/TouchGFX-Widgets` and used without modification, plus
header-only widgets included straight from the SDK:

- `Buttons` — button arc indicators (L1/L2/R1/R2)
- `Title` — screen title with underline
- `Toggle` — on/off toggle switch
- `SDK::GUI::CountdownTimer` — self-registering countdown that fires a callback at
  zero. Header-only, from `SDK/GUI/CountdownTimer.hpp`; not a `.tpkg`.

#### App-Specific Containers

Built specifically for this app and found in `gui/src/containers/`:

**`TimeWheel`** — dual scroll-wheel for picking hours (0–23) and minutes (0–59). Composed of two independent scroll columns with a highlighted center item (`TimeWheelHoursCenterItem`, `TimeWheelMinutesCenterItem`) and adjacent items (`TimeWheelHoursItem`, `TimeWheelMinutesItem`).

**`OptionWheel`** — generic vertical scroll-wheel for selecting one value from a list. Used for `Repeat` (11 items) and `Effect` (3 items) in `EditView`. Center item rendered by `OptionWheelCenterItem`; adjacent items by `OptionWheelItem`.

### Input Handling

**Button Mapping**:
- **L1**: Scroll up / previous item
- **L2**: Scroll down / next item
- **R1**: Primary action (confirm / open menu / save)
- **R2**: Secondary action (exit / snooze)

Any button press resets the idle timer. If no alarm is ringing at the time of the press, `mStayInApp` is set to `true` so the app does not exit after the current flow completes.

### Idle Timeout

The app implements automatic timeout to conserve battery:
- Timeout: `App::Config::kScreenTimeoutSteps` = 30 seconds at the frame rate
- On timeout, the active presenter's `onIdleTimeout()` is called, which navigates via `switchToNextPriorityScreen()`
- Timer resets on any button press (`handleKeyEvent`)

### TouchGFX Integration with UNA SDK

#### TouchGFXCommandProcessor

`SDK::TouchGFXCommandProcessor` is a singleton that bridges the SDK kernel and the TouchGFX event loop. Custom messages received during `waitForFrameTick()` (called from `OSWrappers::waitForVSync()`) are placed in an internal queue. `FrontendApplication::handleTickEvent()` calls `callCustomMessageHandler()` first, then `model.tick()`, ensuring pending service messages are dispatched to presenters before the view updates.

#### Kernel Provider Architecture

`SDK::KernelProviderGUI` is a singleton that stores a pointer to the `SDK::Kernel` object. It is initialised before the GUI starts and retrieved by the `Model` constructor:

```cpp
Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);
    setCapabilities();
}
```

## Custom Message System

All service↔GUI messages are declared in `Libs/Header/Commands.hpp` as `constexpr SDK::MessageType::Type` constants. Every message struct inherits from `SDK::MessageBase`. All structs are wrapped in `#pragma pack(push, 4)` for alignment consistency with the message pool.

```cpp
namespace CustomMessage {

static constexpr size_t kMaxAlarms = 20;  // must match AlarmManager::kInitialCount

// Service <-> GUI
constexpr SDK::MessageType::Type ALARM_LIST       = 0x00000001;

// Service --> GUI
constexpr SDK::MessageType::Type ACTIVATED_ALARM  = 0x00000002;

// GUI --> Service
constexpr SDK::MessageType::Type ACTIVATED_EFFECT = 0x00000003;
constexpr SDK::MessageType::Type ALARM_STOP       = 0x00000004;
constexpr SDK::MessageType::Type ALARM_STOP_ALL   = 0x00000005;
constexpr SDK::MessageType::Type ALARM_SNOOZE     = 0x00000006;
constexpr SDK::MessageType::Type ALARM_SNOOZE_ALL = 0x00000007;

} // namespace CustomMessage
```

**`AlarmList`** transfers the entire alarm list as a fixed-size C-array to avoid heap allocation in the message pool path:

```cpp
struct AlarmList : public SDK::MessageBase {
    Alarm   alarms[kMaxAlarms];
    uint8_t count;
    bool    timeFormat12h;  // Service -> GUI only; the GUI leaves it at the default
    // sizeof = 32 (MessageBase) + 20*sizeof(Alarm) + 2 = 134 bytes → Pool 3 (256 bytes)
};
```

**Message summary**:

| Message | Direction | Trigger |
|---------|-----------|---------|
| `ALARM_LIST` | Service → GUI | On GUI start; after any list change on service side |
| `ALARM_LIST` | GUI → Service | User saves or deletes an alarm |
| `ACTIVATED_ALARM` | Service → GUI | Alarm fires |
| `ACTIVATED_EFFECT` | GUI → Service | `RingingPresenter::activate()` → `model->playAlarm()` |
| `ALARM_STOP` / `ALARM_STOP_ALL` | GUI → Service | User taps Stop on `RingingView` |
| `ALARM_SNOOZE` / `ALARM_SNOOZE_ALL` | GUI → Service | User taps Snooze on `RingingView` |

### Sending Messages

Each message struct keeps a default constructor (which sets its message type) plus, where it has fields, a field-filling constructor that delegates to it:

```cpp
struct ActivatedAlarm : public SDK::MessageBase {
    Alarm alarm;

    ActivatedAlarm()
        : SDK::MessageBase(ACTIVATED_ALARM)
        , alarm{}
    {}

    explicit ActivatedAlarm(const Alarm &alarm)
        : ActivatedAlarm()
    {
        this->alarm = alarm;
    }
};
```

`AlarmList` is the one worth reading closely, because the awkward part of the conversion lives in
its constructor rather than at any call site. It takes a `std::vector<Alarm>` but stores a fixed
array, so it clamps the count and copies element-wise — the caller just hands over the vector:

```cpp
explicit AlarmList(const std::vector<Alarm> &list, bool timeFormat12h = false)
    : AlarmList()
{
    this->count = static_cast<uint8_t>(
        list.size() < kMaxAlarms ? list.size() : kMaxAlarms);
    for (uint8_t i = 0; i < this->count; ++i) {
        this->alarms[i] = list[i];
    }
    this->timeFormat12h = timeFormat12h;
}
```

`timeFormat12h` defaults to `false` because it is only meaningful Service → GUI; the GUI → Service
direction leaves it alone and the service ignores it there.

Sending is then one call in either direction. `SDK::send_msg<T>(kernel, args...)` allocates the message from the kernel pool, forwards `args...` to the constructor, sends it, and releases it, returning `false` if allocation or the send failed:

```cpp
// Service --> GUI
SDK::send_msg<CustomMessage::ActivatedAlarm>(mKernel, alarm);
SDK::send_msg<CustomMessage::AlarmList>(mKernel, list, mTimeFormat12h);

// GUI --> Service
SDK::send_msg<CustomMessage::AlarmActivateEffect>(mKernel, mActiveAlarm);
SDK::send_msg<CustomMessage::AlarmStopAll>(mKernel);   // zero-field message
```

There is no per-app sender class: the message type owns its field filling and `send_msg` owns the allocate/send/release sequence.

`send_msg` is for fire-and-forget sends, and it posts with a zero timeout — if the queue has no room the message is dropped rather than waited for. Reach for `SDK::make_msg<T>()` when the reply matters: it returns an RAII `MessageGuard` that releases on scope exit, so you can send with a timeout (`msg.send(100)`) and read the result back. That timeout bounds the wait for the reply, not for queue space — no send waits out a full queue. `refreshTimeFormat()` does exactly this to pick up the system clock format:

```cpp
if (auto msg = SDK::make_msg<SDK::Message::RequestSystemSettings>(mKernel)) {
    if (msg.send(100) && msg.ok()) {
        mTimeFormat12h = msg->timeFormat;
    }
}
```

## Build and Setup

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Alarm/Software/Apps/Alarm-CMake/`

```cmake
# App configuration
set(APP_NAME "Alarm")
set(APP_TYPE "Utility")
set(APP_AUTOSTART On)
set(DEV_ID "UNA")
set(APP_ID "A19C2A7E4F8B6D31")

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
    ${UNA_SDK_SOURCES_APPSYSTEM}
    ${UNA_SDK_SOURCES_JSON}
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
- UNA SDK common, GUI, app-system, and JSON sources
- Kernel and messaging systems
- File system interface

**App Libraries** (`Libs/`):
- `AlarmManager` — alarm persistence and time-based triggering
- `Service` — service entry point and alarm event handling
- `Alarm.hpp` — shared alarm data structure

### Development Setup

See [SDK Setup and Build Overview](../sdk-setup.md) for comprehensive development environment setup, build instructions, and toolchain requirements.

### Simulator Build

The TouchGFX project includes a Visual Studio simulator build located in `simulator/msvs/Application.vcxproj`.

## Conclusion

The Alarm app demonstrates a lightweight but complete UNA application that operates independently of sensor hardware. Its architecture is simpler than activity-tracking apps — no sensor connections, no FIT recording, no real-time data streams — but it showcases key SDK patterns: background service with autonomous GUI launch, bidirectional messaging for CRUD operations, priority-based screen routing, and a clean separation between the alarm engine (`AlarmManager`) and the transport layer (`Service`).

Key architectural strengths include:
- **Separation of Concerns**: `AlarmManager` handles all timing logic independently; `Service` handles only transport and lifecycle
- **Zero heap allocation in the message path**: Fixed-size `AlarmList` array fits entirely within a single pool block
- **Resilient app lifecycle**: Service can start, trigger an alarm, and launch the GUI autonomously without user interaction
- **Priority-based navigation**: `switchToNextPriorityScreen()` ensures a ringing alarm always surfaces above any other screen the user was on
