# ClockfaceAnalogue - Analogue Clockface

## Overview

`Analogue` is a **Clockface**-type application: an analogue face with an hour
and a minute hand over a printed dial, plus the day, the date, a battery
indicator and a muted-alerts icon. The kernel hands a clockface app the display
in place of its built-in home screen, so this is what the user sees when
nothing else is running.

| | |
|---|---|
| Directory | `Examples/Apps/ClockfaceAnalogue` |
| `APP_NAME` / `APP_USER_NAME` | `Analogue` |
| `APP_TYPE` | `Clockface` |
| `APP_ID` | `A199D48D5F2BB3D0` |
| Packed flags | `0x03` |
| Screens | 1 |
| Display | 240 x 240, `LCD8bpp_ABGR2222` |

The directory prefix is the `APP_TYPE`; the launcher name carries no prefix.

### What it is not

- **No second hand.** The fastest thing the face draws turns once a minute.
- **No touch, no buttons, no navigation.** A clockface is a display, not a
  screen the user drives.
- **No glance interface.** The app serves no glance data, and the packed image
  says so: flags `0x03`, glance-capable bit clear.

## Layout

Positions are the widget's top-left corner on the 240 x 240 face. Everything
here is set in the Designer; none of it is repeated in code.

| Element | Widget | Position | Size | Detail |
|---|---|---|---|---|
| Dial | `Image` | 0, 0 | 240 x 240 | `Dial_240x240.png` |
| Muted alerts | `Image` | 34, 108 | 30 x 25 | `SpeakerMute_30x25.png`, hidden unless muted |
| Battery level | `Battery_45x14` | 160, 113 | 45 x 14 | four segments |
| Day name | `TextArea` | 76, 167 | 88 x 22 | Poppins Medium 16, `192, 128, 0` |
| Day of month | `TextAreaWithOneWildcard` | 76, 185 | 88 x 38 | Poppins Medium 26, `192, 192, 192` |
| Hands, hub | `Line` x4, `Circle` | 0, 0 | 240 x 240 | `192, 192, 192` |

The hands and the hub are given the whole face as their widget box so that a
hand at any angle stays inside it; what is actually rasterised is each stroke's
own minimal rect.

## Architecture

### Components

1. **Service** - owns the clock and the charge level, and publishes both. The
   mute state is published from here too.
2. **GUI** - TouchGFX. One screen, two images, two text areas, one container
   and five canvas widgets.
3. **SDK integration** - the GUI lifecycle callbacks and the custom-message
   handler.

```text
   [sensor layer] ──EVENT_SENSOR_LAYER_DATA──► Service ◄── clock, once a minute
                                                  │
                                       CustomMessage::Time
                                       CustomMessage::Battery
                                       CustomMessage::AlertsMuted
                                                  │
                                                  ▼
   [kernel] ──EVENT_GUI_TICK──► frame ──► FrontendApplication::handleTickEvent
                                                                 │
                                                          callCustomMessageHandler
                                                                 │
                                                                 ▼
                                                  Model ──► MainPresenter ──► MainView
                                       (hands, day, date, battery, mute)
```

Both values the face shows are read in the service and pushed to the GUI. The
GUI reads the clock itself only at the two moments no push is due -- when it is
built and when it resumes -- and nothing it shows is sampled on a frame.

### Where the clock comes from

The clock is `time()` and `localtime_r()`; there is no SDK time interface.

The face draws nothing finer than a minute, so the service reads the clock once
a turn round its loop. One reading does both jobs: it is what gets published,
and it is what says how long of the current minute is left, which is how long
the service then waits for a message. Publishing happens before the wait rather
than on the wait expiring, so a message arriving just before a boundary cannot
swallow that minute; a reading equal to the last one sent is dropped, so an
early turn costs nothing. Each wait is sized from a fresh reading, so a late
wake-up cannot accumulate into drift.

The GUI takes two readings of its own, for the two moments it has to be right
at and no minute boundary is due: when the `Model` is built, and when the face
resumes after being off screen. Both are direct calls, so neither waits on the
service and both are in place before the frame that follows them is drawn.

`WallTime` carries `hour`, `minute`, `mday` and `wday` - everything the face
draws and nothing finer. Two readings compare equal when all four match, which
is what lets the screen decide in one test whether anything has to be redrawn.

