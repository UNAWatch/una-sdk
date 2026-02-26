# Scroll Menu Tutorial

This tutorial demonstrates how to create a scrollable menu with interactive buttons and counter functionality using TouchGFX. The application features a circular scroll menu with three items: Counter (to display the current count), Increase (to increment the counter), and Decrease (to decrement the counter).

## Project Setup

### Prerequisites
- TouchGFX Designer or TouchGFX CLI
- A TouchGFX project with basic screen setup
- Understanding of TouchGFX MVP (Model-View-Presenter) architecture

### Project Structure
The project follows the standard TouchGFX structure:
```
TouchGFX-GUI/
├── gui/
│   ├── include/gui/containers/Menu.hpp
│   ├── include/gui/main_screen/MainView.hpp
│   └── src/
│       ├── containers/Menu.cpp
│       └── main_screen/MainView.cpp
├── generated/
│   └── gui_generated/
│       └── src/main_screen/MainViewBase.cpp
└── assets/texts/texts.xml
```

## Adding the Menu Widget

### 1. Create the Menu Container

First, create a custom container class for the menu functionality. The `Menu` class extends `MenuBase` and provides methods for managing menu items, selection, and animation.

**`gui/include/gui/containers/Menu.hpp`**:
```cpp
#ifndef MENU_HPP
#define MENU_HPP

#include <gui_generated/containers/MenuBase.hpp>

class Menu : public MenuBase
{
public:
    Menu();
    virtual ~Menu() {}

    virtual void initialize();

    static const uint16_t skMaxNumberOfItems = 10;

    virtual void invalidate();

    void setNumberOfItems(int16_t numberOfItems);
    int16_t getNumberOfItems();

    void setTitle(TypedTextId msgId);
    void showTitle(bool state);

    void setBackground(BitmapId iconId);
    void showBackground(bool state);

    void setInfoMsg(TypedTextId msgId);
    void setInfoMsg(const char* msg);
    void setInfoMsgColor(touchgfx::colortype color);

    Buttons &getButtons();

    void selectNext();
    void selectPrev();
    void selectItem(int16_t itemIndex);
    uint16_t getSelectedItem();

    ScrollWheelWithSelectionStyle &getWheel();
    MenuItemSelected *getSelectedItem(int16_t itemIndex);
    MenuItemNotSelected *getNotSelectedItem(int16_t itemIndex);

protected:
    virtual void wheelUpdateItem(MenuItemNotSelected &item, int16_t itemIndex) override;
    virtual void wheelUpdateCenterItem(MenuItemSelected &item, int16_t itemIndex) override;

    touchgfx::Callback <Menu> mAnimationEndCb;
    void scrollWheelAnimationEndCb();

    MenuItemSelected mSelItems[skMaxNumberOfItems] { };
    MenuItemNotSelected mNotSelItems[skMaxNumberOfItems] { };
};

#endif // MENU_HPP
```

### 2. Implement the Menu Container

