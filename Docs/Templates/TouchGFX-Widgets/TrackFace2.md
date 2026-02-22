# TrackFace2

## Overview

The TrackFace2 widget is a custom container component for TouchGFX designed to display lap-specific fitness tracking data in a watch-face style layout. It presents heart rate with a visual indicator bar, lap pace, lap distance, and lap elapsed time in a vertically stacked arrangement with clear labels and separators, optimized for wearable device interfaces during interval training or lap-based activities.

## Purpose

The primary purpose of the TrackFace2 widget is to provide a comprehensive, at-a-glance view of lap-based activity metrics in embedded fitness applications. It displays heart rate with threshold-based visual feedback, lap pace (time per unit distance), lap distance traveled, and lap elapsed time, with support for both metric and imperial units, making it ideal for sports watches, fitness trackers, and interval training screens.

## Components

The widget consists of the following visual components arranged in a layout optimized for wearable displays:

- **Heart Rate Section** (top):
  - `hrText`: Label displaying "HR"
  - `hrValue`: Numeric heart rate value in BPM
  - `hrBar`: Visual heart rate indicator with 5 heart icons (HR_1.png to HR_5.png) based on thresholds

- **Lap Pace Section**:
  - `lapPaceText`: Label displaying "Lap Pace"
  - `lapPaceValue`: Displays lap pace in MM:SS format (minutes:seconds per unit)

- **Lap Distance Section**:
  - `lapDistText`: Label displaying "Lap Dist."
  - `lapDistValue`: Numeric lap distance value
  - `lapDistUnits`: Unit indicator ("km" or "mi.")

- **Lap Timer Section** (bottom):
  - `timerText`: Label displaying "Lap Time"
  - `timerValue`: Lap elapsed time in H:MM:SS or MM:SS format

- **Separators**:
  - Horizontal lines between sections for visual organization

All text elements use Poppins fonts in various weights and sizes for optimal readability on small screens.

## Functionality

The widget's core functionality centers on data display, unit management, and threshold-based heart rate visualization:

- **Heart Rate Display**: Accepts heart rate value and four threshold values to determine which of 5 heart icons to display (very low to very high intensity zones)
- **Lap Pace Display**: Accepts lap pace as seconds per kilometer, automatically converts to MM:SS format (e.g., 387 seconds = "6:27")
- **Lap Distance Display**: Accepts lap distance in meters, converts and displays in kilometers or miles with appropriate precision (1-2 decimal places)
- **Lap Timer Display**: Accepts lap elapsed time in seconds, formats as H:MM:SS or MM:SS (e.g., "0:05:49" or "5:49")
- **Unit System Support**: Automatic conversion between metric (km) and imperial (mi.) units
- **GPS Fix Handling**: Shows "---" placeholders when GPS signal is unavailable or data is invalid
- **Automatic Formatting**: Intelligent precision adjustment for distance values based on magnitude
- **Threshold-Based HR Visualization**: Heart rate bar shows one of 5 heart icons based on configurable intensity zones

The widget automatically invalidates and redraws text areas and heart rate icons when data changes, ensuring immediate visual updates.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TrackFace2 package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TrackFace2 custom container from the Custom Containers panel onto your screen
   - Position as needed (recommended size for optimal layout)
   - The widget includes both TrackFace2 and HrBar custom containers

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All data, unit settings, and heart rate thresholds are controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TrackFace2.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TrackFace2 trackFace;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(trackFace);
   trackFace.setPosition(x, y, width, height);
   ```

4. **Update Data**:
   ```cpp
   // Set heart rate with thresholds (fat burn: 120, cardio: 140, threshold: 160, maximum: 180)
   std::array<uint8_t, 4> thresholds = {120, 140, 160, 180};
   trackFace.setHR(155.0f, 0.0f, thresholds); // 155 BPM, shows hr4 icon

   // Set lap pace (seconds per kilometer)
   trackFace.setLapPace(387.0f, false, true); // 6:27 min/km, metric, GPS fix OK

   // Set lap distance (meters)
   trackFace.setLapDistance(5000.0f, false, true); // 5.00 km, metric, GPS fix OK

   // Set lap timer (seconds)
   trackFace.setLapTimer(349); // 0:05:49
   ```

5. **Handle Unit System**:
   ```cpp
   // Imperial units example
   trackFace.setLapPace(623.0f, true, true);  // 10:23 min/mi (equivalent to 6:27 min/km)
   trackFace.setLapDistance(3218.0f, true, true); // 2.00 mi
   ```

6. **GPS Fix Handling**:
   ```cpp
   // No GPS fix - shows "---"
   trackFace.setLapPace(0.0f, false, false);
   trackFace.setLapDistance(0.0f, false, false);
   ```

7. **Heart Rate Thresholds**:
   ```cpp
   // Define training zones
   std::array<uint8_t, 4> zones = {110, 130, 150, 170}; // Warm-up, Fat burn, Cardio, Threshold
   trackFace.setHR(145.0f, 0.0f, zones); // Shows hr3 (cardio zone)
   ```

## Advanced Features and Hidden Gems

- **Threshold-Based Heart Rate Visualization**: Five distinct heart rate zones provide immediate visual feedback on training intensity without needing to read numbers
- **Intelligent Distance Formatting**: Automatically adjusts decimal precision (2 places for <100 units, 1 place for ≥100 units) for optimal readability
- **GPS-Aware Display**: Gracefully handles signal loss by showing "---" instead of invalid data, improving user experience during GPS dropouts
- **Unit Conversion Built-in**: Seamless metric/imperial switching without external conversion logic required
- **Time Formatting Utilities**: Leverages TouchGFX's Unicode formatting for consistent time display across different locales
- **Efficient Updates**: Only invalidates changed text areas and heart rate icons, minimizing redraw overhead on embedded systems
- **Lap-Focused Metrics**: Specifically designed for interval training with lap-specific data rather than cumulative totals
- **Font Weight Hierarchy**: Uses different font weights (Regular, Italic, SemiBold) to create visual information hierarchy
- **Buffer Management**: Properly sized text buffers prevent overflow and ensure reliable rendering
- **Heart Rate Zone Training Support**: Threshold array allows customization for different fitness levels and training goals

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest with TrackFace2 and HrBar
- `content/texts.xml`: Text database with labels and templates
- `files/gui/include/gui/containers/TrackFace2.hpp`: Header file
- `files/gui/src/containers/TrackFace2.cpp`: Implementation file
- `files/gui/include/gui/containers/HrBar.hpp`: HrBar header file
- `files/gui/src/containers/HrBar.cpp`: HrBar implementation file
- `files/assets/fonts/`: 3 Poppins font variants (Italic, Regular, SemiBold)
- `files/assets/images/`: 6 HR images (HR_group.png, HR_1.png to HR_5.png)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- App::Utils utility functions for unit conversions and time formatting
- Standard TouchGFX container and text components
- C++11 or later for std::array support