The hands are driven from a 12-hour dial, so no 12/24 preference is needed. The
face shows the day of the month as a bare number, so no date order is needed
either.

### Where the charge level comes from

The charge level comes from the sensor layer:

```cpp
SDK::Sensor::Connection mBatterySensor{ SDK::Sensor::Type::BATTERY_LEVEL };
```

The sensor is event driven, not periodic. Connecting publishes the current
level, which is what fills the indicator on boot; after that an event arrives
when the level changes and at no other time. Nothing polls it and nothing
samples it on a schedule.

The service connects once at startup and holds the subscription for the life of
the app. A face is on screen whenever nothing else is running, so there is no
moment worth deferring to, and a subscription that only speaks on a change
costs nothing to keep open.

### The service publishes, it does not decide

Everything the service reports leaves it through one publisher each:

```cpp
void publishTime();
void publishBatteryLevel(uint8_t level);
void publishAlertsMuted(bool muted);
```

Each drops a value equal to the one it last sent, so a source may call as often
as it likes and only a real change costs an IPC round trip. `handleSensorData`
therefore only matches the driver, parses and hands the level on. The `Model`
filters again on arrival, so an unchanged value produces no callback either.

All three messages run service to GUI, one-way, and their results are never
inspected, so `customMessageHandler` returns `true` unconditionally:

```cpp
namespace CustomMessage {
    constexpr SDK::MessageType::Type BATTERY      = 0x00000001;
    constexpr SDK::MessageType::Type ALERTS_MUTED = 0x00000002;
    constexpr SDK::MessageType::Type TIME         = 0x00000003;

    struct Time        : public SDK::MessageBase { uint8_t hour, minute, mday, wday; };
    struct Battery     : public SDK::MessageBase { uint8_t level; };   // 0-100
    struct AlertsMuted : public SDK::MessageBase { bool    muted; };
}
```

`FrontendApplication::handleTickEvent` drains the queue by calling
`callCustomMessageHandler()` once per frame. That call is not the frame gate;
it only moves messages onto the GUI thread.

### The battery indicator

`Battery_45x14` is a reused container: four segments in 25 % bands, teal
(`0x008080`) when reached, dark grey (`0x404040`) when not, and the first
segment red (`0xC00000`) below 25 %. At 0 % nothing is lit.

Its segments sit at fixed coordinates rather than being laid out, so the
container works at the one size its name gives. A different size is a different
container.

### The muted-alerts icon

The icon is on screen only while alerts are silenced. What decides that is
state, not a call to `setVisible` in the Designer: `Model::alertsMuted()`
starts false and `MainView::setupScreen` applies it, so the icon starts hidden
because alerts are not muted. The Designer keeps it visible so it can be
positioned.

`MainView::setAlertsMuted` uses `invalidate()`, not `invalidateContent()`. The
latter does nothing on a widget that has just been hidden, which is exactly
when the area it used to cover has to be repainted.

> **Nothing drives it yet.** The SDK exposes no mute state for an app to read,
> so `Service::publishAlertsMuted` has no caller and the icon stays hidden on a
> real watch. The icon, the message, the filtering and the path that shows and
> hides it are all here and all exercised; attaching a source to that one call
> is the whole of what remains.

## The hands

Each hand is two `Line` widgets with `ROUND_CAP_ENDING` - a wide bar and a
narrow stem that carries on to the hub - and both hands share one `Circle`
drawn as an open ring. All five are drawn in the Designer pointing at twelve
o'clock.

**No geometry is written down in code.** `MainView::setupScreen` reads
`hub.getCenter()` for the pivot and `getStart()` / `getEnd()` for each stroke,
keeps the endpoints relative to the pivot, and `setHandAngle()` rotates those
vectors. Length, thickness, cap style, colour and where the bar ends and the
stem begins all come from the generated `MainViewBase`. Retuning the design is
an edit in the Designer with no matching edit in code, and moving the hub moves
what the hands turn about.

As drawn, measured from the hub in pixels, cap included:

| | stem | bar | thickness |
|---|---|---|---|
| Hour hand | 7 - 21 | 17 - 83 | 2 / 6 |
| Minute hand | 7 - 21 | 17 - 98 | 2 / 6 |

The hub ring spans radius 6 to 8, so each stem ends underneath it and the ring
stays clear inside.

