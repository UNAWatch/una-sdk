# UnaButtons

## Overview

The UnaButtons widget is a custom container component for TouchGFX designed to display a set of four controller-style buttons (L1, L2, R1, R2) with configurable color states. It provides a visual representation of button presses or states using colored overlays that can be toggled programmatically.

## Purpose

The primary purpose of the UnaButtons widget is to offer an intuitive, visual way to display the state of controller buttons in embedded GUI applications, such as gaming interfaces or remote control panels. It supports four distinct states per button: inactive (invisible), white, amber, and red, allowing for flexible status indication.

## Components

The widget consists of the following visual components:

- **Button Images**: Four sets of button images (L1, L2, R1, R2), each with three color variants:
  - Base (white): `Button_L1.png`, `Button_L2.png`, etc.
  - Amber: `Button_L1A.png`, `Button_L2A.png`, etc.
  - Red: `Button_L1R.png`, `Button_L2R.png`, etc.

All images are positioned to create a cohesive button layout, with dimensions appropriate for typical embedded displays.

## Functionality

The widget's core functionality is controlled through four methods: `setL1(Color color)`, `setL2(Color color)`, `setR1(Color color)`, and `setR2(Color color)`, each accepting a `Color` enum value:

- **NONE**: Button is invisible (all images hidden)
- **WHITE**: White button image visible
- **AMBER**: Amber button image visible
- **RED**: Red button image visible

Each method toggles the visibility of the appropriate image for that button, hiding the others and invalidating the area to trigger a redraw.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the UnaButtons package file (manifest.xml)

2. **Add to Screen**:
   - Drag the UnaButtons custom container from the Custom Containers panel onto your screen
   - Position and resize as needed

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - Button states are controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/ButtonsSet.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   ButtonsSet buttonsWidget;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(buttonsWidget);
   buttonsWidget.setPosition(x, y, width, height);
   ```

4. **Update Button States**:
   ```cpp
   buttonsWidget.setL1(ButtonsSet::WHITE);  // Set L1 to white
   buttonsWidget.setR2(ButtonsSet::RED);    // Set R2 to red
   buttonsWidget.setL2(ButtonsSet::NONE);   // Hide L2
   ```

5. **Example Integration**:
   ```cpp
   void MainView::updateButtonStates(bool l1Pressed, bool r1Pressed) {
       buttonsWidget.setL1(l1Pressed ? ButtonsSet::AMBER : ButtonsSet::NONE);
       buttonsWidget.setR1(r1Pressed ? ButtonsSet::RED : ButtonsSet::NONE);
       // Invalidate if needed, but methods already handle invalidation
   }
   ```

## Advanced Features and Hidden Gems

- **Independent Button Control**: Each of the four buttons (L1, L2, R1, R2) can be controlled independently, allowing complex state combinations.

- **Efficient Rendering**: Uses image visibility toggling rather than complex drawing operations, making it suitable for resource-constrained embedded systems with limited processing power.

- **Color-Coded States**: Employs intuitive color coding (white for neutral, amber for warning/alert, red for critical) following common UI conventions for status indication.

- **Fixed Layout**: Images are locked in position within the container, ensuring consistent appearance across different screen configurations and preventing accidental misalignment.

- **No Text Dependencies**: The widget is purely graphical, avoiding text rendering overhead and localization concerns.

- **Automatic Invalidation**: Each state change method automatically invalidates the affected images, ensuring the display updates immediately without manual intervention.

- **TouchGFX 4.26.0+ Compatibility**: Designed for modern TouchGFX versions with custom container support, leveraging the latest framework features.

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database (empty for this widget)
- `files/gui/include/gui/containers/ButtonsSet.hpp`: Header file
- `files/gui/src/containers/ButtonsSet.cpp`: Implementation file
- `files/assets/images/`: 12 PNG image assets (3 variants each for 4 buttons)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)