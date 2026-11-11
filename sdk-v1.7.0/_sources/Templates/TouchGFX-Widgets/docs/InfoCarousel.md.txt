# InfoCarousel

## Overview

The InfoCarousel widget displays a label and a data value, and automatically cycles through multiple data slots at a configurable interval. It registers itself as a TouchGFX timer widget only when more than one slot is active, keeping overhead minimal for static use.

## Components

| Name | Type | Description |
|------|------|-------------|
| `titleText` | TextAreaWithOneWildcard | Slot label, Poppins Italic 18 px, center-aligned |
| `dataValue` | TextAreaWithOneWildcard | Slot value, Poppins SemiBold 30 px, center-aligned |
| `line` | Line (round cap) | Decorative underline at Y=65 |

Container size: **160×87 px**, positioned at X=40 (centered on a 240 px screen).

## API

### Cycling control

```cpp
void setCount(uint16_t count);
// Set number of slots. Resets index to 0, fires callback immediately.
// Timer registered only when count > 1.

void setPeriod(uint16_t ticks);
// Ticks between slot advances (default: 1 tick). Call before setCount().

void setUpdateCallback(touchgfx::GenericCallback<int16_t>& cb);
// Register callback (signature: void fn(int16_t index)). Call before setCount().

void refresh();
// Re-fire the callback for the current index (use when underlying data changed).
```

### Content (call from inside the callback)

```cpp
void setTitle(TypedTextId msgId);
// Set the label text from a typed-text ID.

void setValue(const touchgfx::Unicode::UnicodeChar* messageText);
// Set the value text. Pass nullptr to hide the dataValue field.
```

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `InfoCarousel.tpkg`

2. **Add to screen**: drag the `InfoCarousel` container onto your screen. Default position is X=40.

3. **No designer properties** — all behavior is configured in C++.

## Usage in C++ Code

```cpp
#include <gui/containers/InfoCarousel.hpp>

// In the View header:
InfoCarousel infoCarousel;
touchgfx::Callback<MyView, int16_t> mCarouselCb;

// In setupScreen():
mCarouselCb.setHandler(*this, &MyView::onCarouselUpdate);

infoCarousel.setPeriod(3000);                  // 3000 ticks between advances
infoCarousel.setUpdateCallback(mCarouselCb);
infoCarousel.setCount(3);                      // 3 slots; fires cb(0) immediately

// Callback:
void MyView::onCarouselUpdate(int16_t index)
{
    switch (index) {
        case 0:
            infoCarousel.setTitle(T_LABEL_DISTANCE);
            infoCarousel.setValue(distanceBuffer);
            break;
        case 1:
            infoCarousel.setTitle(T_LABEL_PACE);
            infoCarousel.setValue(paceBuffer);
            break;
        case 2:
            infoCarousel.setTitle(T_LABEL_CALORIES);
            infoCarousel.setValue(nullptr);    // hide value field
            break;
    }
}

// Force refresh when data updates:
infoCarousel.refresh();
```

## Notes

- `setUpdateCallback()` and `setPeriod()` must be called **before** `setCount()`.
- The timer widget is automatically unregistered in the destructor — no manual cleanup required.
- With `count = 1` the widget behaves as a static label/value pair (no timer).

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `content/texts.xml` — text database (uses `TMP_ITALIC_18`, `TMP_SEMIBOLD_30`)
- `files/gui/include/gui/containers/InfoCarousel.hpp` — header
- `files/gui/src/containers/InfoCarousel.cpp` — implementation
- `files/assets/fonts/Poppins-Regular.ttf`, `Poppins-SemiBold.ttf`, `Poppins-Italic.ttf`

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- Poppins Italic and SemiBold fonts (bundled in package)
- `TMP_ITALIC_18`, `TMP_SEMIBOLD_30` text IDs (provided by the `Texts` widget or defined manually)
