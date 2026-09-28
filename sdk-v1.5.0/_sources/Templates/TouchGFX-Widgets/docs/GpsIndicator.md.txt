# GpsIndicator

## Overview

The GpsIndicator widget is a self-animating 8×8 px green dot that blinks while the device is searching for a GPS fix. Once a fix is acquired it hides itself and unregisters from the TouchGFX timer automatically.

## Components

| Name | Type | Description |
|------|------|-------------|
| `dot` | Circle (filled) | 8×8 px green dot (`#00C040`), radius 4 px |

Container size: **8×8 px**.

The widget starts hidden (`setVisible(false)`) in `initialize()` to avoid a phantom flash on screen entry.

## Behavior

| State | Behavior |
|-------|----------|
| `acquired = false` | Dot blinks at `mPeriod` tick interval (toggles visible/hidden) |
| `acquired = true` | Dot hides on the next tick, timer unregisters |
| GPS lost (acquired → false) | Timer re-registers, blinking resumes |

## API

```cpp
void setPeriod(uint32_t ticks);      // ticks between blink toggles (call before screen shown)
void setAcquired(bool acquired);     // update GPS fix state
```

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `GpsIndicator.tpkg`

2. **Add to screen**: drag the `GpsIndicator` container onto your screen and position it.

3. **No designer properties** — period and state are controlled in C++.

## Usage in C++ Code

```cpp
#include <gui/containers/GpsIndicator.hpp>

GpsIndicator gpsIndicator;
add(gpsIndicator);
gpsIndicator.setPosition(x, y, 8, 8);

// In setupScreen():
gpsIndicator.setPeriod(30);          // blink every 30 ticks (~0.5 s at 60 fps)
gpsIndicator.setAcquired(false);     // start searching

// On GPS state change:
gpsIndicator.setAcquired(true);      // fix acquired — hides automatically
gpsIndicator.setAcquired(false);     // fix lost — resumes blinking
```

## Notes

- The timer is registered in `initialize()` and is automatically cleaned up in the destructor.
- `setPeriod()` should be called in `setupScreen()` before the widget is shown.
- No manual `setVisible()` calls are needed — the widget manages its own visibility.

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/GpsIndicator.hpp` — header
- `files/gui/src/containers/GpsIndicator.cpp` — implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No image assets — fully vector-based
