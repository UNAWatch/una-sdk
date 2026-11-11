(tutorials/buttons/architecture)=

# Buttons - Handling User Input and Navigation

Welcome to the UNA SDK tutorial series! The Buttons app demonstrates fundamental concepts of user interaction on the UNA Watch platform. This tutorial focuses on building an application with screens reaction via hardware buttons, providing a foundation for more complex user interfaces.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/Buttons)

As in HelloWorld, the app comes with a **TouchGFX** GUI and an **LVGL** GUI over the same service. Follow the toolkit you chose; the [Two Toolkits](#the-same-behaviour-in-two-toolkits) section shows the two side by side.

## What You'll Learn

- How to implement GUI applications with TouchGFX or LVGL
- Handling hardware button events for navigation
- Understanding the UNA app framework for interactive applications

## Getting Started

### Prerequisites

Before building the Buttons app, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

- UNA SDK cloned (`git clone https://github.com/UNAWatch/una-sdk.git`)
- ST ARM GCC Toolchain (from STM32CubeIDE/CubeCLT, not system GCC)
- CMake 3.21+ and make
- Python 3 with pip packages installed

**Minimum requirements for Buttons:**
- `UNA_SDK` environment variable pointing to SDK root
- ARM GCC toolchain in PATH
- CMake and build tools

**For the TouchGFX GUI:**
- TouchGFX Designer installed (see [toolchain setup](toolchain-setup))

**For the LVGL GUI:**
- The LVGL submodule checked out once: `git submodule update --init ThirdParty/lvgl` (from the SDK root)

### Building and Running Buttons

1. **Verify your environment setup** (see [toolchain setup](toolchain-setup) for details):

   ```bash
   echo $UNA_SDK                   # Should point to SDK root.
                                   # Note for backward compatibility with linux path notation it uses '/'

   which arm-none-eabi-gcc         # Should find ST toolchain
   which cmake                     # Should find CMake
   ```

2. **Navigate to the Buttons directory:**
   ```bash
   cd $UNA_SDK/Docs/Tutorials/Buttons
   ```

3. **Build the application** with the GUI of your choice:
   ```bash
   # TouchGFX GUI
   mkdir build && cd build
   cmake -G "Unix Makefiles" ../Software/Apps/Buttons-CMake
   make

   # LVGL GUI (from the tutorial directory again)
   cd .. && mkdir build-lvgl && cd build-lvgl
   cmake -G "Unix Makefiles" ../Software/Apps/ButtonsLVGL-CMake
   make
   ```

The app will start and display an initial black screen. Use the hardware buttons to navigate between different colored screens, demonstrating buttons navigation patterns. The two builds appear in the launcher as **Buttons** and **ButtonsLVGL**.

### Running on Simulator

**TouchGFX** (Windows only):

1. Open `Buttons.touchgfx` in TouchGFX Designer and click **Generate Code (F4)** (do this once).
2. Navigate to `Buttons\Software\Apps\TouchGFX-GUI\simulator\msvs`
3. Open `Application.vcxproj` in Visual Studio
4. Press **F5** to start debugging and run the simulator

**LVGL** (Windows and Linux): a CMake project in `Software/Apps/LVGL-GUI/simulator`, built the same way as HelloWorld's (see [that tutorial](../HelloWorld/ARCHITECTURE.md#running-on-simulator)); the executable is `ButtonsLVGLSimulator`.

In either simulator, use keyboard keys to simulate hardware buttons:
- **1** = L1 (Black screen)
- **2** = L2 (Red screen)
- **3** = R1 (Blue screen)
- **4** = R2 (Green screen, double-press to exit)

The simulator will display color changes based on button presses. For detailed simulator setup and button mapping, see [Simulator](../../Simulator.md).

## Buttons App Overview

### Navigation Flow
- Start on the black screen
- Press top left button (L1) → Black screen
- Press bottom left button (L2) → Red screen
- Press top right button (R1) → Blue screen
- Press bottom right button (R2) → Green screen
- Press R2 again → Exit the app

### Architecture Components

#### The Service Layer (Backend)
- Manages the application lifecycle
- Handles communication with the GUI layer
- Minimal implementation since no sensors are used
- One copy in `Software/Libs`, linked by both `Buttons-CMake` and `ButtonsLVGL-CMake`

#### The GUI Layer (Frontend)
- Built with TouchGFX (see [TouchGFX Port Architecture](../../TouchGFX-Port-Architecture.md)) or LVGL (see the [RunLVGL tutorial](../RunLVGL/ARCHITECTURE.md))
- Handles button events and screen transitions
- Updates the display based on user input
- Manages screen state and visual elements

#### Button Event Handling
The app responds to hardware button presses through the `handleKeyEvent` method in `MainView.cpp` (TouchGFX) or the `onKey` method in `MainScreen.cpp` (LVGL). Each button press triggers a screen change by updating the background color.

#### Screen Updates
TouchGFX: after changing a widget's properties (like color), `invalidate()` is called to refresh the display, ensuring changes are visible to the user.

LVGL: changing a style property marks the object dirty and LVGL redraws it on the next frame; there is no `invalidate()` to call.

## The Same Behaviour in Two Toolkits

| Piece | TouchGFX (`TouchGFX-GUI/gui`) | LVGL (`LVGL-GUI/gui`) |
|---|---|---|
| The background | the screen's own `__background` box from Designer | the screen object itself (`mRoot`) |
| Set a colour | `__background.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0xff)); __background.invalidate();` | `lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x0000FF), LV_PART_MAIN);` |
| Button hints | `ButtonsSet` container, `setL1(AMBER)` ... | `SDK::LVGL::Buttons`, `set(AMBER, AMBER, AMBER, AMBER)` |
| Key input | `MainView::handleKeyEvent(uint8_t key)` | `LV_EVENT_KEY` on the screen object, `MainScreen::onKey(uint8_t code)` |
| Double-R2 exit | `lastKeyPressed` member in `MainView` | `mLastKey` member in `MainScreen` |

One difference worth knowing: the kernel sends a press code when a button goes down and a release code when it comes up, as well as the click code. TouchGFX's `handleKeyEvent()` sees the same three codes, but this app only compares against the click codes, so the others fall through. The LVGL screen filters them out explicitly at the top of `onKey()`, so that a press does not reset the double-click detection between two R2 clicks.

The LVGL handler in full:

```cpp
void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (/* press or release code */) {
        return;
    }
    switch (code) {
        case Btn::L1: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x000000), LV_PART_MAIN); break;
        case Btn::L2: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0xFF0000), LV_PART_MAIN); break;
        case Btn::R1: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x0000FF), LV_PART_MAIN); break;
        case Btn::R2:
            lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x00FF00), LV_PART_MAIN);
            if (mLastKey == code) {
                mModel.exitApp();
            }
            break;
    }
    mLastKey = code;
}
```

Buttons draws no text, so its LVGL GUI has no `assets` directory and no `Assets.hpp`; the SDK port's fallback font stands in for LVGL's default. Everything else (`Model`, `GuiApp.cpp`, the simulator project, the CMake project) is HelloWorld's with the names changed.

### Size on the watch

| Build | `.uapp` | GUI code (text) | GUI RAM (bss) |
|---|---|---|---|
| Buttons (TouchGFX) | 223 KB | 209 KB | 71 KB |
| ButtonsLVGL | 172 KB | 162 KB | 139 KB |

## Buttons app creation process

The steps below create the TouchGFX GUI. For the LVGL GUI, copy HelloWorld's `LVGL-GUI` and `HelloWorldLVGL-CMake` instead, rename them, change `APP_NAME` and `APP_ID`, and edit `MainScreen.cpp` as shown above; there is no code to generate.

1. **Copy HelloWorld tutorial**
2. **Change naming**: Rename project directory, cmake directory and name of the project in CMakeLists.txt; Also change APP_ID to something else. Step 2 in [Creating New Apps](https://www.developers.unawatch.com/latest/sdk-setup.html#creating-new-apps) gives commmands for generating your own APP ID programatically from the name. 
3. **Commit initial changes**: it's a good practice to use version control system like git
4. ***Edit TouchGFX**: 
   - Rename `*.touchgfx` to `<MY_APP>.touchgfx`
   - Rename the 'Name' field in Buttons.touchgfx at line 163: `Buttons.touchgfx:163` `"Name": "Buttons"` to `"Name": "<MY_APP>"`
   - Add 240x240 a box `box1` at X:0 Y:0
   - Click **Generate code**
5. **Edit MainView.cpp**: 
   - MainView.cpp is located at '...\Software\Apps\TouchGFX-GUI\gui\src\main_screen'
   - You can see `touchgfx::Box box1;` in `MainViewBase.cpp` ('...\Software\Apps\TouchGFX-GUI\generated\gui_generated\src\main_screen') 
   - Add box1 color set to `void MainView::handleKeyEvent(uint8_t key)` :
      ```cpp
      void MainView::handleKeyEvent(uint8_t key)
      {
         if (key == Gui::Config::Button::L1) {
            box1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0));
         }

         if (key == Gui::Config::Button::L2) {
            box1.setColor(touchgfx::Color::getColorFromRGB(0xff, 0, 0));
         }

         if (key == Gui::Config::Button::R1) {
            box1.setColor(touchgfx::Color::getColorFromRGB(0, 0, 0xff));
         }

         if (key == Gui::Config::Button::R2) {
            box1.setColor(touchgfx::Color::getColorFromRGB(0, 0xff, 0));
         }
      }
      ``` 
   - In MainView.hpp ('...\Software\Apps\TouchGFX-GUI\gui\include\gui\main_screen')
   - Add `lastKeyPressed` state to exit on double **R2** clicks:
      ```cpp
      class MainView : public MainViewBase
      {
         uint8_t lastKeyPressed = {'\0'};
      public:
         MainView();
         virtual ~MainView() {}
         virtual void setupScreen();
         virtual void tearDownScreen();

      protected:
         virtual void handleKeyEvent(uint8_t key) override;
      };
      ````
      Exit:
      ```cpp
         if (key == Gui::Config::Button::R2) {
            box1.setColor(touchgfx::Color::getColorFromRGB(0, 0xff, 0));
            if (lastKeyPressed == key) presenter->exit();
         }
         lastKeyPressed = key;
      ```
6. **Add Screen refresh**: `box1.invalidate();`
   ```cpp
   if (key == Gui::Config::Button::R2) {
      box1.setColor(touchgfx::Color::getColorFromRGB(0, 0xff, 0));
      if (lastKeyPressed == key) presenter->exit();
   }
   lastKeyPressed = key;
   box1.invalidate();
   ```
7. **Compile code** using [SDK setup](../../sdk-setup.md) instructions, as shown in [Building and Running Buttons](#building-and-running-buttons). The build ends with the packed application:
``` 
INFO:root:Name           : Buttons
INFO:root:ID             : F1E2D3C448669782
INFO:root:Image          : ...\Buttons-CMake\build\Buttons_<version>.uapp
```

## Understanding Button Event Handling

The Buttons app demonstrates how to handle hardware button events. Key concepts include:

### Button Event Processing
- Button presses are captured in the `handleKeyEvent(uint8_t key)` method (TouchGFX) or the `LV_EVENT_KEY` handler (LVGL)
- Each button has a unique identifier (L1, L2, R1, R2), defined once for both toolkits in `SDK/GUI/Button.hpp`
- Events trigger immediate UI updates and state changes

### Screen State Management
- The app maintains current screen state through color and content changes
- Transitions are handled synchronously in the event handler
- TouchGFX needs an `invalidate()` call to refresh the display after changes; LVGL tracks dirty objects itself

### Exit Handling
- Double-press detection for the R2 button implements app exit
- State tracking with `lastKeyPressed` / `mLastKey` prevents accidental exits

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
- Color changes use `touchgfx::Color::getColorFromRGB()` or `SDK::LVGL::Draw::rgb()`
- TouchGFX screen refresh requires explicit `invalidate()` calls

### Button Configuration
- Buttons are configured in `setupScreen()` (TouchGFX) or the screen's constructor (LVGL) with initial states
- The `ButtonsSet` container / `SDK::LVGL::Buttons` widget draws the hints
- Hint colours are NONE, WHITE, AMBER or RED (LVGL adds GREEN)

### State Tracking
- Use member variables to maintain app state across events
- Track sequences like double-presses for special actions
- Reset state appropriately after actions complete

## Next Steps

1. **Run the Buttons app** - Build and test the navigation flow
2. **Modify button mappings** - Experiment with different screen transitions
3. **Add new screens** - Extend the app with additional colored screens
4. **Explore the widgets** - Add text, images, or other elements
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

The Buttons app provides a solid foundation for understanding user interaction on the UNA platform. Mastering these patterns will enable you to create engaging, responsive applications.
