# Stopwatch - Interval Timer

## Overview

The Stopwatch app is a **Utility**-type application that measures elapsed time and records laps. It is a single-screen app: one face shows the running clock, the controls, and a scrollable list of laps, changing shape according to the state rather than switching between screens.

The app uses no sensors and writes no files. Its only job is timekeeping, and it does that without any periodic work: the kernel hands the service and the GUI the same monotonic millisecond tick, so a start timestamp taken by the service stays meaningful in the GUI. The GUI extrapolates the running time from the last snapshot against its own clock, and the service sends a snapshot only in response to an event — a state-changing command, the GUI starting, or an explicit request to resync — never on a timer. Between those events there is no traffic at all.

Key features include:
- Start, pause, resume, reset, and up to 50 recorded laps
- Tenths-of-a-second precision, matching the 10 Hz frame rate: a hundredths digit would be finer than the screen can move and would sit frozen
- Lap times shown as durations; the running total kept exact by storing lap boundaries rather than per-lap durations
- Two time faces (large and compact) chosen by the lap count, plus an hour form that widens the reading to `HH:MM:SS` once the clock passes an hour
- A readout capped at `99:59:59.9`
- Background counting: the service keeps timing while the GUI is closed or suspended, and re-entering the app picks the time back up
- The service ends itself once the GUI is gone **and** the clock is not running, so a stopped stopwatch holds no resident thread

## Architecture

The app is structured as two independent components: a service that owns the stopwatch state and a GUI that renders it. Communication between them uses the UNA SDK kernel messaging system.

### High-Level Components

1. **Service Layer**: The single source of truth for the stopwatch state; applies commands and publishes snapshots
2. **GUI Layer**: TouchGFX-based single screen; extrapolates the running time and draws the laps
3. **SDK Integration**: Kernel, messaging, monotonic system clock, GUI capabilities

### Component Interaction

```text
                    command (start/pause/lap/reset)
        [GUI] ─────────────────────────────────────► [Service]
          ▲                                               │
          │            snapshot (full State)              │
          └───────────────────────────────────────────────┘
          │
   [Kernel tick] ──► both processes read the same getTimeMs()
```

The headline design point is that the snapshot is **not** sent periodically. The service publishes a full `Stopwatch::State` in response to an event — a state-changing command, the GUI coming up, or an explicit resync request — but never on a timer. The GUI mirror never goes stale between snapshots because the running time is recomputed on every frame from `accumMs + (now - startMs)`, using the same kernel tick the service stamped the snapshot with. This keeps the message path idle while the clock runs, which is what lets the chip reach its low-power state with the stopwatch still counting.

## Service Backend

The service backend is implemented in `Service.hpp` and `Service.cpp`. It owns a `Stopwatch::Core` and answers every app command with a full snapshot.

### Core Classes and Structures

#### Service Class

```cpp
class Service
{
public:
    Service(SDK::Kernel &kernel);
    virtual ~Service() = default;
    void run();

private:
    SDK::Kernel     &mKernel;
    Stopwatch::Core  mStopwatch;
    bool             mGuiStarted;

    void handleCommand(SDK::MessageBase *msg);
    void publish();
};
```

`run()` blocks on `getMessage()` with an infinite timeout — there is no polling loop and no timer. It reacts to three kernel lifecycle messages plus the app's own commands:

- `COMMAND_APP_STOP` — forced exit; releases the message and returns
- `COMMAND_APP_NOTIF_GUI_RUN` — the GUI is up; marks `mGuiStarted` and publishes the current state so the screen opens on real data
- `COMMAND_APP_NOTIF_GUI_STOP` — the GUI is gone; see lifecycle below
- everything else falls through to `handleCommand()`

#### State Structure

`Stopwatch::State` is a plain struct shared verbatim between the service and the GUI (defined in `Libs/Header/Stopwatch.hpp`). It is small enough to travel in a single message and complete enough for the GUI to extrapolate from without further traffic:

