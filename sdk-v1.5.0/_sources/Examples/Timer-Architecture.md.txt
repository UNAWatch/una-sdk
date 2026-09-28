# Timer - Countdown Timer

## Overview

The Timer app is a **Utility**-type application that counts a fixed duration down to zero and raises an alert when it expires. It is a multi-screen app: the user picks or dials a duration, watches it run, and acknowledges the alert, moving between purpose-built screens for each step.

It splits into a background **service** that owns the countdown and a **GUI** that renders it. The two stay traffic-free while counting: the kernel hands both processes the same monotonic millisecond tick, so the service sends a single snapshot on each state change and the GUI extrapolates the live `MM:SS` locally against its own clock. There is no periodic "current time" message.

Around that countdown the app does four things:

- **It fires.** On expiry the service plays an alert (buzzer / vibro / backlight) and, if the GUI is closed, **launches it** to show the alert.
- **It persists.** Manually entered timers are remembered across launches in a small JSON file.
- **It surfaces on the home screen.** While the app is backgrounded and a countdown is live, the service pushes `MM:SS` and a draining progress bar to the kernel home screen through the widget IPC.
- **It sleeps on a deadline.** The service's message wait is bounded by the remaining time (and the widget's next-second boundary), so it wakes to fire and to tick the home widget, then returns to low power.

Key features include:

- Seven fixed presets (1, 3, 5, 10, 15, 30 min, 1 h) plus a custom duration entered on a sphere-wheel picker, up to `99:59`
- Start, pause, resume, reset, stop, and repeat, driven by the four hardware buttons (L1/L2/R1/R2)
- Three alert effects: beep + vibrate, vibrate only, beep only
- The fired alert re-indicates periodically and auto-acknowledges (Done) after about a minute if left untouched
- Recents: the last three manually started timers, de-duplicated newest-first and persisted as JSON
- Background counting: the service keeps timing (and drives the home widget) while the GUI is closed, and re-entering the app re-attaches to it
- The service ends itself once the GUI is gone **and** no countdown is armed **and** no fire is pending, so an idle Timer holds no resident thread

## Architecture

The app is structured as two independent components: a service that owns the countdown state and a GUI that renders it. Communication between them uses the UNA SDK kernel messaging system.

### High-Level Components

1. **Service Layer**: The single source of truth for the countdown; applies commands, fires on expiry, plays alerts, drives the home widget, and persists recents
2. **GUI Layer**: TouchGFX-based multi-screen front end; extrapolates the running time and navigates between screens by state
3. **SDK Integration**: Kernel, messaging, monotonic system clock, home-screen widget IPC, buzzer/vibro/backlight, JSON, GUI capabilities

### Component Interaction

```text
                start / control (pause/resume/reset/stop/repeat) / recents-save
        [GUI] ────────────────────────────────────────────────────────► [Service]
          ▲                                                                  │
          │   state snapshot / fired / recents                              │
          └──────────────────────────────────────────────────────────────────┘
          │                                                                  │
   [Kernel tick] ──► both processes read the same getTimeMs()                │
                                                                             ▼
                                              [buzzer / vibro / backlight] + [home widget]
```

The state snapshot is **not** sent periodically: the service publishes a full `TimerManager::State` in response to an event — a command, the GUI coming up, or a fire — and the GUI recomputes the displayed remainder every frame from the shared tick (`endTick - now` while running, the frozen remainder while paused). The outbound edge on the right is the countdown's payoff: at expiry the service drives hardware alerts and, when backgrounded, the home widget, and it launches the GUI to deliver a fire that happened while the app was closed.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`. It owns a `TimerManager` and answers every command with a fresh snapshot, while `run()` doubles as the countdown clock and the home-widget pump.

### Core Classes and Structures

#### Service Class

```cpp
class Service : public TimerManager::Callback
{
public:
    explicit Service(SDK::Kernel &kernel);
    virtual ~Service();
    void run();

private:
    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;
    TimerManager          mTimerManager;
    SDK::HomeWidget       mWidget;

    bool     mPendingFired = false;   // fire waiting for the GUI it launched
    Timer    mPendingTimer {};
    uint32_t mPendingFiredTick = 0;

    bool     mWidgetActive  = false;  // home widget currently claimed
    int32_t  mLastWidgetSec = -1;     // last MM:SS pushed (dedup)
    // ...
};
```

`run()` blocks on the message queue with a **computed timeout**:

```text
sleepTime = mTimerManager.execute(now)   // ms to expiry while RUNNING, else "no timeout"
pumpWidget(now, sleepTime)               // may shorten it to the next whole second
```

