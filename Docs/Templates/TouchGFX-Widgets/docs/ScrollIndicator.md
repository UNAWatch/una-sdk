# ScrollIndicator

## Overview

The ScrollIndicator widget renders a circular arc indicator on the left edge of the screen that tracks the selected item in a scroll-wheel menu. A sliding arc handle moves along a background rail arc as the user navigates the list. It supports smooth animated transitions including wrap-around (last → first and first → last) using a dual-handle technique.

## Components

| Name | Type | Description |
|------|------|-------------|
| `rail` | Circle arc | Background track, 232°–308°, radius 112 px |
| `handle` | Circle arc | Active position indicator, slides along the rail |
| `handleOvf` | Circle arc | Hidden; used only during wrap-around animations |

Container size: **36×146 px**, positioned at X=0, Y=47. The arc center is at (120, 73) relative to the screen — the same virtual center used by `Buttons` — so the indicator aligns with the bezel geometry.

## Predefined Configs

Two configurations are available as `static constexpr` members:

| Config | Rail range | Handle span | Use case |
|--------|-----------|-------------|----------|
| `ScrollIndicator::kBig` | 232°–308° (76°) | 18° | Menus with few items (≤ 5) |
| `ScrollIndicator::kSmall` | 252°–288° (36°) | 10° | Menus with many items |

The default (applied by the Designer-generated base) is `kBig`. Call `setConfig(kSmall)` to switch.

Custom configs can be built from `ScrollIndicatorConfig`:

```cpp
struct ScrollIndicatorConfig {
    float    handleLen;    // Handle arc span (degrees)
    float    railMin;      // Rail low bound (degrees)
    float    railMax;      // Rail high bound (degrees)
    uint32_t handleColor;  // Handle color (0xRRGGBB)
    uint32_t railColor;    // Rail background color (0xRRGGBB)
};
```

## API

```cpp
void setConfig(const ScrollIndicatorConfig& cfg);  // apply config (optional, default = kBig)

void setCount(uint16_t count);       // total items; handle hidden when count <= 1
void setActiveId(uint16_t index);    // instant jump to item
void animateToId(int16_t index,
                 int16_t animationSteps = 0);  // animated move; 0 = instant
uint16_t getActiveId();              // current (or target) index
```

Wrap-around values for `animateToId`: passing `-1` wraps to the last item; passing `count` wraps to item 0.

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `ScrollIndicator.tpkg`

2. **Add to screen**: drag the `ScrollIndicator` container onto your screen. Position at X=0, Y=47.

3. **No designer properties** — all configuration is done in C++.

## Usage in C++ Code

```cpp
#include <gui/containers/ScrollIndicator.hpp>

ScrollIndicator scrollIndicator;
add(scrollIndicator);
scrollIndicator.setPosition(0, 47, 36, 146);

// Setup (kBig is default; only call setConfig to switch or customize)
scrollIndicator.setConfig(ScrollIndicator::kSmall);
scrollIndicator.setCount(5);     // 5-item list, starts at index 0
scrollIndicator.setActiveId(0);

// Navigate — animated (8 ticks ≈ smooth on 60 fps)
scrollIndicator.animateToId(nextIndex, 8);

// Navigate — instant
scrollIndicator.setActiveId(2);

// Wrap-around (last -> first)
scrollIndicator.animateToId(scrollIndicator.getActiveId() + 1, 8);
// If activeId was the last item, animateToId wraps back to 0 automatically.
```

## Notes

- The handle is automatically hidden when `count <= 1`.
- Starting a new animation while one is in progress snaps the handle to the position it has reached before proceeding — no visual glitch.
- The overflow handle (`handleOvf`) is managed internally and must not be manipulated directly.
- Handle arcs are clipped to the rail range. During a wrap-around slide the outgoing
  handle shrinks into the near rail end and the incoming one grows out of the far end,
  so neither is ever drawn past the rail into empty container space.
- That clipping is presentation only. The widget tracks the unclipped position
  internally, so interrupting a slide resumes from where the handle actually was
  rather than from the rail edge it was drawn at.
- The timer widget is automatically unregistered when the animation completes or in the destructor.

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/ScrollIndicator.hpp` — header
- `files/gui/src/containers/ScrollIndicator.cpp` — implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- No image assets — fully vector-based
