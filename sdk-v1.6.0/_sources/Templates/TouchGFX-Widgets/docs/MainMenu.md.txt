# MainMenu

## Overview

The MainMenu widget is a scroll-wheel menu container. It combines a `ScrollWheelWithSelectionStyle` (center item visually distinct from surrounding items) with a built-in `ScrollIndicator`. The screen provides item data through callbacks — MainMenu handles all wheel rendering, animation, and indicator synchronization.

This package replaces the older `Menu`, `MenuItemNotSelected`, and `MenuItemSelected` widgets and adds support for richer item styles, layout customization, and animation callbacks.

## Bundled Files

The package includes several interdependent classes:

| File | Role |
|------|------|
| `MainMenu` | Top-level container (Designer custom container) |
| `MainMenuItem` | Surrounding (non-selected) wheel item |
| `MainMenuCenterItem` | Center (selected) wheel item |
| `MenuItemConfig` | Data struct describing a single item |
| `MenuItemRenderer` | Stateless rendering helpers (header-only) |
| `ScrollIndicator` | Arc position indicator (managed internally) |
| `Toggle` | Toggle switch used by `TOGGLE` style |

## Components (Designer)

| Name | Type | Size |
|------|------|------|
| `wheel` | ScrollWheelWithSelectionStyle | 240×132 px |
| `scrollIndicator` | ScrollIndicator instance | 36×146 px |

## Item Styles

Each item is configured via `MenuItemConfig::Style`:

| Style | Center item | Surrounding item | Description |
|-------|-------------|-----------------|-------------|
| `SIMPLE` | ✓ | ✓ | Centered label text only |
| `TOGGLE` | ✓ | — (degrades to SIMPLE) | Label + interactive `Toggle` switch |
| `TIP` | ✓ | ✓ | Label + smaller hint text below |
| `INLINE` | ✓ | ✓ | Label + hint on same line, baseline-aligned |
| `ICON` | ✓ | ✓ | Static bitmap + label |
| `ICON_EXT` | ✓ | — (falls back to ICON/SIMPLE) | Dynamic raw-pixel bitmap + label |

## MenuItemConfig

```cpp
struct MenuItemConfig {
    Style style = SIMPLE;

    // Main text (priority: msgId > msgText > msgChar)
    TypedTextId msgId     = TYPED_TEXT_INVALID;
    TypedTextId msgIdType = TYPED_TEXT_INVALID;  // font/alignment override
    const Unicode::UnicodeChar* msgText = nullptr;
    const char*                 msgChar = nullptr;

    // Hint text — TIP / INLINE styles
    TypedTextId tipId     = TYPED_TEXT_INVALID;
    TypedTextId tipIdType = TYPED_TEXT_INVALID;
    const Unicode::UnicodeChar* tipText  = nullptr;
    const char*                 tipChar  = nullptr;
    colortype   tipColor = Color::getColorFromRGB(192, 192, 192);

    // Icon — ICON / ICON_EXT styles
    BitmapId       iconId  = BITMAP_INVALID;
    const uint8_t* extIcon = nullptr;   // raw ABGR2222 pixels, 60×60 (ICON_EXT)

    // Toggle state — TOGGLE style
    bool toggleState = false;

    // Per-item layout overrides (optional)
    const CenterItemLayout* centerLayout = nullptr;
    const ItemLayout*       itemLayout   = nullptr;
};
```

## Layout Priority

Layout geometry (text positions, icon positions) is resolved in this order:

1. `MenuItemConfig::centerLayout` / `itemLayout` — per-item override
2. `MainMenu::setCenterItemLayout()` / `setItemLayout()` — menu-wide default
3. Built-in hardcoded defaults

## API

### Wheel size

```cpp
void    setNumberOfItems(int16_t count);  // configures wheel + ScrollIndicator
int16_t getNumberOfItems();
```

### Navigation

```cpp
void     selectNext();                   // animated, fires middle + ended callbacks
void     selectPrev();                   // animated, fires middle + ended callbacks
void     selectItem(int16_t index);      // instant jump, no animation callbacks
uint16_t getSelectedItem();
```

### Item callbacks

