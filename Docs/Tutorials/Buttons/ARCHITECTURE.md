(tutorials/buttons/architecture)=

# Buttons

Welcome to the UNA SDK tutorial series! Buttons is your first step in learning to build applications for the UNA watch platform. This tutorial focuses on the fundamental app architecture - how service and GUI components communicate - without the complexity of sensors or data logging.

**Tutorial Series Overview:**

1. **Buttons** (this tutorial): Basic app structure and communication
2. **Heart Rate App**: Adding sensor integration and real-time data
3. **Advanced Features**: Custom screens, data persistence, and complex interactions

By starting simple, you'll understand the core patterns that all UNA apps use, making advanced features much easier to learn.

## What You'll Learn

- How to set up your development environment for UNA apps
- The basic structure of a UNA watch application
- How service and GUI layers communicate
- How to build and run apps on simulator and hardware
- Understanding the UNA app framework fundamentals

## Getting Started

### Prerequisites

Before building Buttons, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

- UNA SDK cloned with submodules (`git clone --recursive`)
- ST ARM GCC Toolchain (from STM32CubeIDE/CubeCLT, not system GCC)
- CMake 3.21+ and make
- Python 3 with pip packages installed

**Minimum requirements for Buttons:**
- `UNA_SDK` environment variable pointing to SDK root

- ARM GCC toolchain in PATH
- CMake and build tools

**For GUI development/modification:**

- TouchGFX Designer installed (see [toolchain setup](toolchain-setup))

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

3. **Build the application:**
   ```bash
   mkdir build && cd build
   cmake -G "Unix Makefiles" ../Software/Apps/Buttons-CMake
   make
   ```

The app will start and show a basic GUI demonstrating the UNA app framework. This Buttons focuses on the core architecture - the service-GUI communication pattern that all UNA apps use.

### Working with TouchGFX GUI (Optional)

If you want to explore or modify the GUI design:

1. **Install TouchGFX Designer** (see [toolchain setup](toolchain-setup) for installation)

2. **Open the TouchGFX project:**
   ```
   Buttons.touchgfx
   ```

3. **Make design changes** in TouchGFX Designer (add/modify screens, widgets, interactions)

4. **Generate code** after making changes:
   - Click "Generate Code" button in TouchGFX Designer, OR

5. **Rebuild the app** to include your GUI changes:
   ```bash
   cmake -G "Unix Makefiles" /path/to/Buttons-CMake # If artifacts has been changed
   make
   ```

**Note**: Buttons has a minimal GUI. For learning GUI development, study the more complex examples like Cycling or HRMonitor apps, or see the [toolchain setup](toolchain-setup).

## Buttons app creation process

1. **Copy HelloWorld tutorial**
2. **Change naming**: Rename project directory, cmake directory and name of the project in CMakeLists.txt; Also change APP_ID to something else.
3. **Commit initial changes**: it's a good practice to use version control system like git
4. ***Edit TouchGFX**: 
   - Rename `*.touchgfx` to `Buttons.touchgfx`
   - Rename `Buttons.touchgfx:163` `"Name": "Buttons"`
   - Add 240x240 a box `box1` at X:0 Y:0
   - Click **Generate code**
5. **Edit MainView.cpp**: 
   - You can see `touchgfx::Box box1;` in `MainViewBase.cpp`
   - Add box1 color set to `void MainView::handleKeyEvent(uint8_t key)`:
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
6. **Add Screen refresh**:
   ```cpp
   if (key == Gui::Config::Button::R2) {
      box1.setColor(touchgfx::Color::getColorFromRGB(0, 0xff, 0));
      if (lastKeyPressed == key) presenter->exit();
   }
   lastKeyPressed = key;
   box1.invalidate();
   ```
7. **Compile code** using [SDK setup](../../sdk-setup.md) instructions.
   ``` bash
   
   ```

## Buttons App Overview

Buttons demonstrates the essential UNA app architecture:

### The Service Layer (Backend)
- Runs as the main application thread
- Handles sensor connections and data processing
- Manages app lifecycle (start/stop)
- Communicates with the GUI through messages

### The GUI Layer (Frontend)
- Built with TouchGFX framework
- Displays information to the user
- Receives updates from the service
- Handles user interactions

### Communication Between Layers
- Uses the UNA kernel messaging system
- Service sends data to GUI via custom messages
- GUI can send commands back to service

## Understanding the Commented Code

Buttons includes commented-out implementations of common UNA app features. These serve as reference examples for future tutorials:

- **Heart Rate Sensor Integration**: Complete sensor connection, data parsing, and real-time GUI updates
- **FIT File Logging**: Activity data recording with session summaries
- **Custom Messaging**: Service-to-GUI communication patterns

**Note**: The next tutorial will walk through enabling heart rate monitoring step-by-step. For now, focus on understanding the basic app structure and messaging framework.

## Understanding UNA App Communication

Buttons demonstrates the two main ways UNA apps communicate between service and GUI:

### SDK Custom Messages (Service → GUI)
Used for real-time data updates. The service creates messages using `SDK::make_msg<>()` and sends them via the kernel. The GUI receives them in `Model::customMessageHandler()`.

### AppTypes Events (GUI → Service)
Used for commands and configuration. The GUI sends events through the `IGuiBackend` interface, which the service implements to receive commands.

These patterns form the foundation of all UNA app communication. Future tutorials will show how to implement specific features using these systems.

## Common Patterns and Best Practices

### Sensor Integration
- Always check `matchesDriver(handle)` before processing sensor data
- Validate data with `isDataValid()` before using
- Handle sensor timeouts and disconnections gracefully

### Message Design
- Use unique message type IDs (increment from 0x00000001)
- Keep messages small and focused on single purposes
- Use descriptive names for message types and fields

### GUI Updates
- Only update GUI when necessary to avoid performance issues
- Use appropriate data types (float for measurements, int for counts)
- Handle invalid/missing data gracefully

### File Operations
- Use the kernel's filesystem interface (`mKernel.fs`)
- Handle file I/O errors appropriately
- FIT files are Garmin's standard format for activity data

## Next Steps

1. **Get Buttons running** - Follow the build steps above and confirm the app launches
2. **Explore the code structure** - Look at Service.cpp and Model.cpp to understand the messaging flow
3. **Check the commented examples** - Review the commented HR and FIT code to see what's available
4. **Continue to the next tutorial** - Learn how to enable heart rate monitoring and data logging
5. **Study other example apps** - Look at Alarm or Cycling apps for different patterns

## Troubleshooting

### Build Issues
- Ensure all SDK paths are correctly configured
- Check that TouchGFX is properly installed
- Verify CMake finds all required dependencies

### Runtime Issues
- Check log output for error messages
- Verify sensor connections on real hardware
- Use the simulator for initial testing

### Common Mistakes
- Forgetting to uncomment all related code sections
- Using duplicate message type IDs
- Not handling message memory management properly

Remember: Every complex app started as a simple Buttons. Take it step by step, and you'll be building amazing UNA watch applications in no time!