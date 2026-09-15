(tutorials/scrollmenu/architecture)=

# ScrollMenu - Creating Scrollable Lists and Menus

Welcome to the UNA SDK tutorial series! The ScrollMenu app demonstrates fundamental concepts of menu navigation on the UNA Watch platform. This tutorial focuses on building an application with a scrollable menu using hardware buttons, providing a foundation for more complex user interfaces.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/ScrollMenu)

As in the earlier tutorials, the app comes with a **TouchGFX** GUI and an **LVGL** GUI over the same service. The menu is the first widget that differs substantially between the two: TouchGFX composes it from Designer containers and a `ScrollWheelWithSelectionStyle`, LVGL uses the SDK's `WheelMenu`. See [The Same Menu in Two Toolkits](tutorials/scrollmenu/two-toolkits).

## What You'll Learn

- How to implement GUI applications with TouchGFX or LVGL
- Handling hardware button events for menu navigation
- Understanding the UNA app framework for interactive applications

## Getting Started

### Prerequisites

Before building the ScrollMenu app, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

- UNA SDK cloned (`git clone https://github.com/UNAWatch/una-sdk.git`)
- ST ARM GCC Toolchain (from STM32CubeIDE/CubeCLT, not system GCC)
- CMake 3.21+ and make
- Python 3 with pip packages installed

**Minimum requirements for ScrollMenu:**
- `UNA_SDK` environment variable pointing to SDK root
- ARM GCC toolchain in PATH
- CMake and build tools

**For the TouchGFX GUI:**
- TouchGFX Designer installed (see [toolchain setup](toolchain-setup))

**For the LVGL GUI:**
- The LVGL submodule checked out once: `git submodule update --init ThirdParty/lvgl` (from the SDK root)

### Building and Running ScrollMenu

1. **Verify your environment setup** (see [toolchain setup](toolchain-setup) for details):

   ```bash
   echo $UNA_SDK                   # Should point to SDK root.
                                   # Note for backward compatibility with linux path notation it uses '/'

   which arm-none-eabi-gcc         # Should find ST toolchain
   which cmake                     # Should find CMake
   ```

2. **Navigate to the ScrollMenu directory:**
   ```bash
   cd $UNA_SDK/Docs/Tutorials/ScrollMenu
   ```

3. **Build the application** with the GUI of your choice:
   ```bash
   # TouchGFX GUI
   mkdir build && cd build
   cmake -G "Unix Makefiles" ../Software/Apps/ScrollMenu-CMake
   make

   # LVGL GUI (from the tutorial directory again)
   cd .. && mkdir build-lvgl && cd build-lvgl
   cmake -G "Unix Makefiles" ../Software/Apps/ScrollMenuLVGL-CMake
   make
   ```

The app will start and display an initial menu with three items. Use the hardware buttons to navigate the menu and perform actions, demonstrating menu navigation patterns. The two builds appear in the launcher as **ScrollMenu** and **ScrollMenuLVGL**.

### Running on Simulator

**TouchGFX** (Windows only):

1. Open `ScrollMenu.touchgfx` in TouchGFX Designer and click **Generate Code (F4)** (do this once).
2. Navigate to `ScrollMenu\Software\Apps\TouchGFX-GUI\simulator\msvs`
3. Open `Application.vcxproj` in Visual Studio
4. Press **F5** to start debugging and run the simulator

**LVGL** (Windows and Linux): a CMake project in `Software/Apps/LVGL-GUI/simulator`, built the same way as HelloWorld's (see [that tutorial](tutorials/helloworld/running-on-simulator)); the executable is `ScrollMenuLVGLSimulator`.

In either simulator, use keyboard keys to simulate hardware buttons:
- **1** = L1 (Previous menu item)
- **2** = L2 (Next menu item)
- **3** = R1 (Perform action on selected item)
- **4** = R2 (Exit)

The simulator will display the scrollable menu with Counter, Increase, Decrease items. For detailed simulator setup and button mapping, see [Simulator](../../Simulator.md).

## ScrollMenu App Overview

### Navigation Flow
- Start with menu displaying Counter, Increase, Decrease items
- Press top left button (L1) → Select previous menu item
- Press bottom left button (L2) → Select next menu item
- Press top right button (R1) → Perform action on selected item:
  - Counter: Reset counter to 0
  - Increase: Increment counter
  - Decrease: Decrement counter
