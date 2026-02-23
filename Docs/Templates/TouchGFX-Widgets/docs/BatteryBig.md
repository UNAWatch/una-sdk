# BatteryBig

## Overview

The BatteryBig widget is a custom container component for TouchGFX designed to display a visual battery level indicator. It provides a graphical representation of battery charge levels using colored segments that change based on the input level value.

## Purpose

The primary purpose of the BatteryBig widget is to offer an intuitive, visual way to communicate battery status in embedded GUI applications. It supports four distinct battery states: empty, low, medium, and full, each represented by different color combinations.

## Components

The widget consists of the following visual components:

- **Base Images (Gray)**: Three gray images (`gray1`, `gray2`, `gray3`) that form the base outline of the battery
- **Colored Overlays**:
  - `red1`: Red segment for low battery indication
  - `green1`, `green2`, `green3`: Green segments for medium to full battery indication

All images are positioned to create a cohesive battery icon with dimensions of 66x28 pixels.

## Functionality

The widget's core functionality is controlled through the `setBatteryLevel(uint8_t level)` method, which accepts a battery level value from 0-255. The visibility of different image segments is toggled based on predefined thresholds:

- **Full Battery** (level ≥ `kThresholdHi`): All three green segments visible
- **Medium Battery** (level ≥ `kThresholdLo`): Green segments 1 and 2 visible, gray segment 3 visible
- **Low Battery** (level > 0): Red segment 1 visible, gray segments 2 and 3 visible
- **Empty Battery** (level = 0): All gray segments visible

The thresholds `kThresholdHi` and `kThresholdLo` are defined in `Gui::Config::Battery` and can be customized for different applications.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the BatteryBig package file (manifest.xml)

2. **Add to Screen**:
   - Drag the BatteryBig custom container from the Custom Containers panel onto your screen
   - Position and resize as needed (recommended size: 66x28 pixels)

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - Battery level is controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/BatteryBig.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   BatteryBig batteryWidget;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(batteryWidget);
   batteryWidget.setPosition(x, y, 66, 28);
   ```

4. **Update Battery Level**:
   ```cpp
   batteryWidget.setBatteryLevel(currentBatteryLevel); // 0-255
   ```

5. **Example Integration**:
   ```cpp
   void MainView::updateBatteryDisplay(uint8_t level) {
       batteryWidget.setBatteryLevel(level);
       batteryWidget.invalidate(); // Force redraw if needed
   }
   ```

## Advanced Features and Hidden Gems

- **Configurable Thresholds**: Battery level thresholds are defined in `Gui::Config::Battery::kThresholdHi` and `Gui::Config::Battery::kThresholdLo`, allowing customization of what constitutes "low" and "high" battery levels without modifying the widget code.

- **Efficient Rendering**: Uses image visibility toggling rather than complex drawing operations, making it suitable for resource-constrained embedded systems.

- **Color-Coded States**: Employs intuitive color coding (gray for empty, red for low, green for adequate) following common UI conventions.

- **Fixed Layout**: Images are locked in position within the container, ensuring consistent appearance across different screen configurations.

- **No Text Dependencies**: The widget is purely graphical, avoiding text rendering overhead and localization concerns.

- **TouchGFX 4.26.0+ Compatibility**: Designed for modern TouchGFX versions with custom container support.

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database (empty for this widget)
- `files/gui/include/gui/containers/BatteryBig.hpp`: Header file
- `files/gui/src/containers/BatteryBig.cpp`: Implementation file
- `files/assets/images/`: 7 PNG image assets (gray1-3, red1, green1-3)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- `Gui::Config::Battery` configuration (for thresholds)