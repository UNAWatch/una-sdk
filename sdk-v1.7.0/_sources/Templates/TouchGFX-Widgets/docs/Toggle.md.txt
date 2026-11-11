# Toggle

## Overview

The Toggle widget is a two-state switch implemented entirely with vector primitives (no image assets). A rounded `rail` changes color and a circular `handle` slides horizontally to indicate the ON or OFF state.

## Components

| Name | Type | Description |
|------|------|-------------|
| `rail` | Line (round cap) | Background track, 60×30 px |
| `handle` | Circle | Sliding knob, 30×30 px |

Container size: **60×30 px**.

## Default Colors

| Element | OFF | ON |
|---------|-----|----|
| Rail | `#000000` (black) | `#C08000` (amber) |
| Handle | `#C0C0C0` (gray) | `#C0C0C0` (gray) |

Colors can be overridden before the first `setState()` call.

## API

```cpp
void setState(bool state);          // Set ON/OFF and update visuals
bool getState() const;              // Return current state

void setRailOnColor(colortype);     // Rail color when ON
void setRailOffColor(colortype);    // Rail color when OFF
void setHandleOnColor(colortype);   // Handle color when ON
void setHandleOffColor(colortype);  // Handle color when OFF
```

Handle position: X = 0 (OFF), X = 30 (ON).

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Toggle.tpkg`

2. **Add to screen**: drag the `Toggle` container onto your screen and position it.

3. **No designer properties** — all behavior is controlled in C++.

## Usage in C++ Code

```cpp
#include <gui/containers/Toggle.hpp>

Toggle toggle;
add(toggle);
toggle.setPosition(x, y, 60, 30);

// Basic usage
toggle.setState(true);
bool on = toggle.getState();

// Custom colors (must be set before setState)
toggle.setRailOnColor(touchgfx::Color::getColorFromRGB(0, 200, 100));
toggle.setState(true);

// React to state change
void MyView::onToggleTapped() {
    toggle.setState(!toggle.getState());
}
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/Toggle.hpp` — header
- `files/gui/src/containers/Toggle.cpp` — implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No image assets — fully vector-based
