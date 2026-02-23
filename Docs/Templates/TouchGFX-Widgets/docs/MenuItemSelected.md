# MenuItemSelected

## Overview

The MenuItemSelected widget is a custom TouchGFX container designed to display menu items in their selected state. It provides flexible display options including simple centered text, text with an integrated toggle switch, and text with an additional tip/subtitle, making it perfect for interactive menu systems where selection state needs visual distinction.

## Purpose

This widget serves as the visual representation for menu items that are currently selected. It supports three display styles: a simple centered text layout, a layout with text and a toggle switch, and a layout with main text and tip text. This allows for rich, interactive menu item presentation with clear visual feedback for the selected state.

## Components

The widget consists of the following visual elements:

- **Main Text (text)**: The primary text display, positioned according to the selected style
- **Tip Text (textTip)**: Optional secondary text displayed below the main text in tip mode
- **Toggle Switch (toggleSwitch)**: An integrated toggle control for interactive menu items
- **Text Buffers**: Internal buffers for storing text content (textBuffer, textTipBuffer)

## Functionality

### Display Styles

- **Simple Style**: Displays centered main text only
- **Toggle Style**: Displays main text with an integrated toggle switch on the right
- **Tip Style**: Displays main text with tip text below

### Configuration Methods

- `config(TypedTextId msgId)`: Set main text using typed text ID (simple style)
- `config(UnicodeChar *msg)`: Set main text using unicode string (simple style)
- `configToggle(TypedTextId msgId)`: Set main text using typed text ID (toggle style)
- `configToggle(UnicodeChar *msg)`: Set main text using unicode string (toggle style)
- `configTip(TypedTextId msgId, TypedTextId msgIdTip)`: Set main and tip text using typed text IDs
- `configTip(TypedTextId msgId, UnicodeChar *msgTip)`: Set main text ID and tip string

### Toggle Control

- `setToggle(bool state)`: Set the toggle switch state
- `getToggle()`: Get the current toggle switch state

### Customization

- `setMsgTypedTextId(TypedTextId)`: Change font for main text
- `setTipTypedTextId(TypedTextId)`: Change font for tip text

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Import the MenuItemSelected package into your TouchGFX project

2. **Add to Screen**:
   - The widget is typically used within scroll wheels or lists, not directly on screens
   - It's instantiated programmatically by parent containers like Menu

3. **Configuration**:
   - All configuration is done programmatically

## Usage in C++ Code

### Basic Configuration

```cpp
#include <gui/containers/MenuItemSelected.hpp>

MenuItemSelected item;

// Simple text
item.config(T_MENU_ITEM_1);

// Text with toggle
item.configToggle(T_MENU_ITEM_2);
item.setToggle(true);

// Text with tip
item.configTip(T_MENU_ITEM_3, T_MENU_TIP_3);

// Custom fonts
item.setMsgTypedTextId(T_CUSTOM_FONT);
item.setTipTypedTextId(T_CUSTOM_TIP_FONT);

// Update display
item.updStyle();
```

### Integration with Menu

```cpp
// In Menu widget, items are managed automatically
MenuItemSelected *item = menu.getSelectedItem(index);
item->configToggle(T_ITEM_TEXT);
item->setToggle(userPreference);
```

### Toggle Interaction

```cpp
MenuItemSelected item;
item.configToggle(T_SETTING_TEXT);

// Set initial state
item.setToggle(false);

// Check state
bool currentState = item.getToggle();

// Toggle programmatically
item.setToggle(!item.getToggle());
```

### Copying Items

```cpp
MenuItemSelected item1, item2;
item1.configToggle(T_TEXT);
item1.setToggle(true);
// Copy configuration and state
item2 = item1;
```

## Advanced Features and Hidden Gems

- **Toggle Integration**: Built-in toggle switch for settings-style menu items
- **Flexible Positioning**: Automatic layout adjustment based on style (centered, left-aligned with toggle, top-aligned with tip)
- **Dual Input Support**: Accepts both typed text IDs and direct unicode strings for maximum flexibility
- **Font Customization**: Separate font control for main text and tip text
- **State Preservation**: Toggle state maintained during configuration changes
- **Copy Operator**: Full configuration and state copying via assignment operator
- **Default Fonts**: Automatic fallback to default fonts (T_TMP_SEMIBOLD_30, T_TMP_ITALIC_18) if not specified
- **Buffer Management**: Internal text buffers with defined sizes (TEXT_SIZE, TEXTTIP_SIZE)

## Technical Details

- **Base Class**: Inherits from `MenuItemSelectedBase`
- **Minimum TouchGFX Version**: 4.26.0
- **Text Buffers**: Pre-allocated buffers for text storage
- **Style Management**: Internal style enum for display modes (STYLE_SIMPLE, STYLE_TOGGLE, STYLE_TIP)
- **Invalidation**: Automatic invalidation after style updates
- **Toggle Component**: Uses integrated Toggle widget for switch functionality

## Best Practices

1. **Text Length**: Ensure text fits within buffer sizes and widget dimensions
2. **Font Consistency**: Use consistent fonts across menu items for visual coherence
3. **Toggle Usage**: Use toggle style for binary settings or options
4. **State Management**: Preserve toggle states when updating item configuration
5. **Performance**: Call updStyle() after configuration changes
6. **Memory**: Buffers are statically allocated, no dynamic memory allocation

## Limitations

- Fixed buffer sizes for text
- No built-in touch handling for the widget itself (toggle handles its own interaction)
- Designed for use within scroll wheels/lists
- Requires manual style updates after configuration
- Toggle positioning is fixed (right side of text)

This widget provides the foundation for selected menu item display in interactive menu systems, offering flexibility in text presentation and built-in toggle functionality while maintaining efficient rendering.