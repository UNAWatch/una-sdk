# Buttons

## Overview

The Buttons widget renders four arc indicators that represent the physical side buttons of the UNA Watch (L1, L2 on the left bezel; R1, R2 on the right bezel). Each arc can be shown in one of four states: hidden, white, amber, or red — communicating the action assigned to that button in the current screen context.

## Components

Four `Circle` arc components, all sharing the same 240×146 px canvas and drawn at radius 113 px:

| Name | Position | Angles |
|------|----------|--------|
| `bL1` | Top-left | 291°–308° |
| `bL2` | Bottom-left | 232°–249° |
| `bR2` | Bottom-right | 111°–128° |
| `bR1` | Top-right | 52°–69° |

Container size: **240×146 px** (matches the watch screen width; positioned at Y=47 to align with the bezel).

## Color States

```cpp
enum Color {
    NONE  = 0,   // Indicator hidden
    WHITE,        // #C0C0C0 — neutral action
    AMBER,        // #C08000 — primary action
    RED,          // #C00000 — warning / destructive action
};
```

## API

```cpp
void setL1(Color color = NONE);   // Top-left button
void setL2(Color color = NONE);   // Bottom-left button
void setR1(Color color = NONE);   // Top-right button
void setR2(Color color = NONE);   // Bottom-right button
```

Calling a setter with `NONE` hides the arc. Any other value makes it visible with the specified color.

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Buttons.tpkg`

2. **Add to screen**: drag the `Buttons` container onto your screen. Position it at X=0, Y=47 to align the arcs with the physical bezel edges.

3. **No designer properties** — colors are set programmatically per screen.

## Usage in C++ Code

```cpp
#include <gui/containers/Buttons.hpp>

Buttons buttons;
add(buttons);
buttons.setPosition(0, 47, 240, 146);

// Indicate button roles for the current screen
buttons.setL1(Buttons::NONE);     // Left-top: unused
buttons.setL2(Buttons::AMBER);    // Left-bottom: primary action
buttons.setR1(Buttons::WHITE);    // Right-top: neutral
buttons.setR2(Buttons::RED);      // Right-bottom: stop / discard

// Hide all indicators
buttons.setL1();
buttons.setL2();
buttons.setR1();
buttons.setR2();
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/Buttons.hpp` — header
- `files/gui/src/containers/Buttons.cpp` — implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No image assets — fully vector-based