so the loop wakes exactly when it must — to fire the countdown, to tick the home widget's `MM:SS`, or when a command arrives — and otherwise lets the chip sleep. It reacts to three kernel lifecycle messages plus the app's own commands:

- `COMMAND_APP_STOP` — forced exit; silences any active alert (`stopEffect()`), drops the home widget, detaches the callback, and returns
- `COMMAND_APP_NOTIF_GUI_RUN` — the GUI is up; publishes the current state and recents, and delivers a pending background fire
- `COMMAND_APP_NOTIF_GUI_STOP` — the GUI is gone; the loop keeps running only if something still needs it (see lifecycle)
- `TIMER_START` / `TIMER_CONTROL` / `TIMER_RECENTS_SAVE` — the app's own commands

### Command Handling

`handleControl()` reads the kernel tick once, applies the matching transition to the `TimerManager`, and publishes:

| `TimerCmd` | Effect |
|------------|--------|
| `PAUSE` | Freeze the countdown, keep the remainder |
| `RESUME` | Resume from the frozen remainder |
| `RESET` | Reload the full duration (re-arms only when running; otherwise restores the time without changing state) |
| `STOP` | Silence the alert and return to `IDLE` |
| `REPEAT` | Silence the alert and re-arm the full duration, held paused |
| `REPLAY_ALERT` | Re-play the fired alert effect only; state unchanged, no snapshot |

`TIMER_START` arms a fresh countdown; `TIMER_RECENTS_SAVE` overwrites the persisted recents. Every path except `REPLAY_ALERT` ends in a snapshot to the GUI (only when the GUI is up).

### Firing and GUI launch

`TimerManager` calls back into the service when the countdown reaches zero:

```cpp
void Service::onFired(const Timer& timer)
{
    playEffect(timer.effect);                 // backlight + buzzer/vibro pattern
    if (mGuiStarted) {
        // GUI is up -> fired in-app
        SDK::send_msg<CustomMessage::TimerFired>(mKernel, timer, false);
    } else {
        // Launch the GUI, then deliver the fire once it signals it is running.
        // (RequestAppRunGui)
        mPendingFired = true; mPendingTimer = timer;
        mPendingFiredTick = mKernel.sys.getTimeMs();
    }
}
```

A fire raised while the GUI is closed launches it and parks the delivery in `mPendingFired` until the GUI signals `COMMAND_APP_NOTIF_GUI_RUN`. That wait is **bounded**: if the launch is dropped or the GUI never signals, the pending delivery expires after ten seconds so the service can go idle and exit rather than blocking forever.

### Alerts

`playEffect()` raises the backlight (auto-off) and sends a repeating buzzer and/or vibro pattern chosen by the timer's `Effect`:

| `Timer::Effect` | Buzzer | Vibro |
|-----------------|--------|-------|
| `EFFECT_BEEP_AND_VIBRO` | ✓ | ✓ |
| `EFFECT_VIBRO` | | ✓ |
| `EFFECT_BEEP` | ✓ | |

`stopEffect()` sends empty buzzer and vibro play messages, which silence both. It runs on `STOP`, on `REPEAT`, and on a forced app exit so a firing alert never outlives the app.

### Home-screen widget

While the app is **backgrounded** and a countdown is `RUNNING` or `PAUSED`, `pumpWidget()` claims the kernel's single home-widget slot and pushes the live reading:

```cpp
mWidget.update(WIDGET_SHOW_TEXT | WIDGET_SHOW_PERCENT | WIDGET_PROGRESS_FROM_END,
               percent, "MM:SS");
```

- `WIDGET_PROGRESS_FROM_END` pins the bar fill to its end, so it **drains from the start** as the count runs down.
- The push is de-duplicated on the shown second (`mLastWidgetSec`), so a paused countdown pushes once and a running one once per second — `pumpWidget()` shortens the loop's `sleepTime` to the next whole-second boundary so the reading ticks crisply.
- With the GUI open the user already sees the Running screen, so the widget is released; closing the app (countdown still going) brings it back. `STOP`/expiry release it too.

### Service Lifecycle and App Startup

The Timer service is **not** an autostart service (`APP_AUTOSTART Off`); it is launched when the user opens the app or when a background fire re-launches the GUI. The kernel does not stop a service when its GUI closes, so the service must end **itself** once it has nothing left to do:

```cpp
// Nothing keeps us alive: no GUI, no armed/paused countdown, no pending fire.
if (!mGuiStarted && !mTimerManager.hasActiveTimers() && !mPendingFired) {
    if (now - startTime > kStartupGraceMs) {   // 5 s grace after launch
        break;                                 // exit the run loop
    }
}
```