```cpp
struct State {
    uint32_t accumMs;           // Elapsed time banked by previous runs
    uint32_t startMs;           // Tick at the last resume; valid only while running
    uint32_t laps[kMaxLaps];    // Elapsed time at each lap boundary
    uint8_t  lapCount;          // Number of valid entries in laps
    bool     running;           // True while the clock is advancing
};
```

**Laps are stored as boundaries, not durations.** Each entry is the elapsed time at the moment the lap was taken; a single lap's duration is derived on display as `laps[i] - laps[i-1]`. This keeps the running total exact instead of accumulating rounding across laps.

**Constants**:

| Constant | Value | Meaning |
|----------|-------|---------|
| `kMaxLaps` | 50 | Maximum laps retained; bounded so `State` fits the 256-byte message pool block |

#### Core Class

`Stopwatch::Core` holds the state and applies transitions over a **caller-supplied** clock. The tick is passed in rather than read internally, so the logic stays testable on the host and the caller decides which tick a transition is stamped with:

```cpp
class Core {
public:
    void start(uint32_t nowMs);   // Start/resume; no-op if already running
    void pause(uint32_t nowMs);   // Bank the time run so far
    void reset();                 // Clear elapsed time and every lap
    bool lap(uint32_t nowMs);     // Record a boundary; false if stopped or full
    bool isRunning() const;
    const State &state() const;
};
```

Two free functions in the same header do the timekeeping arithmetic and are shared by both processes:

- `elapsed(state, nowMs)` — `running ? accumMs + (nowMs - startMs) : accumMs`. The unsigned subtraction stays correct across the 32-bit tick wrap that happens roughly every 49.7 days.
- `lapDuration(state, index)` — the gap between one lap boundary and the previous one.

### Command Handling

`handleCommand()` reads the kernel tick once, applies the matching transition, and calls `publish()`:

| Message | Effect |
|---------|--------|
| `STOPWATCH_START` | `mStopwatch.start(now)` |
| `STOPWATCH_PAUSE` | `mStopwatch.pause(now)` |
| `STOPWATCH_LAP` | `mStopwatch.lap(now)` |
| `STOPWATCH_RESET` | `mStopwatch.reset()` |
| `STOPWATCH_REQUEST` | no state change; the following `publish()` answers with the current snapshot |

`publish()` sends the full state to the GUI, but only when `mGuiStarted` is true — there is no point publishing to a screen that is not there.

### Service Lifecycle and App Startup

Unlike the Alarm app, the Stopwatch service is **not** an autostart service. It is launched when the user opens the app, and it exists only to keep the clock advancing.

The kernel does not stop a service when its GUI closes. That is what lets the stopwatch keep counting after the user walks back to the menu — but it also means nothing else will ever reclaim the thread. The service therefore ends **itself** on `COMMAND_APP_NOTIF_GUI_STOP`, but only when the clock is not running:

```cpp
case SDK::MessageType::COMMAND_APP_NOTIF_GUI_STOP:
    mGuiStarted = false;
    if (!mStopwatch.isRunning()) {
        // A stopped stopwatch does no work and is not worth a resident thread.
        return;                       // state goes with it
    }
    break;                            // still counting: stay alive, GUI-less
```

So the resident thread is earned only while the clock advances. Close the GUI while paused or cleared, and the service exits and its state is discarded; close it while running, and the service stays alive with no GUI, still timing, until the app is reopened (which re-attaches to the resident service) or the clock is paused and the GUI closed.

## Custom Message System

All service↔GUI messages are declared in `Libs/Header/Commands.hpp` as `constexpr SDK::MessageType::Type` constants. Every message struct inherits from `SDK::MessageBase`, and all are wrapped in `#pragma pack(push, 4)` for alignment consistency with the message pool.

