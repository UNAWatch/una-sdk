# MenuItemNotSelected

## Overview

The MenuItemNotSelected widget is a custom TouchGFX container designed to display menu items in their unselected state. It provides flexible text display options with support for simple centered text or text with an additional tip/subtitle, making it ideal for use in scrollable menu systems.

## Purpose

This widget serves as the visual representation for menu items that are not currently selected. It supports two display styles: a simple centered text layout and a more detailed layout with main text and a tip text below. This allows for rich menu item presentation while maintaining a clean, unselected appearance.

## Components

The widget consists of the following visual elements:

- **Main Text (text)**: The primary text display, centered in simple mode
- **Tip Text (textTip)**: Optional secondary text displayed below the main text in tip mode
- **Text Buffers**: Internal buffers for storing text content (textBuffer, textTipBuffer)

## Functionality

### Display Styles

- **Simple Style**: Displays centered main text only
- **Tip Style**: Displays main text with tip text below

### Configuration Methods

- `config(TypedTextId msgId)`: Set main text using typed text ID (simple style)
- `config(UnicodeChar *msg)`: Set main text using unicode string (simple style)
- `configTip(TypedTextId msgId, TypedTextId msgIdTip)`: Set main and tip text using typed text IDs
- `configTip(TypedTextId msgId, UnicodeChar *msgTip)`: Set main text ID and tip string

### Customization

- `setMsgTypedTextId(TypedTextId)`: Change font for main text
- `setTipTypedTextId(TypedTextId)`: Change font for tip text
- `setTipColor(colortype)`: Set color for tip text

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Import the MenuItemNotSelected package into your TouchGFX project

2. **Add to Screen**:
   - The widget is typically used within scroll wheels or lists, not directly on screens
   - It's instantiated programmatically by parent containers like Menu

3. **Configuration**:
   - All configuration is done programmatically

## Usage in C++ Code

### Basic Configuration

```cpp
#include <gui/containers/MenuItemNotSelected.hpp>

MenuItemNotSelected item;

// Simple text
item.config(T_MENU_ITEM_1);

// Text with tip
item.configTip(T_MENU_ITEM_1, T_MENU_TIP_1);

// Custom fonts
item.setMsgTypedTextId(T_CUSTOM_FONT);
item.setTipTypedTextId(T_CUSTOM_TIP_FONT);
item.setTipColor(Color::getColorFromRGB(128, 128, 128));

// Update display
item.updStyle();
```

### Integration with Menu

```cpp
// In Menu widget, items are managed automatically
MenuItemNotSelected *item = menu.getNotSelectedItem(index);
item->configTip(T_ITEM_TEXT, T_ITEM_TIP);
```

### Copying Items

```cpp
MenuItemNotSelected item1, item2;
item1.configTip(T_TEXT, T_TIP);
// Copy configuration
item2 = item1;
```

## Advanced Features and Hidden Gems

- **Dual Input Support**: Accepts both typed text IDs and direct unicode strings for maximum flexibility
- **Font Customization**: Separate font control for main text and tip text
- **Color Control**: Independent color setting for tip text
- **Buffer Management**: Internal text buffers with defined sizes (TEXT_SIZE, TEXTTIP_SIZE)
- **Copy Operator**: Full configuration copying via assignment operator
- **Default Fonts**: Automatic fallback to default fonts (T_TMP_MEDIUM_18, T_TMP_ITALIC_18) if not specified

## Technical Details

- **Base Class**: Inherits from `MenuItemNotSelectedBase`
- **Minimum TouchGFX Version**: 4.26.0
- **Text Buffers**: Pre-allocated buffers for text storage
- **Style Management**: Internal style enum for display modes
- **Invalidation**: Automatic invalidation after style updates

## Best Practices

1. **Text Length**: Ensure text fits within buffer sizes
2. **Font Consistency**: Use consistent fonts across menu items
3. **Color Usage**: Use tip color to provide visual hierarchy
4. **Performance**: Call updStyle() after configuration changes
5. **Memory**: Buffers are statically allocated, no dynamic memory

## Limitations

- Fixed buffer sizes for text
- No built-in touch handling
- Designed for use within scroll wheels/lists
- Requires manual style updates after configuration

This widget provides the foundation for unselected menu item display in complex menu systems, offering flexibility in text presentation while maintaining efficient rendering.