- Press bottom right button (R2) → Exit the app

### Architecture Components

#### The Service Layer (Backend)
- Manages the application lifecycle
- Handles communication with the GUI layer
- Minimal implementation since no sensors are used
- One copy in `Software/Libs`, linked by both `ScrollMenu-CMake` and `ScrollMenuLVGL-CMake`

#### The GUI Layer (Frontend)
- Built with TouchGFX or LVGL
- Handles button events and screen transitions
- Updates the display based on user input
- Manages screen state and visual elements

#### Button Event Handling
The app responds to hardware button presses through the `handleKeyEvent` method in `MainView.cpp` (TouchGFX) or `onKey` in `MainScreen.cpp` (LVGL). L1 and L2 buttons navigate the menu by selecting previous or next items. R1 button performs the action associated with the currently selected menu item, such as resetting, incrementing, or decrementing the counter. R2 exits the application.

#### Menu Item Management
The menu displays items with different appearances for selected and unselected states:
- **Selected items** are highlighted in the center of the menu (typically with different styling)
- **Unselected items** are shown in the background with standard appearance
- When updating dynamic content (like the counter value), both selected and unselected versions of the item are updated to maintain consistency when scrolling

#### Screen Updates
TouchGFX: after performing actions that change the menu display (such as updating the counter value), the `invalidate()` method is called on the menu and its items to refresh the display, ensuring changes are visible to the user.

LVGL: the wheel's `refresh()` re-renders its slots from the item table, and LVGL redraws what changed on the next frame.

(tutorials/scrollmenu/two-toolkits)=
## The Same Menu in Two Toolkits

| Piece | TouchGFX (`TouchGFX-GUI/gui`) | LVGL (`LVGL-GUI/gui`) |
|---|---|---|
| The menu | `Menu` custom container: a `ScrollWheelWithSelectionStyle`, a background image, `Title`, `SideBarBig`, `Buttons` | `SDK::LVGL::WheelMenu` (`SDK/GUI/LVGL/WheelMenu.hpp`): lens, two item strips, `ScrollIndicator` |
| Item appearance | `MenuItemSelected` / `MenuItemNotSelected` containers, one pair per item, copied into the wheel's drawables | one `WheelMenu::Item` per entry: style, text, optional font, tip, toggle state, icons |
| Item texts | text keys `T_COUNTER`, `T_INCREASE`, `T_DECREASE` from Designer's Texts tab | plain `const char*` in the item table |
| Fonts | typographies `Poppins_SemiBold_30`, `Poppins_Medium_18`, `Poppins_Italic_18` | the same three faces, converted by `lvgl_assets.py` and passed as `WheelMenu::Fonts` |
| Navigate | `menu1.selectPrev()` / `menu1.selectNext()` (wheel + side bar animate together) | `mMenu->prev()` / `mMenu->next()` (the widget slides and moves its indicator) |
| Which item | `menu1.getSelectedItem()` | `mMenu->selected()` |
| Change the counter's text | `config(buffer)` on both the selected and not-selected copies, then `invalidate()` on the menu and items | the item's `text` points at a buffer the screen rewrites, then `mMenu->refresh()` |
| The lens behind the selection | `Background_TealDark.png` bitmap | drawn by the widget (`setBackground()` recolours it) |
| The scroll indicator | `SideBarBig` container with a rail bitmap and two `Circle` arcs | drawn by the widget's `ScrollIndicator` with `lv_arc` |

The LVGL screen in full, less the boilerplate shared with the earlier tutorials:

```cpp
using SDK::LVGL::WheelMenu;
using Style = WheelMenu::Item::Style;

mItems[COUNTER]  = { Style::Simple, mCounterText };   // a buffer, so it can show the count
mItems[INCREASE] = { Style::Simple, "Increase" };
mItems[DECREASE] = { Style::Simple, "Decrease" };

const WheelMenu::Fonts fonts = { &poppins_semibold_30, &poppins_medium_18, &poppins_italic_18 };
mMenu = std::make_unique<WheelMenu>(mRoot, mItems, ITEM_COUNT, fonts);

// ... in onKey():
case Btn::L1: mMenu->prev(); break;
case Btn::L2: mMenu->next(); break;
case Btn::R1:
    switch (mMenu->selected()) {
        case COUNTER:  mCounter = 0; break;
        case INCREASE: mCounter++;   break;
        case DECREASE: mCounter--;   break;
    }
    snprintf(mCounterText, sizeof(mCounterText), "Counter: %d", mCounter);
    mMenu->refresh();
    break;
```