```cpp
namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type STOPWATCH_STATE   = 0x00000001;

// GUI --> Service
constexpr SDK::MessageType::Type STOPWATCH_START   = 0x00000002;
constexpr SDK::MessageType::Type STOPWATCH_PAUSE   = 0x00000003;
constexpr SDK::MessageType::Type STOPWATCH_LAP     = 0x00000004;
constexpr SDK::MessageType::Type STOPWATCH_RESET   = 0x00000005;
constexpr SDK::MessageType::Type STOPWATCH_REQUEST = 0x00000006;

} // namespace CustomMessage
```

`StopwatchState` carries the entire state — including the fixed 50-entry lap array — as a value, keeping the path allocation-free. A compile-time assertion guards the pool budget:

```cpp
struct StopwatchState : public SDK::MessageBase {
    Stopwatch::State state;
    StopwatchState()
        : SDK::MessageBase(STOPWATCH_STATE)
        , state{}
    {}

    explicit StopwatchState(const Stopwatch::State &state)
        : StopwatchState()
    {
        this->state = state;
    }
};
static_assert(sizeof(StopwatchState) <= 256,
              "StopwatchState must fit the largest kernel message pool block");
```

The second constructor fills the snapshot and delegates to the first — the shape every example app
uses, which keeps the type tag in a single initializer. It is also what makes publishing a single
call:

```cpp
SDK::send_msg<CustomMessage::StopwatchState>(mKernel, mStopwatch.state());
```

The remaining messages are empty commands, which need no arguments at all — the
GUI sends them the same way:

```cpp
bool startStopwatch() { return SDK::send_msg<CustomMessage::StopwatchStart>(mKernel); }
```

`SDK::send_msg<T>` allocates the message from the kernel pool, forwards any arguments to the
message's constructor, sends it, and releases it, returning `false` if allocation or the send
failed. There is no per-app sender class.

`send_msg` is for fire-and-forget sends, and it posts with a zero timeout — it never waits for a
reply, and a message that finds no room in the queue is dropped. `MainView` uses the returned bool for
exactly one press: pause is the only control whose screen holds a pending state (`mPausePending`), so
it must know the command left. Start, lap and reset discard it and let the answering snapshot correct
the display.

A dropped message is tolerated rather than recovered. `publish()` is itself a zero-timeout send, so a
snapshot can be lost too, and the GUI only re-requests one on start and resume — a snapshot dropped
mid-session is corrected by the next command, not immediately. That is an acceptable trade for an
example, not a pattern to copy into something that must not miss an update. Reach for
`SDK::make_msg<T>()` when the reply matters: it returns an RAII `MessageGuard` that releases on scope
exit, so you can send with a timeout and read the result back (`msg.send(timeout) && msg.ok()`). That
timeout bounds the wait for the reply, not for queue space — no send waits out a full queue.

**Message summary**:

| Message | Direction | Trigger |
|---------|-----------|---------|
| `STOPWATCH_STATE` | Service → GUI | After any command; on GUI start; in reply to a request |
| `STOPWATCH_START` / `_PAUSE` / `_LAP` / `_RESET` | GUI → Service | User presses the corresponding control |
| `STOPWATCH_REQUEST` | GUI → Service | GUI start and resume, to (re)sync the mirror |

## GUI

The GUI is built with TouchGFX and follows the Model-View-Presenter pattern. It has a single screen.

### Project Structure

```text
Stopwatch/Software/Apps/TouchGFX-GUI/
├── *.touchgfx        # TouchGFX Designer project
├── gui/              # Custom GUI code (Model, MainView/Presenter, containers, TimeFormat)
├── assets/           # Fonts and texts
├── generated/        # Auto-generated TouchGFX code
└── simulator/        # Simulator build
```

### Model

The `Model` is the GUI-side mirror of the state the service owns. It is never polled: it stores the last snapshot and computes the live time on demand.

