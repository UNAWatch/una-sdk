# Menu

## Overview

The Menu widget is a custom TouchGFX container that provides a scrollable menu interface with animated selection. It combines a scroll wheel, side bar indicator, title, background image, and info message to create a comprehensive menu system for embedded applications.

## Purpose

The Menu widget serves as a user interface component for displaying and selecting from a list of menu items. It supports up to 10 items with distinct visual states for selected and unselected items, making it ideal for navigation menus in wearable devices, smart appliances, or any application requiring menu-based interaction.

## Components

The widget consists of the following key components:

- **ScrollWheelWithSelectionStyle**: The main scrollable list that displays menu items
- **SideBar**: A visual indicator showing the current position in the menu
- **Title**: Text display for the menu title
- **Background**: Image background for the menu
- **Info Message**: Additional text information with configurable color
- **Buttons**: Button controls (L1, L2, R1, R2) with default states
- **MenuItemSelected/MenuItemNotSelected**: Arrays of item widgets for selected and unselected states

## Functionality

### Core Features

- **Scrollable Menu**: Supports up to 10 menu items with smooth scrolling animation
- **Visual Selection**: Distinct appearance for selected vs unselected items
- **Side Bar Synchronization**: Side bar animates in sync with the scroll wheel
- **Configurable Elements**: Title, background, and info message can be shown/hidden and customized
- **Button Integration**: Pre-configured button states for navigation
- **Animation Callbacks**: Support for animation end notifications

### Key Methods

- `setNumberOfItems(int16_t)`: Set the number of menu items (max 10)
- `selectNext()` / `selectPrev()`: Navigate to next/previous item with animation
- `selectItem(int16_t)`: Jump to specific item
- `setTitle(TypedTextId)` / `showTitle(bool)`: Control title display
- `setBackground(BitmapId)` / `showBackground(bool)`: Control background image
- `setInfoMsg(TypedTextId)` / `setInfoMsgColor(colortype)`: Control info message

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Import the Menu custom container package

2. **Add to Screen**:
   - Drag the Menu widget onto your screen
   - Position and resize as needed

3. **Configure Properties**:
   - The widget has no Designer-configurable properties
   - All configuration is done programmatically

## Usage in C++ Code

### Basic Setup

```cpp
#include <gui/containers/Menu.hpp>

Menu menu;

void setupScreen() {
    menu.initialize();
    add(menu);
    
    // Set number of items
    menu.setNumberOfItems(5);
    
    // Set title
    menu.setTitle(T_MENU_TITLE);
    menu.showTitle(true);
    
    // Set background
    menu.setBackground(BITMAP_MENU_BG);
    menu.showBackground(true);
}
```

### Navigation

```cpp
// Navigate through menu
menu.selectNext();  // Go to next item
menu.selectPrev();  // Go to previous item
menu.selectItem(2); // Jump to item 3

// Get current selection
uint16_t current = menu.getSelectedItem();
```

### Customization

```cpp
// Set info message
menu.setInfoMsg(T_INFO_TEXT);
menu.setInfoMsgColor(Color::getColorFromRGB(255, 255, 255));

// Access buttons
Buttons &buttons = menu.getButtons();
buttons.setL1(Buttons::GREEN);

// Access wheel and items
ScrollWheelWithSelectionStyle &wheel = menu.getWheel();
MenuItemSelected *selectedItem = menu.getSelectedItem(0);
MenuItemNotSelected *unselectedItem = menu.getNotSelectedItem(0);
```

## Adding Actions and Event Handling

To make the menu interactive and execute actions based on user selections, you need to handle button events and animation callbacks.

### Handling Button Events

The menu includes pre-configured buttons (L1, L2, R1, R2) that can be used to trigger actions. Typically, R1 can be used as a "select" or "confirm" button.

```cpp
// In your view or presenter, set up button callbacks
void MyView::setupMenuActions() {
    Buttons &buttons = menu.getButtons();

    // Set R1 button to green (ready state)
    buttons.setR1(Buttons::GREEN);

    // Add callback for R1 button press
    buttons.setR1Callback(touchgfx::Callback<MyView>(this, &MyView::handleMenuSelect));
}

// Callback function to handle menu selection
void MyView::handleMenuSelect() {
    uint16_t selectedItem = menu.getSelectedItem();

    // Execute action based on selected item
    switch (selectedItem) {
        case 0:
            // Action for first item
            presenter->gotoScreen1();
            break;
        case 1:
            // Action for second item
            presenter->gotoScreen2();
            break;
        // Add cases for other items
        default:
            break;
    }
}
```

