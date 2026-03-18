(tutorials/scrollmenu/architecture)=

# ScrollMenu Tutorial

Welcome to the UNA SDK tutorial series! The ScrollMenu app demonstrates fundamental concepts of menu navigation on the UNA watch platform. This tutorial focuses on building an application with a scrollable menu using hardware buttons, providing a foundation for more complex user interfaces.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/ScrollMenu)

## What You'll Learn

- How to implement GUI applications with TouchGFX
- Handling hardware button events for menu navigation
- Understanding the UNA app framework for interactive applications

## Getting Started

### Prerequisites

Before building the ScrollMenu app, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

- UNA SDK cloned with submodules (`git clone --recursive`)
- ST ARM GCC Toolchain (from STM32CubeIDE/CubeCLT, not system GCC)
- CMake 3.21+ and make
- Python 3 with pip packages installed

**Minimum requirements for ScrollMenu:**
- `UNA_SDK` environment variable pointing to SDK root
- ARM GCC toolchain in PATH
- CMake and build tools

**For GUI development/modification:**
- TouchGFX Designer installed (see [toolchain setup](toolchain-setup))

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

3. **Build the application:**
   ```bash
   mkdir build && cd build
   cmake -G "Unix Makefiles" ../Software/Apps/ScrollMenu-CMake
   make
   ```

The app will start and display an initial menu with three items. Use the hardware buttons to navigate the menu and perform actions, demonstrating menu navigation patterns.

## ScrollMenu App Overview

### Navigation Flow
- Start with menu displaying Counter, Increase, Decrease items
- Press left button (L1) → Select previous menu item
- Press top left button (L2) → Select next menu item
- Press top right button (R1) → Perform action on selected item:
  - Counter: Reset counter to 0
  - Increase: Increment counter
  - Decrease: Decrement counter
- Double-press bottom right button (R2) → Exit the app

### Architecture Components

#### The Service Layer (Backend)
- Manages the application lifecycle
- Handles communication with the GUI layer
- Minimal implementation since no sensors are used

#### The GUI Layer (Frontend)
- Built with TouchGFX framework
- Handles button events and screen transitions
- Updates the display based on user input
- Manages screen state and visual elements

#### Button Event Handling
The app responds to hardware button presses through the `handleKeyEvent` method in `MainView.cpp`. L1 and L2 buttons navigate the menu by selecting previous or next items. R1 button performs the action associated with the currently selected menu item, such as resetting, incrementing, or decrementing the counter. R2 button detects double-presses to exit the application.

#### Menu Item Management
The menu displays items with different appearances for selected and unselected states:
- **Selected items** are highlighted in the center of the menu (typically with different styling)
- **Unselected items** are shown in the background with standard appearance
- When updating dynamic content (like the counter value), both selected and unselected versions of the item are updated to maintain consistency when scrolling

#### Screen Updates
After performing actions that change the menu display (such as updating the counter value), the `invalidate()` method is called on the menu and its items to refresh the display, ensuring changes are visible to the user.

## ScrollMenu app creation process

1. **Copy HelloWorld tutorial**
2. **Change naming**: Rename project directory, cmake directory and name of the project in CMakeLists.txt; Also change APP_ID to something else.
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
   - Rename `*.touchgfx` to `ScrollMenu.touchgfx`
   - Rename `ScrollMenu.touchgfx:163` `"Name": "ScrollMenu"`
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
              if (lastKeyPressed == key) presenter->exit();
          }
          lastKeyPressed = key;
      }
      ```
8. **Compile code** using [SDK setup](../../sdk-setup.md) instructions.

## Understanding Menu Navigation

The ScrollMenu app demonstrates how to handle hardware button events for menu navigation in TouchGFX applications. Key concepts include:

### Button Event Processing
- Button presses are captured in the `handleKeyEvent(uint8_t key)` method
- L1 and L2 buttons navigate the menu (select previous/next item)
- R1 button performs the action associated with the selected menu item
- R2 button detects double-presses for app exit

### Menu State Management
- The app maintains current menu selection state
- Menu items display dynamic content (counter value)
- The `invalidate()` call ensures the display refreshes after changes

### Exit Handling
- Double-press detection for the R2 button implements app exit
- State tracking with `lastKeyPressed` prevents accidental exits

## Common Patterns and Best Practices

### Button Handling
- Map button IDs to meaningful actions consistently
- Use state variables to track multi-press sequences
- Always call `invalidate()` after visual changes

### UI Updates
- Keep event handlers lightweight to maintain responsiveness
- Use appropriate TouchGFX widgets for different content types
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

### MainView.cpp Structure
- `handleKeyEvent()` is the central point for user input
- Menu navigation uses `menu1.selectPrev()` and `menu1.selectNext()`
- Menu actions update counter and refresh display with `invalidate()` calls

### Menu Configuration
- Menu is configured in `setupScreen()` with item texts and count
- The Menu widget manages item selection and display
- Menu items can be configured with text using `config()` method

### State Tracking
- Use member variables like `counter` and `lastKeyPressed` to maintain app state
- Track sequences like double-presses for special actions
- Update menu item text dynamically based on state changes

## Next Steps

1. **Run the ScrollMenu app** - Build and test the menu navigation
2. **Modify menu items** - Experiment with different menu actions
3. **Add new menu items** - Extend the app with additional menu options
4. **Explore TouchGFX widgets** - Add text, images, or other elements
5. **Study advanced examples** - Look at apps with more complex navigation

## Troubleshooting

### Build Issues
- Ensure TouchGFX Designer is properly installed
- Check that all project files are generated correctly
- Verify CMake configuration matches your environment

### Runtime Issues
- Confirm button mappings in the simulator match hardware
- Check log output for event handling errors
- Test on actual hardware for button responsiveness

### Common Mistakes
- Forgetting to call `invalidate()` after UI changes
- Incorrect button ID constants
- Not handling all button states appropriately

The ScrollMenu app provides a solid foundation for understanding menu navigation on the UNA platform. Mastering these patterns will enable you to create engaging, responsive applications.