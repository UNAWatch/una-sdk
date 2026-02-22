# TrackFace1

## Overview

The TrackFace1 widget is a custom container component for TouchGFX designed to display fitness tracking data in a watch-face style layout. It presents three key metrics - pace, distance, and elapsed time - in a vertically stacked arrangement with clear labels and separators, optimized for wearable device interfaces.

## Purpose

The primary purpose of the TrackFace1 widget is to provide a comprehensive, at-a-glance view of running or walking activity metrics in embedded fitness applications. It displays pace (time per unit distance), distance traveled, and elapsed time, with support for both metric and imperial units, making it ideal for sports watches, fitness trackers, and activity monitoring screens.

## Components

The widget consists of the following visual components arranged in a 240x240 pixel layout:

- **Pace Section** (top):
  - `paceText`: Label displaying "Pace"
  - `paceValue`: Displays pace in MM:SS format (minutes:seconds per unit)

- **Distance Section** (middle):
  - `distanceText`: Label displaying "Distance"
  - `distanceValue`: Numeric distance value
  - `distanceUnits`: Unit indicator ("km" or "mi.")

- **Timer Section** (bottom):
  - `timerText`: Label displaying "Timer"
  - `timerValue`: Elapsed time in H:MM:SS format

- **Separators**:
  - `line1`: Horizontal line above distance section
  - `line2`: Horizontal line above timer section

All text elements use gray color (#C0C0C0) with Poppins fonts in various weights and sizes for optimal readability.

## Functionality

The widget's core functionality centers on data display and unit management:

- **Pace Display**: Accepts pace as seconds per kilometer, automatically converts to MM:SS format (e.g., 387 seconds = "6:27")
- **Distance Display**: Accepts distance in meters, converts and displays in kilometers or miles with appropriate precision (1-2 decimal places)
- **Timer Display**: Accepts elapsed time in seconds, formats as H:MM:SS (e.g., "0:05:49")
- **Unit System Support**: Automatic conversion between metric (km) and imperial (mi.) units
- **GPS Fix Handling**: Shows "---" placeholders when GPS signal is unavailable or data is invalid
- **Automatic Formatting**: Intelligent precision adjustment for distance values based on magnitude

The widget automatically invalidates and redraws text areas when data changes, ensuring immediate visual updates.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TrackFace1 package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TrackFace1 custom container from the Custom Containers panel onto your screen
   - Position as needed (recommended 240x240 size for optimal layout)

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All data and unit settings are controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TrackFace1.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TrackFace1 trackFace;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(trackFace);
   trackFace.setPosition(x, y, 240, 240);
   ```

4. **Update Data**:
   ```cpp
   // Set pace (seconds per kilometer)
   trackFace.setPace(387.0f, false, true); // 6:27 min/km, metric, GPS fix OK

   // Set distance (meters)
   trackFace.setDistance(5000.0f, false, true); // 5.00 km, metric, GPS fix OK

   // Set timer (seconds)
   trackFace.setTimer(349); // 0:05:49
   ```

5. **Handle Unit System**:
   ```cpp
   // Imperial units example
   trackFace.setPace(623.0f, true, true);  // 10:23 min/mi (equivalent to 6:27 min/km)
   trackFace.setDistance(3218.0f, true, true); // 2.00 mi
   ```

6. **GPS Fix Handling**:
   ```cpp
   // No GPS fix - shows "---"
   trackFace.setPace(0.0f, false, false);
   trackFace.setDistance(0.0f, false, false);
   ```

## Advanced Features and Hidden Gems

- **Intelligent Distance Formatting**: Automatically adjusts decimal precision (2 places for <100 units, 1 place for ≥100 units) for optimal readability
- **GPS-Aware Display**: Gracefully handles signal loss by showing "---" instead of invalid data, improving user experience during GPS dropouts
- **Unit Conversion Built-in**: Seamless metric/imperial switching without external conversion logic required
- **Time Formatting Utilities**: Leverages TouchGFX's Unicode formatting for consistent time display across different locales
- **Efficient Updates**: Only invalidates changed text areas, minimizing redraw overhead on embedded systems
- **Fixed Layout Optimization**: Carefully positioned elements ensure consistent appearance across different screen densities
- **Font Weight Hierarchy**: Uses different font weights (Regular, Medium, SemiBold) to create visual information hierarchy
- **Buffer Management**: Properly sized text buffers prevent overflow and ensure reliable rendering

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database with labels and templates
- `files/gui/include/gui/containers/TrackFace1.hpp`: Header file
- `files/gui/src/containers/TrackFace1.cpp`: Implementation file
- `files/assets/fonts/`: 4 Poppins font variants (Italic, Medium, Regular, SemiBold)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- App::Utils utility functions for unit conversions and time formatting
- Standard TouchGFX container and text components