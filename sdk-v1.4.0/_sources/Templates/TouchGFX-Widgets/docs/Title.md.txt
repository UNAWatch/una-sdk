# Title

## Overview

The Title widget displays a screen heading as a single-line text with a decorative horizontal underline. It supports setting the text from a typed-text ID, a Unicode buffer, or a plain C-string, and allows changing the accent color at runtime.

## Components

| Name | Type | Description |
|------|------|-------------|
| `titleText` | TextAreaWithOneWildcard | Heading text, Poppins Italic 18 px, center-aligned |
| `line` | Line (round cap) | Decorative underline at Y=38 |

Container size: **140×41 px**, positioned at X=50 (centered on a 240 px screen).

Default color for both text and line: `#404040` (dark gray). Typically changed to a contextual accent color via `setColor()`.

Text buffer size: 16 characters (15 usable + null terminator).

## API

```cpp
void set(TypedTextId msgId);                           // from localised text ID
void set(touchgfx::Unicode::UnicodeChar* messageText); // from Unicode buffer
void set(const char* messageText);                     // from UTF-8 / ASCII string

void setColor(touchgfx::colortype color);              // line + text color
```

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Title.tpkg`

2. **Add to screen**: drag the `Title` container onto your screen. Default position is X=50, Y=0.

3. **No designer properties** — text and color are set programmatically.

## Usage in C++ Code

```cpp
#include <gui/containers/Title.hpp>

Title title;
add(title);
title.setPosition(50, 0, 140, 41);

// Set from a typed text ID
title.set(T_MY_SCREEN_TITLE);

// Set from a C-string (e.g. dynamic content)
title.set("Lap 3");

// Change accent color
title.setColor(touchgfx::Color::getColorFromRGB(0, 128, 128));
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `content/texts.xml` — text database (uses `TMP_ITALIC_18`)
- `files/gui/include/gui/containers/Title.hpp` — header
- `files/gui/src/containers/Title.cpp` — implementation
- `files/assets/fonts/Poppins-Regular.ttf`, `Poppins-Italic.ttf`

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- Poppins Italic font (bundled in package)
- `TMP_ITALIC_18` text ID (provided by the `Texts` widget or defined manually)