A short startup grace keeps the service alive just after launch so the GUI has time to appear; after that, an idle Timer with the GUI closed exits and frees its thread. A live or paused countdown, or a fire still waiting for its GUI, keeps it resident.

## TimerManager

`TimerManager` (in `TimerManager.hpp` / `TimerManager.cpp`) owns the single active countdown and the recents store. It is driven over a **caller-supplied** millisecond tick — the tick is passed into every control call rather than read internally, which keeps the state logic host-testable.

### Countdown state machine

```cpp
enum class TimerState : uint8_t { IDLE, RUNNING, PAUSED, FIRED };
```

| Method | Transition |
|--------|-----------|
| `start(dur, effect, now)` | arm a fresh countdown → `RUNNING` |
| `pause(now)` | `RUNNING` → `PAUSED`, banking the remainder |
| `resume(now)` | `PAUSED` → `RUNNING` from the remainder |
| `reset(now)` | reload the full duration; re-arms **only** when already `RUNNING`, otherwise restores the remainder and keeps the state |
| `stop()` | any → `IDLE`, remainder cleared |
| `repeat(now)` | reload the full duration, held `PAUSED` (resume from the Running screen) |
| `execute(now)` | while `RUNNING`, expire once `now - endTick >= 0` → `FIRED` and fire the observer; returns ms to the next required call, or "no timeout" when `IDLE`/`PAUSED`/`FIRED` |

The wrap-safe expiry test (`static_cast<int32_t>(now - endTick) >= 0`) stays correct across the 32-bit tick wrap. `reset()` deliberately refuses to turn a stopped or fired timer into a running one — a guarantee the Service relies on so a stray `RESET` can never silently restart a countdown with an alert still playing.

### Observer

The Service implements `TimerManager::Callback`:

- `onFired(timer)` — the countdown reached zero (drives the alert + GUI launch above)
- `onRecentsChanged(list)` — the recents list changed on load or save (forwarded to the GUI)

### Recents persistence

The last `Timer::kMaxRecents` (3) manually entered timers are stored as JSON in `timer.json`, using the SDK JSON stream reader/writer:

```json
{ "recents": [ { "sec": 300, "effect": "beep_vibro" }, ... ] }
```

Entries are validated on load (duration `<= 99:59`, effect one of `beep_vibro` / `vibro` / `beep`) and capped to three. A timer's **identity is its duration** (`Timer::operator==` compares `durationSec` only): the wheel shows only the time, so two timers of the same length are one entry, with the effect carried alongside as a last-used detail.

## Shared Types

`Timer` and `TimerState` live in `Libs/Header/Timer.hpp` and are shared verbatim by the service and the GUI:

```cpp
struct Timer {
    uint16_t durationSec;                 // 0 .. kMaxDurationSec (99:59)
    enum Effect : uint8_t { EFFECT_BEEP_AND_VIBRO, EFFECT_VIBRO, EFFECT_BEEP, EFFECT_COUNT };
    Effect   effect;
    enum Action : uint8_t { ACTION_START, ACTION_EDIT, ACTION_DELETE, ACTION_COUNT };

    static constexpr uint16_t kMaxDurationSec = 99 * 60 + 59;
    static constexpr size_t   kMaxRecents     = 3;   // single source of truth
    bool operator==(const Timer&) const;             // identity: duration only
};
```

`kMaxRecents` is the **single source of truth** for the recents cap: both the persistence layer (`TimerManager`) and the IPC message capacity (`CustomMessage`) derive from it, so they cannot drift apart.

## Custom Message System

All service↔GUI messages are declared in `Libs/Header/Commands.hpp` as `constexpr SDK::MessageType::Type` constants. Every message struct inherits from `SDK::MessageBase` and is wrapped in `#pragma pack(push, 4)` for pool alignment.

```cpp
namespace CustomMessage {
    // GUI --> Service
    constexpr Type TIMER_START        = 0x00000001;
    constexpr Type TIMER_CONTROL      = 0x00000002;   // carries a TimerCmd
    constexpr Type TIMER_RECENTS_SAVE = 0x00000003;

    // Service --> GUI
    constexpr Type TIMER_STATE        = 0x00000010;   // live countdown snapshot
    constexpr Type TIMER_FIRED        = 0x00000011;   // expiry (background flag)
    constexpr Type TIMER_RECENTS      = 0x00000012;
}
```

The live snapshot carries what the GUI needs to extrapolate locally — `endTick` for the running case, the frozen `remainingMs` for paused — plus the duration and effect:

```cpp
struct TimerStateMsg : public SDK::MessageBase {
    uint8_t       state;        // TimerState
    uint32_t      endTick;      // absolute expiry tick (RUNNING)
    uint32_t      remainingMs;  // frozen remainder (PAUSED)
    uint16_t      durationSec;
    Timer::Effect effect;

    TimerStateMsg();            // type id + defaults
    TimerStateMsg(TimerState state, uint32_t endTick, uint32_t remainingMs,
                  uint16_t durationSec, Timer::Effect effect);   // delegates to the above
};
```

`TimerFired` adds a `background` flag (true when the fire happened while the GUI was closed), and the two recents messages carry a fixed three-entry `RecentEntry[]` by value.

Each message type owns its field filling: alongside the defaulting default constructor, it has a constructor taking the fields it carries, delegating to that default. Sending is then one call in either direction — `SDK::send_msg<T>(kernel, args...)` allocates the message from the kernel pool, forwards `args...` to the constructor, sends it, and releases it, returning `false` if allocation or the send failed:

```cpp
// Service --> GUI
SDK::send_msg<CustomMessage::TimerStateMsg>(mKernel, s.state, s.endTick, s.remainingMs,
                                            s.durationSec, s.effect);
SDK::send_msg<CustomMessage::TimerFired>(mKernel, timer, false);
SDK::send_msg<CustomMessage::TimerRecents>(mKernel, list);

// GUI --> Service
SDK::send_msg<CustomMessage::TimerStart>(mKernel, timer.durationSec, timer.effect);
SDK::send_msg<CustomMessage::TimerControl>(mKernel, CustomMessage::TimerCmd::PAUSE);
```

There is no per-app sender class. See [Alarm-Architecture.md](Alarm-Architecture.md#sending-messages) for the full `send_msg` / `make_msg` contract, including when a reply matters.

**Message summary**:

| Message | Direction | Trigger |
|---------|-----------|---------|
| `TIMER_START` | GUI → Service | User starts a preset/custom timer |
| `TIMER_CONTROL` | GUI → Service | Pause / resume / reset / stop / repeat / replay-alert |
| `TIMER_RECENTS_SAVE` | GUI → Service | Recents list changed (add/remove) |
| `TIMER_STATE` | Service → GUI | After any command; on GUI start; startup routing |
| `TIMER_FIRED` | Service → GUI | Countdown reached zero |
| `TIMER_RECENTS` | Service → GUI | Recents loaded or saved |

## GUI

The GUI is built with TouchGFX and follows the Model-View-Presenter pattern, with several screens navigated by state.

### Project Structure

```text
Timer/Software/Apps/TouchGFX-GUI/
├── *.touchgfx        # TouchGFX Designer project
├── gui/              # Custom GUI code (Model, screens, containers)
├── generated/        # Auto-generated TouchGFX code
└── simulator/        # Simulator build
```

### Model

The `Model` is the GUI-side mirror of the state the service owns. It is never polled: it stores the last snapshot and computes the live remainder on demand (`getRemainingMs()` → `endTick - now` while running, frozen while paused, `0` when fired, full duration when idle).

Beyond mirroring, the Model owns the **selection and list state** that the multi-screen flow needs:

- **Presets & recents**: seven fixed presets (`{60, 180, 300, 600, 900, 1800, 3600}` s) built at construction, plus the recents mirror. `startTimer()` adds a custom or modified-preset timer to recents (an unchanged preset is already in the list, so it is not duplicated); `addRecent()` is move-to-front with de-dup, capped to three, and persists via the service.
- **Startup routing**: the app opens on a blank **Startup** screen; the first `TIMER_STATE` routes to the **Running** screen if a countdown is already live (a warm re-entry or cold start into a running timer) or to **Main** otherwise, so Main's "New" face never flashes first. A cold-start fire arrives as `TIMER_FIRED` and routes straight to **Fired**.
- **Selection restore**: after a Menu action (edit/delete) the Model can ask Main to re-select the edited timer, falling back to the item that took its slot when it was deleted.
- **Capabilities** (`setCapabilities()`): enables the USB charging screen; music control is off.
- **Idle timeout**: a 30-second inactivity timer (`kScreenTimeoutSteps`) closes the app; any button press resets it, and a hold keeps it awake for the whole press.

### Screens

| Screen | Role |
|--------|------|
| **Startup** | Blank landing frame; immediately routed away by the first service message (avoids a launch flash) |
| **Main** | Orbit-wheel of `New`, then presets, then recents; pick one to open its Menu |
| **Edit** (Set Timer) | Enter a custom duration on sphere-wheel pickers and choose the effect |
| **Menu** | Per-timer actions: Start / Edit / Delete (Delete offered for recents only, not presets) |
| **Running** | Live countdown with pause/resume/reset controls |
| **Fired** | Alert screen: Done / Repeat; re-indicates periodically and auto-Done after ~1 min |
| **Deleted** | Brief confirmation after removing a recent |
| **Alert** | Effect preview/selection |

### Reusable containers

- **OrbitMenu** — a "sphere" wheel: rows shrink and curve toward a large centre value, computed each frame from one scalar scroll position; navigation is discrete (`selectNext()/selectPrev()`). Used for the Main list.
- **SpherePicker** — the curved value picker used on the Edit screen for minutes/seconds.
- **CountdownTimer** — a self-registering tick widget that counts frames down and fires a callback at zero. Shared and header-only, from `SDK/GUI/CountdownTimer.hpp`.
- **ScrollIndicator** — the bezel rail/handle that accompanies scrollable lists.
- **Buttons** — the four watch-bezel arc indicators (L1/L2/R1/R2), each in one of five colours (hidden, white, amber, red, teal).
- **Title** — the screen heading.

> Note: containers that register themselves with TouchGFX (`OrbitMenu`, `SpherePicker`, `CountdownTimer`, `ScrollIndicator`) unregister in their destructors — the `FrontendHeap` partition is reused, so a stale timer-widget pointer would otherwise be ticked after the object is gone.

### Input Handling

The app is button-driven; the four buttons change meaning per screen (e.g. on Running: R1 pause/resume, L2 reset, R2 exit). In the simulator the buttons map to keys **`1 = L1`, `2 = L2`, `3 = R1`, `4 = R2`**.

## Build and Setup

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Timer/Software/Apps/Timer-CMake/`

```cmake
set(APP_NAME "Timer")
set(APP_USER_NAME "Timer")
set(APP_TYPE "Utility")
set(APP_AUTOSTART Off)
set(DEV_ID "UNA")
set(APP_ID "A1D74A3F0CFF98D3")

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
```

The app is **not** autostarted — it is launched on demand from the menu (or re-launched by the service to deliver a background fire).

### Build Targets

The service pulls in the common and app-system SDK groups plus **JSON** (for recents persistence); the GUI pulls in the common and GUI groups plus the generated TouchGFX sources:

```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_APPSYSTEM}
    ${UNA_SDK_SOURCES_JSON}
)
una_app_build_service(${APP_NAME}Service.elf)