Three things the Designer content has to keep, or rotation breaks:

1. every `Line` box stays at `(0, 0, 240, 240)` - a line is clipped by its own
   box, and a rotated hand leaves a box shaped for a vertical one;
2. the strokes are drawn vertically, pointing up, since that is the zero angle;
3. the hub exists, because it is the only source of the pivot.

Angles come straight off the dial, and screen y grows downwards, so the plain
rotation matrix already turns clockwise:

```cpp
minuteAngle = minute * 6.0f;                        // 360 / 60
hourAngle   = (hour % 12) * 30.0f + minute * 0.5f;  // 360 / 12, plus the creep
```

The hour hand creeps with the minutes rather than jumping on the hour, which is
what a mechanical movement does and what makes the pair read as one time rather
than two readings. It also means both hands move on the same event, so the
redraw test stays a single comparison.

Measured against the dial at eight exact times, with everything but the hands
hidden: the angle error stays under **0.15 degrees**, against a minute mark of
6 degrees. The tip radius holds constant across all angles - minute 97.5 to
97.9, hour 82.5 to 83.0 - which is also what shows that nothing is being
clipped.

### Why not a rotated bitmap

The obvious alternative is to draw each hand once as artwork and rotate the
bitmap with the texture mapper. Measured on this display, against the same hand
drawn as vector strokes, deviation of the hand's centreline from a straight
line, in pixels:

| | 0 deg | 2 deg | 6 deg |
|---|---|---|---|
| vector strokes | 0.044 | 0.061 | 0.082 |
| bitmap, nearest neighbour | 0.082 | 0.278 | 0.278 |
| bitmap, bilinear | 0.082 | 0.118 | 0.112 |

The 0.278 is a quantisation floor rather than an angle effect - it is the same
at 2 and 6 degrees. Worse is what happens in motion. The hour hand advances
half a degree a minute, and under nearest-neighbour sampling that step changes
no pixels at all on one minute and then moves a double step on the next
(0.000, 0.167, 0.333, 0.167 px), while the vector strokes move on every step.
A bitmap hand also comes out about 0.15 px fatter, because its anti-aliased
edge is quantised into the framebuffer's four levels per channel before any
rotation happens.

Pre-rendering one sprite per position does not rescue it either: a thin hand
rotated 45 degrees has an 82 x 82 bounding box, about 6.7 KB in ABGR2222, which
does not compress - times sixty minute positions.

### Canvas buffer

Rasterising the strokes needs the screen's canvas buffer, sized by
`CanvasBufferSize` in the `.touchgfx`. Sweeping all 720 hand positions with
TouchGFX's own memory report on peaks at **1872 bytes** and never splits a
draw, so the **3600** the screen declares covers the worst case. Re-measure
after retuning a stroke: a wider or longer one costs more outline.

## Assets

The tick ring is one image, not a ring of widgets. Twelve hour marks and
forty-eight minute dots would otherwise be sixty drawables that never change,
each with its own invalidation bookkeeping; as a bitmap it is one blit and the
hands simply repaint over it.

| File | Size |
|---|---|
| `Dial_240x240.png` | 240 x 240 |
| `SpeakerMute_30x25.png` | 30 x 25 |

## Texts

The day name is seven static Text IDs, not a wildcard. The set is known at
build time, so the face swaps the `TextArea`'s typed text instead of writing
characters into a RAM buffer, and translation stays a text-database job:

```cpp
// AnalogueLabels.hpp, indexed by std::tm::tm_wday
inline constexpr touchgfx::TypedTextId kDayLabels[7] = {
    T_TEXT_SUN, T_TEXT_MON, T_TEXT_TUE, T_TEXT_WED,
    T_TEXT_THU, T_TEXT_FRI, T_TEXT_SAT
};
```

| Text ID | Typography | Buffer | Content |
|---|---|---|---|
| `TEXT_MON` … `TEXT_SUN` | `Poppins_Medium_16` | none | `Mon` … `Sun` |
| `TMP_MEDIUM_26` | `Poppins_Medium_26` | - | wildcard template |
| `WC_DATE` | `Poppins_Medium_26` | 3 | day of month |

The day of month is the one string written at runtime, so it is the one with a
buffer. The base class binds it once; the face only `snprintf`s into the buffer
and invalidates. Only `Poppins-Medium.ttf` is shipped, because only it is used.