### Navigation Handling

Navigation can be handled through button presses or other input methods:

```cpp
void MyView::handleL1Press() {
    menu.selectPrev();
}

void MyView::handleL2Press() {
    menu.selectNext();
}
```

### Animation Callbacks

The menu provides an animation end callback that can be overridden to perform actions after selection animations complete:

```cpp
// Override the animation end callback in your Menu class
void Menu::scrollWheelAnimationEndCb() {
    // Call base implementation if needed
    MenuBase::scrollWheelAnimationEndCb();

    // Perform actions after animation completes
    uint16_t selectedItem = getSelectedItem();

    // Optional: Update UI or trigger effects
    updateInfoMessageForSelection(selectedItem);

    // Note: For action execution, prefer button callbacks over animation callbacks
    // to ensure user intent is clear
}
```

### Complete Example

Here's a complete example showing how to set up a functional menu with actions:

```cpp
class MyView : public MyViewBase {
public:
    void setupScreen() {
        MyViewBase::setupScreen();

        // Basic menu setup
        menu.initialize();
        add(menu);
        menu.setNumberOfItems(3);
        menu.setTitle(T_MENU_TITLE);

        // Set up button callbacks
        setupMenuActions();
    }

private:
    void setupMenuActions() {
        Buttons &buttons = menu.getButtons();
        buttons.setR1(Buttons::GREEN);
        buttons.setR1Callback(touchgfx::Callback<MyView>(this, &MyView::handleSelect));

        buttons.setL1(Buttons::WHITE);
        buttons.setL1Callback(touchgfx::Callback<MyView>(this, &MyView::handlePrev));

        buttons.setL2(Buttons::WHITE);
        buttons.setL2Callback(touchgfx::Callback<MyView>(this, &MyView::handleNext));
    }

    void handleSelect() {
        uint16_t item = menu.getSelectedItem();
        switch (item) {
            case 0: presenter->showSettings(); break;
            case 1: presenter->showProfile(); break;
            case 2: presenter->showAbout(); break;
        }
    }

    void handlePrev() { menu.selectPrev(); }
    void handleNext() { menu.selectNext(); }
};
```

This approach ensures the menu is fully interactive, with clear user feedback and proper action execution based on selections.

## Advanced Features and Hidden Gems

- **Synchronized Animations**: Side bar and scroll wheel animate together using configurable animation steps from `Gui::Config::kMenuAnimationSteps`
- **Item Pool Management**: Pre-allocated arrays of selected/unselected items for efficient memory usage
- **Animation Callbacks**: Empty callback method `scrollWheelAnimationEndCb()` available for custom animation end handling
- **Button Defaults**: Pre-configured button states (L1/NONE, L2/NONE, R1/AMBER, R2/WHITE) for consistent UI
- **Flexible Item Access**: Direct access to individual item widgets for advanced customization
- **Configurable Limits**: Maximum 10 items with compile-time constant for easy adjustment

## Technical Details

- **Base Class**: Inherits from `MenuBase` (generated by TouchGFX Designer)
- **Minimum TouchGFX Version**: 4.26.0
- **Maximum Items**: 10 (configurable via `skMaxNumberOfItems`)
- **Animation**: Uses TouchGFX animation system with configurable steps
- **Dependencies**: Requires `MenuItemSelected` and `MenuItemNotSelected` custom containers

## Best Practices

1. **Item Limit**: Respect the 10-item limit to avoid array bounds issues
2. **Animation Steps**: Configure `Gui::Config::kMenuAnimationSteps` for smooth performance
3. **Item Initialization**: Use `getSelectedItem()` and `getNotSelectedItem()` for custom item setup
4. **Button Handling**: Leverage the integrated button system for navigation
5. **Memory Management**: Items are pre-allocated, so no dynamic allocation overhead

## Limitations

- Fixed maximum of 10 items
- Requires companion `MenuItemSelected` and `MenuItemNotSelected` widgets
- No built-in touch handling (navigation via code or buttons)
- Animation end callback is empty by default

This is a comprehensive menu widget suitable for applications requiring animated, scrollable menu interfaces with rich visual feedback.