set(GUI_SOURCES
    ${TOUCHGFX_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_GUI}
)
una_app_build_gui(${APP_NAME}GUI.elf)

una_app_build_app()
```

### Dependencies

**SDK Components**: UNA SDK common, GUI, and app-system sources; the JSON stream reader/writer; the kernel and messaging systems; the home-widget IPC (`SDK::HomeWidget`); buzzer/vibro/backlight request messages. No sensor or FIT sources.

**App Libraries** (`Libs/`):

- `Timer.hpp` — the shared `Timer` struct, `TimerState`, and the recents cap (single source of truth)
- `TimerManager` — the countdown state machine and JSON recents store (host-testable over an injected tick)
- `Commands.hpp` — the service↔GUI message contract
- `Service` — the service entry point, message loop, alerts, and home-widget pump

### Testability

`TimerManager` takes the current tick as a parameter on every control call rather than reading it internally, so its state machine and recents logic can be exercised on the host over an injected clock.

### Simulator Build

The TouchGFX project includes a Visual Studio simulator build in `simulator/msvs/Application.vcxproj`. In the simulator the buttons map to keys `1=L1 2=L2 3=R1 4=R2`.

See [SDK Setup and Build Overview](../sdk-setup.md) for the full development environment and toolchain.

## Conclusion

The Timer app pairs a small countdown state machine with everything a real countdown needs: it fires, it alerts, it persists, and it reaches out to the home screen. The service owns the state and stays silent between transitions; the GUI derives the live `MM:SS` from a shared monotonic clock and navigates a handful of state-driven screens; and the two are bridged by a compact, allocation-free message contract.

Key architectural strengths:

- **Deadline-bounded sleep**: the service's message wait is sized to the remaining time and the widget's next second, so it wakes only to fire, tick, or handle a command — and sleeps otherwise
- **Fire-and-launch**: a background expiry plays the alert and launches the GUI to deliver it, with a bounded wait so a dropped launch can never pin the service alive
- **Home-screen presence**: a backgrounded countdown surfaces `MM:SS` and a draining bar through the widget IPC, de-duplicated to one push per second
- **Single source of truth**: the recents cap is defined once and derived by both the store and the IPC layer, so persistence and message capacity cannot drift
- **State-derived UI**: screens and button meanings follow the countdown state, and the countdown logic stays host-testable behind an injected clock
```
