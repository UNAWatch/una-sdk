# Battery

## Overview

The Battery widget is a four-segment charge indicator drawn entirely with vector primitives (Box and Line components). Each segment represents a 25 % charge band and changes color based on the current level.

## Components

The widget contains four segments (s1–s4, left to right), each composed of a Box and two Lines:

| Segment | Level threshold | Color at threshold |
|---------|----------------|--------------------|
| s1 | > 0 % (low) | Red `#C00000` |
| s1 | ≥ 25 % | Teal `#008080` |
| s2 | ≥ 25 % | Teal `#008080` |
| s3 | ≥ 50 % | Teal `#008080` |
| s4 | ≥ 75 % | Teal `#008080` |

s4 also includes a terminal nub (third Line) that represents the battery cap.

Container size: **90×28 px**.

## Color Scheme

| State | Color |
|-------|-------|
| Inactive segment | `#404040` (dark gray) |
| Active segment | `#008080` (teal) |
| Low battery (s1 only, 1–24 %) | `#C00000` (red) |

## API

```cpp
void setLevel(uint8_t level);   // level: 0–100 (percentage)
```

| Level | Visual |
|-------|--------|
| 0 % | All segments inactive (dark gray) |
| 1–24 % | s1 red, s2–s4 inactive |
| 25–49 % | s1–s2 teal, s3–s4 inactive |
| 50–74 % | s1–s3 teal, s4 inactive |
| 75–100 % | All segments teal |

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Battery.tpkg`

2. **Add to screen**: drag the `Battery` container onto your screen and position it.

3. **No designer properties** — level is set programmatically.

## Usage in C++ Code

```cpp
#include <gui/containers/Battery.hpp>

Battery battery;
add(battery);
battery.setPosition(x, y, 90, 28);

// Update charge level (0-100)
battery.setLevel(currentChargePercent);
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/Battery.hpp` — header
- `files/gui/src/containers/Battery.cpp` — implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No image assets — fully vector-based
