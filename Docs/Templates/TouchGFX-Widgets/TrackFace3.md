# TrackFace3

## Overview

The TrackFace3 widget is a custom container component for TouchGFX designed to display essential watch face information including current time and battery status. It presents the time in a digital format alongside a battery percentage indicator and a visual battery level icon, optimized for wearable device interfaces.

## Purpose

The primary purpose of the TrackFace3 widget is to provide a clean, at-a-glance view of critical smartwatch metrics in embedded fitness applications. It displays the current time and battery charge level with both numerical and visual indicators, making it ideal for watch faces, status screens, and power management interfaces in wearable devices.

## Components

The widget consists of the following visual components arranged in a vertical layout:

- **Time Section** (top):
  - `dayTimeText`: Label displaying "Day Time"
  - `valueDayTime`: Current time in HH:MM format (e.g., "17:35")

- **Battery Section** (bottom):
  - `valuePercent`: Battery level as percentage text (e.g., "63%")
  - `battery`: BatteryBig widget providing visual battery indicator with color-coded segments

All text elements use Poppins fonts in various weights and sizes for optimal readability on small screens.

## Functionality

The widget's core functionality centers on time and battery status display:

- **Time Display**: Accepts hour and minute values, automatically formats as HH:MM (e.g., 17, 35 → "17:35")
- **Battery Display**: Accepts battery level (0-255), displays as percentage and updates visual indicator
- **Visual Battery Indicator**: Integrates BatteryBig widget with four states:
  - Full (green): All segments green when level ≥ kThresholdHi
  - Medium (green/gray): Two green segments, one gray when level ≥ kThresholdLo
  - Low (red/gray): One red segment, two gray when level > 0
  - Empty (gray): All segments gray when level = 0

The widget automatically invalidates and redraws text areas and battery icon when data changes, ensuring immediate visual updates.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TrackFace3 package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TrackFace3 custom container from the Custom Containers panel onto your screen
   - Position as needed (recommended size for optimal layout)
   - The widget includes both TrackFace3 and BatteryBig custom containers

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All data (time and battery level) is controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TrackFace3.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TrackFace3 trackFace;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(trackFace);
   trackFace.setPosition(x, y, width, height);
   ```

4. **Update Time**:
   ```cpp
   // Set current time (hour, minute)
   trackFace.setTime(17, 35); // Displays "17:35"
   ```

5. **Update Battery Level**:
   ```cpp
   // Set battery level (0-255)
   trackFace.setBatteryLevel(160); // Displays "63%" and updates battery icon
   ```

6. **Complete Example**:
   ```cpp
   void MainView::updateWatchFace(uint8_t hour, uint8_t minute, uint8_t batteryLevel) {
       trackFace.setTime(hour, minute);
       trackFace.setBatteryLevel(batteryLevel);
   }
   ```

## Advanced Features and Hidden Gems

- **Integrated Battery Visualization**: Seamlessly combines numerical percentage with intuitive color-coded battery icon for comprehensive status communication
- **Efficient Change Detection**: Only updates display when time or battery values actually change, reducing unnecessary redraws on embedded systems
- **Unicode Formatting**: Leverages TouchGFX's Unicode::snprintf for consistent time and percentage formatting across locales
- **Modular Design**: Reuses BatteryBig component, promoting code reuse and consistent battery visualization across the application
- **Threshold-Based Battery States**: Battery visual states are configurable through Gui::Config::Battery thresholds, allowing customization for different device requirements
- **Minimal Resource Usage**: Text-based time display avoids bitmap fonts for time values, reducing memory footprint
- **Watch Face Optimization**: Specifically designed for circular watch layouts with vertical stacking of time and battery information
- **Automatic Invalidation**: Smart invalidation of only changed UI elements minimizes screen update overhead

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest with TrackFace3 and BatteryBig
- `content/texts.xml`: Text database with labels and templates
- `files/gui/include/gui/containers/TrackFace3.hpp`: Header file
- `files/gui/src/containers/TrackFace3.cpp`: Implementation file
- `files/gui/include/gui/containers/BatteryBig.hpp`: BatteryBig header file
- `files/gui/src/containers/BatteryBig.cpp`: BatteryBig implementation file
- `files/assets/fonts/`: 3 Poppins font variants (Italic, Regular, SemiBold)
- `files/assets/images/`: 7 PNG battery image assets (gray1-3, red1, green1-3)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- BatteryBig custom container for visual battery indicator
- `Gui::Config::Battery` configuration (for battery thresholds)
- Standard TouchGFX container and text components