```cpp
class Model : public touchgfx::UIEventListener,
              public SDK::Interface::IGuiLifeCycleCallback,
              public SDK::Interface::ICustomMessageHandler
{
public:
    bool startStopwatch();   // each returns whether the command was accepted
    bool pauseStopwatch();
    bool lapStopwatch();
    bool resetStopwatch();

    const Stopwatch::State &stopwatch() const;   // last snapshot
    uint32_t nowMs() const;                      // the same tick the service stamps
    uint32_t elapsedMs() const;                  // Stopwatch::elapsed(mState, nowMs())
};
```

**Key Model behaviours**:

- **`customMessageHandler()`**: accepts only `STOPWATCH_STATE`, copies the snapshot into `mState`, and notifies the listener via `onStopwatchChanged()`.
- **`elapsedMs()`**: extrapolates the running time from the stored snapshot against the current kernel tick. This is the mechanism that lets the display advance without any message from the service.
- **`onResume()` / `onStart()`**: send `STOPWATCH_REQUEST` so the mirror is resynced after the GUI was (re)started or unparked — the stopwatch may have moved on while the GUI was suspended.
- **`onSuspend()`**: intentionally empty. The service is untouched by a GUI suspend and keeps timing, so there is nothing to save.
- **`setCapabilities()`**: called in the constructor; enables the USB charging screen, phone notifications, and music-control overlays while this app is active.

There is **no idle-timeout timer**. Unlike most apps, this one never closes itself on inactivity: the user leaves via the red R2 button, and a kernel suspend simply parks the GUI while the service keeps time.

### The Single Screen: MainView

`MainView` renders three shapes rather than three screens — which shape is showing is derived from the state, not tracked separately:

```cpp
enum class Mode { Idle, Running, Paused };
```

- **Idle**: cleared, nothing timed yet (`accumMs == 0 && lapCount == 0`)
- **Running**: the clock advances; `handleTickEvent()` refreshes the reading each frame
- **Paused**: stopped with time on the clock

Only a running screen redraws itself on the tick; idle and paused screens are repainted by the snapshot that put them there.

#### Two faces and the hour form

The time readout has two **faces**, chosen by the lap count, and an orthogonal **hour form** applied within either face:

- **Large face** (default): big reading centred on the display, laps below a separator line.
- **Compact face**: once the laps outgrow what the large reading leaves room for, the reading shrinks and rises under the title, giving the lap list more rows. The threshold (`kCompactFromLaps`) is derived from the large list's row capacity, not hard-coded, so it cannot drift from the geometry.
- **Hour form**: once the clock passes an hour the reading widens to `HH:MM:SS` and drops the tenths (there is no room for them, and sub-second precision past an hour is pointless). The font shrinks — `60 → 40` on the large face, `40 → 35` on the compact — and the reading stretches to reclaim the space the fraction leaves.

Only the **compact** geometry is spelled out in code; the large face is read back from the widgets in `setupScreen()`, so moving them in the Designer is enough and nothing has to be mirrored by hand. The lap list is the exception: the generator sizes its drawable pool from the height it is given in the Designer, so the Designer holds the taller (compact) list rect and the shorter large-face rect is the constant.

`applyLayout(compact, hours)` memoizes on the current `(face, hours)` pair and only calls `setLayout()` on a real change. `setLayout()` invalidates the outgoing rectangles **before** moving the widgets, so the old area is repainted and does not leave stale pixels behind.

The `HH:MM:SS` cap at `99:59:59.9` and all the field formatting live in `gui/common/TimeFormat.hpp`, which fills the gap left by `SDK::Utils::toHMS` (that helper works in whole seconds and cannot express tenths).

#### Lap list and scroll indicator

The laps are shown in a TouchGFX scroll list of `LapListItem`s. `lapListUpdateItem()` fills each row with its index and derived duration; out-of-range rows are cleared. `refreshLaps()` reads the previous count back from the list itself (no shadow copy), grows or shrinks it, and follows a newly taken lap so the newest row stays in view. Because `animateToItem()` only scrolls when the target lies outside the window, it is aimed at the row on the edge the window is moving toward.

