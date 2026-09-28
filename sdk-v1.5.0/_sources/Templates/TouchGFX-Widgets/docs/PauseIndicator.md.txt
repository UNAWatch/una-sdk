# PauseIndicator

## Overview

The PauseIndicator widget is a full-width overlay bar shown when a workout is paused. It displays a pause icon alongside the elapsed time formatted as `H:MM:SS` (when hours > 0) or `MM:SS`.

## Components

| Name | Type | Description |
|------|------|-------------|
| `background` | Circle arc (filled) | Dark gray `#404040` pill-shaped background, 164×34 px, centered at X=38 |
| `pauseIcon` | Image | `Pause_14x14.png` at X=76, Y=10 |
| `timerValue` | TextAreaWithOneWildcard | Elapsed time text, Poppins Italic 18 px, X=88, Y=5 |

Container size: **240×34 px** (full screen width).

The background is a large-radius circle centered above the visible area, producing a rounded top-pill shape.

## API

```cpp
void setTime(std::time_t seconds);
```

| Input | Output |
|-------|--------|
| `125` | `"02:05"` |
| `7424` | `"2:03:44"` |

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `PauseIndicator.tpkg`

2. **Add to screen**: drag the `PauseIndicator` container onto your screen. Position at X=0; Y depends on your layout (typically near the top).

3. **No designer properties** — time is set programmatically. Use `setVisible(true/false)` to show/hide the overlay.

## Usage in C++ Code

```cpp
#include <gui/containers/PauseIndicator.hpp>

PauseIndicator pauseIndicator;
add(pauseIndicator);
pauseIndicator.setPosition(0, y, 240, 34);
pauseIndicator.setVisible(false);  // hidden by default

// When workout pauses:
pauseIndicator.setVisible(true);
pauseIndicator.setTime(elapsedSeconds);

// Update elapsed time while paused:
pauseIndicator.setTime(elapsedSeconds);

// When workout resumes:
pauseIndicator.setVisible(false);
pauseIndicator.invalidate();
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `content/texts.xml` — text database (uses `TMP_ITALIC_18`)
- `files/gui/include/gui/containers/PauseIndicator.hpp` — header
- `files/gui/src/containers/PauseIndicator.cpp` — implementation
- `files/assets/images/Pause_14x14.png` — pause icon
- `files/assets/fonts/Poppins-Regular.ttf`, `Poppins-Italic.ttf`

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- Poppins Italic font (bundled in package)
- `TMP_ITALIC_18` text ID (provided by the `Texts` widget or defined manually)