**`gui/src/containers/Menu.cpp`**:
```cpp
#include <gui/containers/Menu.hpp>

Menu::Menu() :
    mAnimationEndCb(this, &Menu::scrollWheelAnimationEndCb)
{

}

void Menu::initialize()
{
    MenuBase::initialize();

    // Default buttons state
    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::AMBER);
    buttons.setR2(Buttons::WHITE);

    for (int i = 0; i < skMaxNumberOfItems; i++) {
        mSelItems[i].initialize();
        mNotSelItems[i].initialize();
    }

    wheel.setAnimationEndedCallback(mAnimationEndCb);
}

void Menu::invalidate()
{
    for (int i = 0; i < wheel.getNumberOfItems(); i++) {
        wheel.itemChanged(i);
    }
    wheel.invalidate();
}

void Menu::setNumberOfItems(int16_t numberOfItems)
{
    if (numberOfItems <= skMaxNumberOfItems) {
        wheel.setNumberOfItems(numberOfItems);
        sideBar.setCount(numberOfItems);
    }
}

int16_t Menu::getNumberOfItems()
{
    return wheel.getNumberOfItems();
}

void Menu::setTitle(TypedTextId msgId)
{
    title.set(msgId);
    title.invalidate();
}

void Menu::showTitle(bool state)
{
    title.setVisible(state);
    title.invalidate();
}

void Menu::setBackground(BitmapId iconId)
{
    backgroung.setBitmap(touchgfx::Bitmap(iconId));
    backgroung.invalidate();
}

void Menu::showBackground(bool state)
{
    backgroung.setVisible(state);
    backgroung.invalidate();
}

void Menu::setInfoMsg(TypedTextId msgId)
{
    if (msgId != TYPED_TEXT_INVALID) {
        Unicode::snprintf(infoBuffer, INFO_SIZE, "%s", touchgfx::TypedText(msgId).getText());
        info.setVisible(true);
    } else {
        info.setVisible(false);
    }
    info.invalidate();
}

void Menu::setInfoMsg(const char* msg)
{
    if (msg != nullptr) {
        Unicode::snprintf(infoBuffer, INFO_SIZE, "%s", msg);
        info.setVisible(true);
    } else {
        info.setVisible(false);
    }
    info.invalidate();
}

void Menu::setInfoMsgColor(touchgfx::colortype color)
{
    info.setColor(color);
    info.invalidate();
}

Buttons &Menu::getButtons()
{
    return buttons;
}

void Menu::selectNext()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t p = wheel.getSelectedItem() + 1;
    wheel.animateToItem(p, Gui::Config::kMenuAnimationSteps);
    sideBar.animateToId(p, Gui::Config::kMenuAnimationSteps);
}

void Menu::selectPrev()
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    int16_t p = wheel.getSelectedItem() - 1;
    wheel.animateToItem(p, Gui::Config::kMenuAnimationSteps);
    sideBar.animateToId(p, Gui::Config::kMenuAnimationSteps);
}

void Menu::selectItem(int16_t itemIndex)
{
    if (wheel.getNumberOfItems() <= 1) {
        return;
    }

    wheel.animateToItem(itemIndex, 0);
    sideBar.setActiveId(wheel.getSelectedItem());
}

uint16_t Menu::getSelectedItem()
{
    return wheel.getSelectedItem();
}

ScrollWheelWithSelectionStyle &Menu::getWheel()
{
    return wheel;
}

MenuItemSelected *Menu::getSelectedItem(int16_t itemIndex)
{
    if (itemIndex < skMaxNumberOfItems) {
        return &mSelItems[itemIndex];
    }

    return nullptr;
}

MenuItemNotSelected *Menu::getNotSelectedItem(int16_t itemIndex)
{
    if (itemIndex < skMaxNumberOfItems) {
        return &mNotSelItems[itemIndex];
    }

    return nullptr;
}

void Menu::wheelUpdateItem(MenuItemNotSelected &item, int16_t itemIndex)
{
    item = mNotSelItems[itemIndex];
}

void Menu::wheelUpdateCenterItem(MenuItemSelected &item, int16_t itemIndex)
{
    item = mSelItems[itemIndex];
}

void Menu::scrollWheelAnimationEndCb()
{

}
```

## Configuring Menu Items

### 1. Define Text Resources

Add the required text keys to your text database. In `assets/texts/texts.xml`, add the following text entries:

```xml
<Text Id="T_COUNTER" Alignment="Center" TypographyId="Poppins_Medium_18">
    <Translation Language="GB">Counter</Translation>
</Text>
<Text Id="T_INCREASE" Alignment="Center" TypographyId="Poppins_Medium_18">
    <Translation Language="GB">Increase</Translation>
</Text>
<Text Id="T_DECREASE" Alignment="Center" TypographyId="Poppins_Medium_18">
    <Translation Language="GB">Decrease</Translation>
</Text>
```

### 2. Configure Menu Items in the View

In your main view class, configure the menu items during screen setup:

**`gui/include/gui/main_screen/MainView.hpp`**:
```cpp
#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

class MainView : public MainViewBase
{
    uint8_t lastKeyPressed = {'\0'};
    int counter = 0;
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MAINVIEW_HPP
```