`WheelMenu` keeps a pointer to the item table for as long as it lives, so the table is a member of the screen, not a local. It is the same widget the RunLVGL activity app builds its menus from; `Item::Style` also offers `Tip` (a hint line under the text), `Toggle` (an on/off switch) and `Icon` (a bitmap beside the text), and `setSlideMidCallback()` reports the moment the incoming item takes the centre.

### Size on the watch

| Build | `.uapp` | GUI code (text) | GUI RAM (bss) |
|---|---|---|---|
| ScrollMenu (TouchGFX) | 295 KB | 275 KB | 88 KB |
| ScrollMenuLVGL | 188 KB | 179 KB | 139 KB |

## ScrollMenu app creation process

The steps below create the TouchGFX GUI. For the LVGL GUI, copy HelloWorld's `LVGL-GUI` and `HelloWorldLVGL-CMake`, rename them, change `APP_NAME` and `APP_ID`, add the three fonts to `assets/assets.json` and regenerate, and write `MainScreen.cpp` as shown above.

1. **Copy HelloWorld tutorial**
2. **Change naming**: Rename project directory, cmake directory and name of the project in CMakeLists.txt; Also change APP_ID to something else. Step 2 in [Creating New Apps](https://www.developers.unawatch.com/latest/sdk-setup.html#creating-new-apps) gives commmands for generating your own APP ID programatically from the name. 
3. **Commit initial changes**: it's a good practice to use version control system like git
4. ***Add text keys using TouchGFX Designer**:
    - In TouchGFX Designer, go to the **Texts** tab
    - Click **+ Add Text** to create new text entries
    - Add three text entries with the following properties:
      - **Text Id**: "Counter", **Alignment**: Center, **Typography**: Poppins_Medium_25, **Translation (GB)**: "Counter"
      - **Text Id**: "Increase", **Alignment**: Center, **Typography**: Poppins_Medium_25, **Translation (GB)**: "Increase"
      - **Text Id**: "Decrease", **Alignment**: Center, **Typography**: Poppins_Medium_25, **Translation (GB)**: "Decrease"
    - These will generate the text keys T_COUNTER, T_INCREASE, T_DECREASE used in the code
5. ***Edit TouchGFX**: 
   - Rename `*.touchgfx` to `MY_APP.touchgfx`
   - Rename `ScrollMenu.touchgfx:163` `"Name": "MY_APP"`
   - Open the main screen in the designer
   - From the widget palette, locate and drag a **Menu** widget onto the main screen canvas
   - Name the menu widget `menu1` in the properties panel
   - Configure the menu properties:
     - Set the number of items to 3
     - Customize the appearance of selected and unselected items (fonts, colors, etc.)
     - Set initial text for each item using the text keys (T_COUNTER, T_INCREASE, T_DECREASE)
   - Position and size the menu widget appropriately on the screen (typically centered)
   - Click **Generate code**
6. **Edit MainView.hpp**:
    - Add member variables for state tracking:
      ```cpp
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
      ```
7. **Edit MainView.cpp**:
   - In `setupScreen()`, configure the menu items:
      ```cpp
      void MainView::setupScreen()
      {
          MainViewBase::setupScreen();

          menu1.setNumberOfItems(3);
          menu1.getNotSelectedItem(0)->config(T_COUNTER);
          menu1.getNotSelectedItem(1)->config(T_INCREASE);
          menu1.getNotSelectedItem(2)->config(T_DECREASE);
          menu1.getSelectedItem(0)->config(T_COUNTER);
          menu1.getSelectedItem(1)->config(T_INCREASE);
          menu1.getSelectedItem(2)->config(T_DECREASE);
          menu1.invalidate();

          buttons.setL1(ButtonsSet::NONE);
          buttons.setL2(ButtonsSet::NONE);
          buttons.setR1(ButtonsSet::NONE);
          buttons.setR2(ButtonsSet::AMBER);
      }
      ``` 
   - Implement `handleKeyEvent` for menu navigation and actions:
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
              auto* counter_nosel_item = menu1.getNotSelectedItem(0);
              auto* counter_sel_item = menu1.getSelectedItem(0);

              touchgfx::Unicode::UnicodeChar buffer[32];
              switch (selected) {
              case 0:
                  // Reset counter
                  counter = 0;
                  touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
                  counter_nosel_item->config(buffer);
                  counter_sel_item->config(buffer);
                  break;
              case 1:
                  counter++;
                  touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
                  counter_nosel_item->config(buffer);
                  counter_sel_item->config(buffer);
                  // Increment counter
                  break;
              case 2:
                  // Decrement counter
                  counter--;
                  touchgfx::Unicode::snprintf(buffer, 32, "Counter: %d", counter);
                  counter_nosel_item->config(buffer);
                  counter_sel_item->config(buffer);
                  break;
              }
              menu1.invalidate();
              counter_nosel_item->invalidate();
              counter_sel_item->invalidate();
          }

          if (key == Gui::Config::Button::R2) {
              presenter->exit();
          }
      }
      ```
8. **Compile code** using [SDK setup](../../sdk-setup.md) instructions.

## Understanding Menu Navigation

The ScrollMenu app demonstrates how to handle hardware button events for menu navigation. Key concepts include:

### Button Event Processing
- Button presses are captured in the `handleKeyEvent(uint8_t key)` method (TouchGFX) or the `LV_EVENT_KEY` handler (LVGL)
- L1 and L2 buttons navigate the menu (select previous/next item)
- R1 button performs the action associated with the selected menu item
- R2 button exits the app

### Menu State Management
- The app maintains current menu selection state
- Menu items display dynamic content (counter value)
- TouchGFX needs `invalidate()` calls to refresh the display after changes; the LVGL wheel is re-rendered with `refresh()`

## Common Patterns and Best Practices

### Button Handling
- Map button IDs to meaningful actions consistently
- Use state variables to track multi-press sequences
- In TouchGFX, always call `invalidate()` after visual changes

### UI Updates
- Keep event handlers lightweight to maintain responsiveness
- Use appropriate widgets for different content types
- Consider user feedback for button presses (visual/audio)

### Code Organization
- Separate UI logic from business logic
- Use clear naming for button actions and screen states
- Document button mappings in comments

### Performance Considerations
- Minimize work in event handlers
- Batch UI updates when possible
- Avoid blocking operations that could delay response

## Key Code Insights for New Developers

### MainView.cpp / MainScreen.cpp Structure
- `handleKeyEvent()` / `onKey()` is the central point for user input
- Menu navigation uses `menu1.selectPrev()` / `menu1.selectNext()` or `mMenu->prev()` / `mMenu->next()`
- Menu actions update the counter and refresh the display

### Menu Configuration
- TouchGFX: the menu is configured in `setupScreen()` with item texts and count; items are configured with `config()`
- LVGL: the menu is built in the screen's constructor from an item table and a `Fonts` struct

### State Tracking
- Use member variables like `counter` to maintain app state
- Update menu item text dynamically based on state changes

## Next Steps

1. **Run the ScrollMenu app** - Build and test the menu navigation
2. **Modify menu items** - Experiment with different menu actions
3. **Add new menu items** - Extend the app with additional menu options
4. **Explore the widgets** - Add text, images, or other elements; in LVGL try the `Tip` and `Toggle` item styles
5. **Study advanced examples** - Look at apps with more complex navigation

## Troubleshooting

### Build Issues
- Ensure TouchGFX Designer is properly installed
- Check that all project files are generated correctly
- For the LVGL build, check that `ThirdParty/lvgl` is populated
- Verify CMake configuration matches your environment

### Runtime Issues
- Confirm button mappings in the simulator match hardware
- Check log output for event handling errors
- Test on actual hardware for button responsiveness

### Common Mistakes
- Forgetting to call `invalidate()` after UI changes (TouchGFX)
- Incorrect button ID constants
- Not handling all button states appropriately

The ScrollMenu app provides a solid foundation for understanding menu navigation on the UNA platform. Mastering these patterns will enable you to create engaging, responsive applications.
