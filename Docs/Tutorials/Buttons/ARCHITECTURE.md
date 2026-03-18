(tutorials/buttons/architecture)=

# Buttons Tutorial

Welcome to the UNA SDK tutorial series! The Buttons app demonstrates fundamental concepts of user interaction on the UNA watch platform. This tutorial focuses on building an application with screens reaction via hardware buttons, providing a foundation for more complex user interfaces.

[Project Folder](https://github.com/UNAWatch/una-sdk/tree/main/Docs/Tutorials/Buttons)

## What You'll Learn

- How to implement GUI applications with TouchGFX
- Handling hardware button events for navigation
- Understanding the UNA app framework for interactive applications

## Getting Started

### Prerequisites

Before building the Buttons app, you need to set up the UNA SDK environment. Follow the [toolchain setup](toolchain-setup) for complete installation instructions, including:

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

The app will start and display an initial black screen. Use the hardware buttons to navigate between different colored screens, demonstrating buttons navigation patterns.

## Buttons App Overview

### Navigation Flow
- Start on the black screen
- Press top right button (R1) → Blue screen
- Press bottom right button (R2) → Green screen
- Press left button (L1) → Red screen
- Press top left button (L2) → Return to black screen
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
The app responds to hardware button presses through the `handleKeyEvent` method in `MainView.cpp`. Each button press triggers a screen change by updating the background color and potentially displaying text.

#### Screen Updates
After changing screen properties (like color), the `invalidate()` method is called to refresh the display, ensuring changes are visible to the user.

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
6. **Add Screen refresh**: `box1.invalidate();`
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
PS C:\Users\sd\Desktop\una-dev\una-sdk> cd .\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\
PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake> mkdir build


    Directory: C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
d-----         2/18/2026   9:37 PM                build


PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake> cd .\build\
PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build>  cmake -G 'Unix Makefiles'  ..
-- The ASM compiler identification is GNU
-- Found assembler: C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712/tools/bin/arm-none-eabi-gcc.exe
-- The C compiler identification is GNU 13.3.1
-- The CXX compiler identification is GNU 13.3.1
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712/tools/bin/arm-none-eabi-gcc.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/ST/STM32CubeIDE_2.0.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.100.202509120712/tools/bin/arm-none-eabi-g++.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
[2026-02-18 21:37:08] Starting una-version.sh script
[2026-02-18 21:37:08] Default BUILD_VERSION set to: 0.0.0-dev
[2026-02-18 21:37:08] No argument provided, using current directory as UNA_GIT_DIR: C:/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/Buttons-CMake
[2026-02-18 21:37:08] Workspace: C:/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/Buttons-CMake
[2026-02-18 21:37:08] Successfully changed directory to: C:/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/Buttons-CMake
[2026-02-18 21:37:09] git rev-parse --git-dir output: 'C:/Users/sd/Desktop/una-dev/una-sdk/.git'
[2026-02-18 21:37:09] git rev-parse --show-toplevel output: 'C:/Users/sd/Desktop/una-dev/una-sdk'
[2026-02-18 21:37:09] toplevel: C:/Users/sd/Desktop/una-dev/una-sdk
[2026-02-18 21:37:09] Successfully changed directory to toplevel: C:/Users/sd/Desktop/una-dev/una-sdk
[2026-02-18 21:37:09] UNA_WORKSPACE set to: C:/Users/sd/Desktop/una-dev/una-sdk
[2026-02-18 21:37:09] Git directory found, proceeding with version detection
[2026-02-18 21:37:09] COMMIT_HASH: bf09e2b
[2026-02-18 21:37:09] Not on exact tag
[2026-02-18 21:37:09] DESC: v0.1.1-22-gbf09e2b
[2026-02-18 21:37:09] Tags found, parsing TAG and HASH
[2026-02-18 21:37:09] TAG before removing 'v': v0.1.1-22
[2026-02-18 21:37:09] TAG after removing 'v': 0.1.1-22
[2026-02-18 21:37:09] HASH: bf09e2b
[2026-02-18 21:37:09] BUILD_VERSION set to TAG-HASH: 0.1.1-22-bf09e2b
[2026-02-18 21:37:09] Checking for uncommitted changes
[2026-02-18 21:37:09] git status --porcelain output: ''
[2026-02-18 21:37:09] No uncommitted changes
[2026-02-18 21:37:09] Final BUILD_VERSION=0.1.1-22-bf09e2b
[2026-02-18 21:37:09] Script completed successfully
Detected BUILD_VERSION: 0.1.1-22-bf09e2b
UNA_APP_SERVICE_STACK_SIZE: 10*1024
UNA_APP_SERVICE_RAM_LENGTH: 500K
APP_ID: F1E2D3C448669782
APP_NAME: Buttons
DEV_ID: UNA
UNA_APP_GUI_STACK_SIZE: 10*1024
UNA_APP_GUI_RAM_LENGTH: 600K
App autostart is OFF
-- Configuring done (2.8s)
-- Generating done (0.1s)
-- Build files have been written to: C:/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/Buttons-CMake/build
PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build> make
[  0%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Libs/Sources/ActivityWriter.cpp.obj
[  1%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Libs/Sources/Service.cpp.obj
[  2%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/AtExitImpl.cpp.obj
[  3%] Building ASM object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/startup_user_app.s.obj
[  4%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/system.cpp.obj
[  5%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Kernel/KernelBuilder.cpp.obj
[  5%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/UnaLogger/Logger.cpp.obj
[  6%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/EntryPoint/Service/main.cpp.obj
[  7%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/FitHelper/FitHelper.cpp.obj
[  8%] Building C object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/ThirdParty/FitSDKRelease_21.171.00/c/fit.c.obj
[  9%] Building C object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/ThirdParty/FitSDKRelease_21.171.00/c/fit_convert.c.obj
[ 10%] Building C object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/ThirdParty/FitSDKRelease_21.171.00/c/fit_crc.c.obj
[ 10%] Building C object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/ThirdParty/FitSDKRelease_21.171.00/c/fit_product.c.obj
[ 11%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/JSON/JsonStreamReader.cpp.obj
[ 12%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/JSON/JsonStreamWriter.cpp.obj
[ 13%] Building C object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/ThirdParty/coreJSON/source/core_json.c.obj
[ 14%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/SensorLayer/SensorConnection.cpp.obj
[ 15%] Building CXX object CMakeFiles/ButtonsService.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/TrackMap/TrackMapBuilder.cpp.obj
[ 16%] Linking CXX executable ButtonsService.elf
Packing ButtonsService.elf
INFO:root:ELF-file    : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\ButtonsService.elf
INFO:root:Out         : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp
INFO:root:Version     : 1.0
INFO:root:Output file : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp\ButtonsService.srv

[ 16%] Built target ButtonsService.elf
[ 17%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/ApplicationFontProvider.cpp.obj
[ 18%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/CachedFont.cpp.obj
[ 19%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/CompressedFontCache.cpp.obj
[ 20%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/CompressedUnmappedFontCache.cpp.obj
[ 21%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/FontCache.cpp.obj
[ 21%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Italic_18_2bpp_0.cpp.obj
[ 22%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Italic_18_2bpp_4.cpp.obj
[ 23%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Italic_20_2bpp_0.cpp.obj
[ 24%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_LightItalic_18_2bpp_0.cpp.obj
[ 25%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Light_60_2bpp_0.cpp.obj
[ 26%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Medium_10_2bpp_0.cpp.obj
[ 26%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Medium_18_2bpp_0.cpp.obj
[ 27%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Medium_50_2bpp_0.cpp.obj
[ 28%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_Regular_18_2bpp_0.cpp.obj
[ 29%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_18_2bpp_0.cpp.obj
[ 30%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_18_2bpp_4.cpp.obj
[ 31%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_20_2bpp_0.cpp.obj
[ 31%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_25_2bpp_0.cpp.obj
[ 32%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_30_2bpp_0.cpp.obj
[ 33%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_35_2bpp_0.cpp.obj
[ 34%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_40_2bpp_0.cpp.obj
[ 35%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Font_Poppins_SemiBold_60_2bpp_0.cpp.obj
[ 36%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/GeneratedFont.cpp.obj
[ 37%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Italic_18_2bpp.cpp.obj
[ 37%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Italic_20_2bpp.cpp.obj
[ 38%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_LightItalic_18_2bpp.cpp.obj
[ 39%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Light_60_2bpp.cpp.obj
[ 40%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Medium_10_2bpp.cpp.obj
[ 41%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Medium_18_2bpp.cpp.obj
[ 42%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Medium_50_2bpp.cpp.obj
[ 42%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_Regular_18_2bpp.cpp.obj
[ 43%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_18_2bpp.cpp.obj
[ 44%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_20_2bpp.cpp.obj
[ 45%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_25_2bpp.cpp.obj
[ 46%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_30_2bpp.cpp.obj
[ 47%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_35_2bpp.cpp.obj
[ 47%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_40_2bpp.cpp.obj
[ 48%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Kerning_Poppins_SemiBold_60_2bpp.cpp.obj
[ 49%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Italic_18_2bpp.cpp.obj
[ 50%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Italic_20_2bpp.cpp.obj
[ 51%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_LightItalic_18_2bpp.cpp.obj
[ 52%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Light_60_2bpp.cpp.obj
[ 52%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Medium_10_2bpp.cpp.obj
[ 53%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Medium_18_2bpp.cpp.obj
[ 54%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Medium_50_2bpp.cpp.obj
[ 55%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_Regular_18_2bpp.cpp.obj
[ 56%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_18_2bpp.cpp.obj
[ 57%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_20_2bpp.cpp.obj
[ 58%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_25_2bpp.cpp.obj
[ 58%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_30_2bpp.cpp.obj
[ 59%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_35_2bpp.cpp.obj
[ 60%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_40_2bpp.cpp.obj
[ 61%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/Table_Poppins_SemiBold_60_2bpp.cpp.obj
[ 62%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/UnmappedDataFont.cpp.obj
[ 63%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/fonts/src/VectorFontRendererBuffers.cpp.obj
[ 63%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/gui_generated/src/common/FrontendApplicationBase.cpp.obj
[ 64%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/gui_generated/src/containers/ButtonsSetBase.cpp.obj
[ 65%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/3d628a36580307821a6a00b9871d2825/Software/Apps/TouchGFX-GUI/generated/gui_generated/src/main_screen/MainViewBase.cpp.obj
[ 66%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/BitmapDatabase.cpp.obj
[ 67%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/SVGDatabase.cpp.obj
[ 68%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L1.cpp.obj
[ 68%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L1A.cpp.obj
[ 69%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L1R.cpp.obj
[ 70%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L2.cpp.obj
[ 71%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L2A.cpp.obj
[ 72%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_L2R_.cpp.obj
[ 73%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R1.cpp.obj
[ 73%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R1A.cpp.obj
[ 74%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R1R.cpp.obj
[ 75%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R2.cpp.obj
[ 76%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R2A.cpp.obj
[ 77%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Button_R2R.cpp.obj
[ 78%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_Heart.cpp.obj
[ 79%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_MenuAssets_Cross.cpp.obj
[ 79%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_MenuAssets_CrossYellow.cpp.obj
[ 80%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/9e8b1f0454dae51d1529dac2c47dcb08/Buttons/Software/Apps/TouchGFX-GUI/generated/images/src/image_MenuAssets_Tick.cpp.obj
[ 81%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/texts/src/LanguageGb.cpp.obj
[ 82%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/texts/src/Texts.cpp.obj
[ 83%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/ff49e2950d17f26ffd1c2220befc6afd/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/generated/texts/src/TypedTextDatabase.cpp.obj
[ 84%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/c1f22cc05670b178c052f9be5e8d2c31/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/gui/src/common/FrontendApplication.cpp.obj
[ 84%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/gui/src/containers/ButtonsSet.cpp.obj
[ 85%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/gui/src/main_screen/MainPresenter.cpp.obj
[ 86%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/gui/src/main_screen/MainView.cpp.obj
[ 87%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Docs/Tutorials/Buttons/Software/Apps/TouchGFX-GUI/gui/src/model/Model.cpp.obj
[ 88%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/AtExitImpl.cpp.obj
[ 89%] Building ASM object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/startup_user_app.s.obj
[ 89%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/AppSystem/system.cpp.obj
[ 90%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Kernel/KernelBuilder.cpp.obj
[ 93%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/STM32TouchController.cpp.obj
[ 94%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/TouchGFXCommandProcessor.cpp.obj
[ 94%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/TouchGFXGPIO.cpp.obj
[ 95%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/TouchGFXHAL.cpp.obj
[ 96%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/generated/OSWrappers.cpp.obj
[ 97%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/generated/STM32DMA.cpp.obj
[ 98%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/generated/TouchGFXConfiguration.cpp.obj
[ 99%] Building CXX object CMakeFiles/ButtonsGUI.elf.dir/C_/Users/sd/Desktop/una-dev/una-sdk/Libs/Source/Port/TouchGFX/generated/TouchGFXGeneratedHAL.cpp.obj
[100%] Linking CXX executable ButtonsGUI.elf
Packing ButtonsGUI.elf
INFO:root:ELF-file    : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\ButtonsGUI.elf
INFO:root:Out         : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp
INFO:root:Version     : 1.0
INFO:root:Output file : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp\ButtonsGUI.gui
[100%] Built target ButtonsGUI.elf
[100%] Merging Buttons application
INFO:root:Merge complete
INFO:root:Service file   : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp\ButtonsService.srv
INFO:root:GUI file       : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Tmp\ButtonsGUI.gui
INFO:root:Name           : Buttons
INFO:root:ID             : F1E2D3C448669782
INFO:root:App Version    : 0.1.1
INFO:root:LibC Version   : 0.0.3
INFO:root:Flags          : 0x00000020
INFO:root:Glance-capable : yes
INFO:root:CRC            : 0x4B064208
INFO:root:Image          : C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build\Buttons_0.1.1-22-bf09e2b.uapp (225256 bytes)        

[100%] Built target ButtonsApp
PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Software\Apps\Buttons-CMake\build> cd ../../../../
PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons> ls .\Output\


    Directory: C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons\Output


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----         2/17/2026   7:10 PM            165 .gitignore
-a----         2/18/2026   9:37 PM         802019 ButtonsGUI.elf.elf.map
-a----         2/18/2026   9:37 PM         321587 ButtonsService.elf.elf.map
-a----         2/18/2026   9:37 PM         225256 Buttons_0.1.1-22-bf09e2b.uapp


PS C:\Users\sd\Desktop\una-dev\una-sdk\Docs\Tutorials\Buttons>
```

## Understanding Button Event Handling

The Buttons app demonstrates how to handle hardware button events in TouchGFX applications. Key concepts include:

### Button Event Processing
- Button presses are captured in the `handleKeyEvent(uint8_t key)` method
- Each button has a unique identifier (L1, L2, R1, R2)
- Events trigger immediate UI updates and state changes

### Screen State Management
- The app maintains current screen state through color and content changes
- Transitions are handled synchronously in the event handler
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
- Color changes use `touchgfx::Color::getColorFromRGB()`
- Screen refresh requires explicit `invalidate()` calls

### Button Configuration
- Buttons are configured in `setupScreen()` with initial states
- The ButtonsSet widget manages hardware button integration
- Button states can be NONE, PRESSED, or other states

### State Tracking
- Use member variables to maintain app state across events
- Track sequences like double-presses for special actions
- Reset state appropriately after actions complete

## Next Steps

1. **Run the Buttons app** - Build and test the navigation flow
2. **Modify button mappings** - Experiment with different screen transitions
3. **Add new screens** - Extend the app with additional colored screens
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

The Buttons app provides a solid foundation for understanding user interaction on the UNA platform. Mastering these patterns will enable you to create engaging, responsive applications.