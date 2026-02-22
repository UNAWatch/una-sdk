# Toggle

## Overview

The Toggle widget is a custom container component for TouchGFX designed to provide an interactive toggle switch interface. It visually represents a boolean state with a sliding handle that moves between on and off positions, accompanied by appropriate background rail images.

## Purpose

The primary purpose of the Toggle widget is to offer an intuitive, touch-interactive way to control binary settings in embedded GUI applications. It provides clear visual feedback for on/off states and supports a disabled appearance variant, making it suitable for settings screens, configuration panels, and control interfaces.

## Components

The widget consists of the following visual components:

- **Handle**: A movable element (`Toggle_Handle.png`) that slides horizontally to indicate the current state
- **Rail Images**:
  - `railOn`: Background rail for on state (`Toggle_On.png`)
  - `railOff`: Background rail for off state (`Toggle_Off.png`)
  - `railOffGray`: Alternative gray background rail for disabled appearance (`Toggle_Def.png`)

The handle moves between x=0 (off position) and x=30 (on position), providing smooth visual transition feedback.

## Functionality

The widget's core functionality revolves around state management:

- **State Control**: Boolean state (true = on, false = off) controlled via `setState(bool)` and retrieved via `getState()`
- **Visual Updates**: When state changes, the appropriate rail images are shown/hidden and the handle position is updated
- **Gray Background Mode**: Optional disabled appearance via `setBackgroundGray(bool)`, which uses the gray rail instead of the standard off rail
- **Touch Interaction**: Inherits click handling from the base class for user interaction

The widget automatically invalidates and redraws when state changes, ensuring immediate visual feedback.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the Toggle package file (manifest.xml)

2. **Add to Screen**:
   - Drag the Toggle custom container from the Custom Containers panel onto your screen
   - Position and resize as needed (recommended size accommodates the handle movement range)

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - Initial state and gray background are controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/Toggle.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   Toggle toggleWidget;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(toggleWidget);
   toggleWidget.setPosition(x, y, width, height);
   ```

4. **Control State**:
   ```cpp
   // Set toggle to on
   toggleWidget.setState(true);

   // Check current state
   bool currentState = toggleWidget.getState();

   // Enable gray background (disabled appearance)
   toggleWidget.setBackgroundGray(true);
   ```

5. **Example Integration**:
   ```cpp
   void MainView::toggleSetting() {
       bool newState = !toggleWidget.getState();
       toggleWidget.setState(newState);
       // Apply setting based on newState
       applySetting(newState);
   }

   void MainView::setDisabledMode(bool disabled) {
       toggleWidget.setBackgroundGray(disabled);
       toggleWidget.setState(false); // Reset to off when disabled
   }
   ```

## Advanced Features and Hidden Gems

- **Smooth Handle Animation**: The handle moves instantly between positions, providing clear visual feedback without complex animations that could impact performance
- **Efficient Rendering**: Uses simple image visibility toggling rather than bitmap manipulation, ideal for resource-constrained embedded systems
- **Gray Background Variant**: Built-in disabled state appearance without requiring separate assets or complex state management
- **Touch-Friendly Design**: Leverages TouchGFX's click handling infrastructure for reliable touch interaction
- **Minimal API**: Clean, simple interface focused on essential functionality without unnecessary complexity
- **Fixed Layout**: Handle movement is hardcoded for consistent behavior across different implementations
- **No Text Dependencies**: Purely graphical widget avoiding localization and font rendering overhead

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database (empty for this widget)
- `files/gui/include/gui/containers/Toggle.hpp`: Header file
- `files/gui/src/containers/Toggle.cpp`: Implementation file
- `files/assets/images/`: 4 PNG image assets (Toggle_Def.png, Toggle_Handle.png, Toggle_Off.png, Toggle_On.png)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- Standard TouchGFX container and image components