## Frame pacing

The kernel paces the app with one `EVENT_GUI_TICK` per frame. On target,
`OSWrappers::waitForVSync` blocks in
`TouchGFXCommandProcessor::waitForFrameTick` until that tick arrives, so one
TouchGFX frame is one kernel tick and `FrontendApplication::handleTickEvent` is
the per-frame hook.

Anything that does need per-frame work belongs in a `handleTickEvent`, not in
`IGuiLifeCycleCallback::onFrame()`. The two look equivalent and are not:
`onFrame` is driven from `waitForFrameTick` on target, but the simulator's
`App::Core::runGuiComm` calls `waitForFrameTick` as a bare sync primitive and
never sends an `EVENT_GUI_TICK`, so a face built on `onFrame` works on hardware
and sits frozen in the simulator.

The app holds no notion of the rate, and nothing it shows is sampled per frame.
`MainView` has no `handleTickEvent` at all: the tick drains the message queue,
and a tick that brings no message does nothing. What the face draws is decided
by what arrives - the hands and the date by a `Time` message, the battery
level by a `Battery` one - so the tick rate costs the app nothing but the drain.

This is what having no second hand buys, and it does not generalise. A value
that turns once a second has to be sampled at least twice a second to be shown
without stumbling, and at that rate it may as well be read on the frame.

### Resume

`Model::onResume()` only raises a flag. `tick()` acts on it: it takes a reading,
because the face may have been off screen across a minute boundary and the next
push would not be due until the one after it, and then invalidates the whole
screen, because whatever the kernel drew over the face is still in the
framebuffer when the app comes back.

Both belong in `tick()` rather than in `onResume()` because `tick()` runs on the
thread that draws. `onResume()` is dispatched from `waitForFrameTick()`, which
the simulator drives on a thread of its own while TouchGFX renders on the main
one, so touching a widget from there would race the render loop. The reading
still lands before the repaint, so the hands come back already right.

## Simulator

The simulator builds the service, the GUI and a simulated sensor layer, so the
battery path is exercised end to end rather than left until hardware.
`simulator/ConfigurationSimulator.hpp` configures that layer: only the battery
sensor is enabled, starting at 78 % and falling 0.2 %/s, so the indicator
visibly crosses a band about twenty seconds in.

## Build and Setup

### Build System Overview

**Primary Build File**: `CMakeLists.txt` in
`ClockfaceAnalogue/Software/Apps/Analogue-CMake/`

```cmake
set(APP_NAME "Analogue")
set(APP_USER_NAME "Analogue")
set(APP_TYPE "Clockface")
set(DEV_ID "UNA")
set(APP_ID "A199D48D5F2BB3D0")

include($ENV{UNA_SDK}/cmake/una-app.cmake)
include($ENV{UNA_SDK}/cmake/una-sdk.cmake)
```

`APP_TYPE` is what packs the image as a clockface and leaves the
glance-capable bit clear. The app is not autostarted; the kernel gives a
clockface the display in place of its own home screen.

### Build Targets

The service takes one optional SDK group, the sensor layer, which is where the
charge level comes from; the GUI takes the common and GUI groups plus the
generated TouchGFX sources:

```cmake
set(SERVICE_SOURCES
    ${LIBS_SOURCES}
    ${UNA_SDK_SOURCES_COMMON}
    ${UNA_SDK_SOURCES_APPSYSTEM}
    ${UNA_SDK_SOURCES_SENSOR}
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

The face keeps no state and writes no files, so it takes neither the JSON nor
the file-system group.

### Dependencies

**SDK Components**: common, GUI and app-system sources, and the sensor layer
for `BATTERY_LEVEL`. No JSON, no FIT, no file system.

**App Libraries** (`Libs/`):

- `Commands.hpp` - the three service-to-GUI messages and their types
- `Service` - the clock loop, the sensor subscription and the publishers

See [SDK Setup and Build Overview](../sdk-setup.md) for the full development
environment and toolchain, and [Simulator](../Simulator.md) for the Windows and
Linux simulator builds.

## Known gaps

- **A change of clock is not seen until the minute turns.** The service is
  asleep until its next boundary, so a clock corrected underneath it -- by a
  phone sync, or by hand -- reaches the face up to a minute later. Leaving the
  face and returning to it corrects it at once, because resuming asks.