The `ScrollIndicator` rail keeps the lap list company from the very first lap so it does not pop in halfway through a run. Its handle appears only once the list is actually scrollable (more laps than fit). Scrolling belongs to the running clock: on pause the whole indicator is taken away and L2 becomes the reset action.

### Input Handling

The screen reads four buttons; several change meaning with the clock:

| Button | While Running | While Paused / Idle |
|--------|---------------|---------------------|
| **R1** | Pause | Start / resume |
| **R2** | Lap (white) | **Exit** the app (red) |
| **L1** | Scroll laps up | — |
| **L2** | Scroll laps down | Reset (only when paused) |

R2 is the only way out of the app, and only once the clock is stopped: while running it is white and takes a lap; while stopped it turns red and leaves. The button-arc colours track the icon next to each: the pause icon is amber and the play icon teal (R1), reset is white (L2). L1 stays dark even though it scrolls, because the scroll indicator sits on the same stretch of bezel and is the navigation cue on its own.

A subtlety on pause: R1 sets `mPausePending` only if the pause command was actually accepted for delivery. Until the confirming snapshot arrives the mirror still reads as running, so ticking the display on would run it past the moment the service stopped and the snapshot would then yank the digits backward; holding the display until the snapshot avoids that.

## Build and Setup

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in `Stopwatch/Software/Apps/Stopwatch-CMake/`

```cmake
set(APP_NAME "Stopwatch")
set(APP_USER_NAME "Stopwatch")
set(APP_TYPE "Utility")
set(DEV_ID "UNA")
set(APP_ID "A159B1C005CFD2A5")

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
```

The app is **not** autostarted — it is launched on demand from the menu.

### Build Targets

The source lists are deliberately narrow. The service owns no sensors and writes no files, so the FIT, sensor, track-map, and calibration groups of `UNA_SDK_SOURCES_SERVICE` are left out; only `COMMON` and `APPSYSTEM` are pulled in:

```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_APPSYSTEM}
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

**SDK Components**: UNA SDK common, GUI, and app-system sources; the kernel and messaging systems. No sensor, FIT, or file-system sources.

**App Libraries** (`Libs/`):
- `Stopwatch.hpp` — state struct, timekeeping arithmetic, and the `Core` transition logic (header-only, host-testable)
- `Commands.hpp` — the service↔GUI message contract
- `Service` — the service entry point and message loop

### Host Tests

The timekeeping logic is covered by host unit tests in `Tests/Host/apps/Stopwatch/`, exercising `Stopwatch::Core` over an injected clock: start/pause/resume accumulation, tick-wrap correctness, lap boundaries and durations, the full-lap-store guard, and the "a stopped clock is not running whatever it holds" invariant.

### Simulator Build

The TouchGFX project includes a Visual Studio simulator build in `simulator/msvs/Application.vcxproj`. In the simulator the buttons map to keys `1=L1 2=L2 3=R1 4=R2`.

See [SDK Setup and Build Overview](../sdk-setup.md) for the full development environment and toolchain.

## Conclusion

The Stopwatch app is the second simple **Utility** example alongside Alarm, and it demonstrates a different corner of the SDK. Where Alarm shows an autostart service with autonomous GUI launch and JSON persistence, Stopwatch shows a **traffic-free, extrapolation-based** design: a service that owns a tiny state and is silent between transitions, a GUI that derives the live reading from a shared monotonic clock, and a single screen that reshapes itself — two faces plus an hour form — instead of navigating between many.

Key architectural strengths:
- **Zero periodic IPC**: the running time is computed, not messaged; the message path is idle while the clock runs, letting the chip stay in low power
- **Exact totals**: laps stored as boundaries keep the running total free of accumulated rounding
- **Allocation-free path**: the whole state, including the fixed lap array, fits one 256-byte pool block
- **Purposeful lifetime**: the service earns its thread only while the clock advances, and ends itself otherwise — background counting is free, but a stopped stopwatch costs nothing
- **State-derived UI**: one screen with three modes and two faces, each derived from the state rather than tracked, keeps the view logic small and consistent