**`gui/src/main_screen/MainView.cpp`**:
```cpp
#include <gui/main_screen/MainView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Unicode.hpp>

MainView::MainView()
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    menu1.setNumberOfItems(3);
    menu1.getNotSelectedItem(0)->config(T_COUNTER);
    menu1.getNotSelectedItem(1)->config(T_INCREASE);
    menu1.getNotSelectedItem(2)->config(T_DECREASE);
    menu1.invalidate();

    buttons.setL1(ButtonsSet::NONE);
    buttons.setL2(ButtonsSet::NONE);
    buttons.setR1(ButtonsSet::NONE);
    buttons.setR2(ButtonsSet::AMBER);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}
```

## Implementing Button Interactions

### 1. Handle Key Events

Implement the `handleKeyEvent` method to respond to button presses:

```cpp
void MainView::handleKeyEvent(uint8_t key)
{
    if (key == Gui::Config::Button::L1) {
        menu1.selectPrev();
    }

    if (key == Gui::Config::Button::L2) {
        menu1.selectNext();
    }

    if (key == Gui::Config::Button::R1) {
        int selected = menu1.getSelectedItem();
        if (selected == 0) {
            // Display counter
            char buffer[32];
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsg(buffer);
        } else if (selected == 1) {
            // Increment counter
            counter++;
            char buffer[32];
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsg(buffer);
        } else if (selected == 2) {
            // Decrement counter
            counter--;
            char buffer[32];
            touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
            menu1.setInfoMsg(buffer);
        }
    }

    if (key == Gui::Config::Button::R2) {
        if (lastKeyPressed == key) presenter->exit();
    }
    lastKeyPressed = key;
}
```

### 2. Button Mapping

The button interactions are mapped as follows:
- **L1 Button**: Navigate to previous menu item
- **L2 Button**: Navigate to next menu item
- **R1 Button**: Execute action for selected menu item
- **R2 Button**: Exit application (double press)

## Adding Counter Logic

### 1. Counter Variable

Add a counter variable to track the current count:

```cpp
class MainView : public MainViewBase
{
    uint8_t lastKeyPressed = {'\0'};
    int counter = 0;  // Counter variable
public:
    // ... rest of the class
};
```

### 2. Counter Operations

The counter logic is implemented in the `handleKeyEvent` method:

- **Counter Display**: When "Counter" is selected and R1 is pressed, display the current counter value
- **Increment**: When "Increase" is selected and R1 is pressed, increment the counter and display the new value
- **Decrement**: When "Decrease" is selected and R1 is pressed, decrement the counter and display the new value

### 3. Display Counter Value

Use `setInfoMsg` to display the counter value in the menu's info area:

```cpp
char buffer[32];
touchgfx::Unicode::snprintf(buffer, 32, "%d", counter);
menu1.setInfoMsg(buffer);
```

## Testing the Application

### 1. Build and Run

1. Generate code using TouchGFX Designer or CLI
2. Build the project for your target platform
3. Run the application on simulator or hardware

### 2. Test Navigation

- Press L1/L2 buttons to navigate between menu items
- Verify that the selected item is highlighted and centered

### 3. Test Counter Functionality

1. Select "Counter" item and press R1 - should display current counter value (initially 0)
2. Select "Increase" item and press R1 - should increment counter and display new value
3. Select "Decrease" item and press R1 - should decrement counter and display new value
4. Repeat steps 1-3 to verify counter operations work correctly

### 4. Test Exit Functionality

- Press R2 twice quickly to exit the application

### 5. Edge Cases

- Test navigation when only one item is selected
- Test counter overflow/underflow (if applicable)
- Test rapid button presses

## Code Changes Summary

### Files Modified:
1. **`gui/include/gui/containers/Menu.hpp`** - Added Menu class declaration
2. **`gui/src/containers/Menu.cpp`** - Implemented Menu class functionality
3. **`gui/include/gui/main_screen/MainView.hpp`** - Added counter variable
4. **`gui/src/main_screen/MainView.cpp`** - Implemented menu setup and button handling
5. **`assets/texts/texts.xml`** - Added text resources for menu items

### Key Implementation Points:
- Menu uses ScrollWheelWithSelectionStyle for smooth scrolling
- Button interactions handle navigation and actions
- Counter state is maintained in the View class
- Info messages display counter values and feedback
- Animation callbacks ensure smooth transitions

This implementation provides a complete scroll menu with interactive buttons and counter functionality, demonstrating TouchGFX's capabilities for creating engaging user interfaces.