```cpp
// Called for each surrounding item during scroll:
void setUpdateItemCallback(GenericCallback<MainMenuItem&, int16_t>& cb);

// Called for the center item during scroll:
void setUpdateCenterItemCallback(GenericCallback<MainMenuCenterItem&, int16_t>& cb);
```

Inside each callback, call `item.apply(cfg)` to render the item.

### Animation callbacks

```cpp
// Fires at the midpoint of selectNext/selectPrev animation (not fired by selectItem):
void setAnimationMiddleCallback(GenericCallback<int16_t>& cb);

// Fires when animation completes:
void setAnimationEndedCallback(GenericCallback<int16_t>& cb);

void setAnimationSteps(int16_t steps);  // default: 4
```

### Layout

```cpp
void setCenterItemLayout(const CenterItemLayout& layout);
void setItemLayout(const ItemLayout& layout);
```

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `MainMenu.tpkg`

2. **Add to screen**: drag the `MainMenu` container onto your screen.

3. **No designer properties** — all configuration is done in C++.

## Usage in C++ Code

```cpp
#include <gui/containers/MainMenu.hpp>
#include <gui/containers/MenuItemConfig.hpp>

// In the View header:
MainMenu mainMenu;
touchgfx::Callback<MyView, MainMenuItem&, int16_t>       mItemCb;
touchgfx::Callback<MyView, MainMenuCenterItem&, int16_t> mCenterCb;
touchgfx::Callback<MyView, int16_t>                      mAnimMiddleCb;
touchgfx::Callback<MyView, int16_t>                      mAnimEndedCb;

// In setupScreen():
mItemCb.setHandler(*this,       &MyView::onUpdateItem);
mCenterCb.setHandler(*this,     &MyView::onUpdateCenterItem);
mAnimMiddleCb.setHandler(*this, &MyView::onAnimMiddle);
mAnimEndedCb.setHandler(*this,  &MyView::onAnimEnded);

mainMenu.setUpdateItemCallback(mItemCb);
mainMenu.setUpdateCenterItemCallback(mCenterCb);
mainMenu.setAnimationMiddleCallback(mAnimMiddleCb);
mainMenu.setAnimationEndedCallback(mAnimEndedCb);
mainMenu.setNumberOfItems(5);

// Item callback:
void MyView::onUpdateItem(MainMenuItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style  = MenuItemConfig::TIP;
    cfg.msgId  = myItems[index].titleId;
    cfg.tipChar = myItems[index].subtitle;
    item.apply(cfg);
}

// Center item callback:
void MyView::onUpdateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    MenuItemConfig cfg;
    cfg.style       = MenuItemConfig::TOGGLE;
    cfg.msgId       = myItems[index].titleId;
    cfg.toggleState = mySettings[index];
    item.apply(cfg);
}

// Navigate:
mainMenu.selectNext();
mainMenu.selectPrev();
mainMenu.selectItem(2);  // instant, no animation
uint16_t current = mainMenu.getSelectedItem();
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `content/texts.xml` — text database
- `files/gui/include/gui/containers/MainMenu.hpp` — top-level container header
- `files/gui/src/containers/MainMenu.cpp`
- `files/gui/include/gui/containers/MainMenuItem.hpp`
- `files/gui/src/containers/MainMenuItem.cpp`
- `files/gui/include/gui/containers/MainMenuCenterItem.hpp`
- `files/gui/src/containers/MainMenuCenterItem.cpp`
- `files/gui/include/gui/containers/MenuItemConfig.hpp` — data structs (header-only)
- `files/gui/include/gui/containers/MenuItemRenderer.hpp` — rendering helpers (header-only)
- `files/gui/include/gui/containers/ScrollIndicator.hpp` + `.cpp` — bundled dependency
- `files/gui/include/gui/containers/Toggle.hpp` + `.cpp` — bundled dependency
- `files/assets/fonts/` — Poppins Regular, Medium, SemiBold, Italic

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- Poppins font family (bundled in package)
- `BitmapDatabase.hpp` — for static icon bitmap IDs (`ICON` style)
- `TextKeysAndLanguages.hpp` — for typed text IDs
