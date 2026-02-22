# TrackSummary

## Overview

The TrackSummary widget is a custom container component for TouchGFX designed to display comprehensive track activity summary information including distance traveled, average pace, elapsed time, and a visual track map. It presents fitness tracking data in a clean, organized layout optimized for wearable device interfaces and activity summary screens.

## Purpose

The primary purpose of the TrackSummary widget is to provide a detailed overview of completed fitness activities in embedded applications. It displays key metrics (distance, pace, time) alongside a visual representation of the track path, making it ideal for post-activity summaries, workout history screens, and performance tracking interfaces in wearable fitness devices.

## Components

The widget consists of the following visual components arranged in a structured layout:

- **Distance Section** (top-left):
  - `distanceValue`: Distance value in numeric format (e.g., "23.98")
  - `distanceUnits`: Distance units ("km" or "mi.")

- **Average Pace Section** (top-right):
  - `avgPaceValue`: Average pace in MM:SS format (e.g., "4:46")
  - `avgPaceText`: Label "AVG PACE"

- **Timer Section** (bottom-left):
  - `timerValue`: Elapsed time in HH:MM:SS format (e.g., "2:03:44")
  - `timerText`: Label "TIMER"

- **Track Map Section** (bottom-right):
  - `track`: PolyLine component rendering the visual track path with rounded line caps

All text elements use Poppins fonts in various weights and sizes for optimal readability on small screens.

## Functionality

The widget's core functionality centers on activity data display and track visualization:

- **Distance Display**: Accepts distance in meters, automatically converts to kilometers or miles based on imperial flag, displays with appropriate precision (1-2 decimal places)
- **Pace Display**: Accepts average pace in seconds per meter, converts to minutes per kilometer/mile, formats as MM:SS
- **Timer Display**: Accepts elapsed time in seconds, formats as HH:MM:SS
- **Track Visualization**: Accepts track map data with coordinate points, renders as a yellow polyline with configurable positioning and scaling

The widget automatically invalidates and redraws text areas when data changes, ensuring immediate visual updates. The track map uses a custom PolyLine component for efficient canvas-based rendering with smooth rounded line caps.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TrackSummary package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TrackSummary custom container from the Custom Containers panel onto your screen
   - Position as needed (recommended size accommodates all components)
   - The widget includes TrackSummary and PolyLine custom containers

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All data (distance, pace, time, track map) is controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TrackSummary.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TrackSummary trackSummary;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(trackSummary);
   trackSummary.setPosition(x, y, width, height);
   ```

4. **Update Distance**:
   ```cpp
   // Set distance in meters, imperial flag for units
   trackSummary.setDistance(23980.0f, false); // Displays "23.98 km"
   trackSummary.setDistance(15000.0f, true);  // Displays "9.32 mi."
   ```

5. **Update Average Pace**:
   ```cpp
   // Set average pace in seconds per meter, imperial flag for units
   trackSummary.setAvgPace(286.0f, false); // Displays "4:46" (min/km)
   trackSummary.setAvgPace(286.0f, true);  // Displays "7:32" (min/mi)
   ```

6. **Update Timer**:
   ```cpp
   // Set elapsed time in seconds
   trackSummary.setTimer(7424); // Displays "2:03:44"
   ```

7. **Update Track Map**:
   ```cpp
   // Set track map data
   SDK::TrackMapScreen map = getTrackData();
   trackSummary.setMap(map);
   ```

8. **Complete Example**:
   ```cpp
   void MainView::updateTrackSummary(float distance, float avgPace, uint32_t time, const SDK::TrackMapScreen& map) {
       trackSummary.setDistance(distance, isImperialUnits);
       trackSummary.setAvgPace(avgPace, isImperialUnits);
       trackSummary.setTimer(time);
       trackSummary.setMap(map);
   }
   ```

## Advanced Features and Hidden Gems

- **Custom PolyLine Rendering**: Implements sophisticated canvas-based polyline drawing with rounded line caps using Q5 fixed-point arithmetic for pixel-perfect rendering on embedded displays
- **Automatic Unit Conversion**: Seamlessly handles metric/imperial conversions for distance and pace with appropriate precision formatting
- **Efficient Track Positioning**: Automatically adjusts track map position based on coordinate bounds to ensure optimal visualization within the allocated area
- **Modular Canvas Architecture**: PolyLine component provides reusable canvas drawing capabilities with configurable line width, color, and smooth geometry
- **Memory-Efficient Text Formatting**: Uses TouchGFX's Unicode::snprintf for consistent formatting across locales with minimal memory overhead
- **Embedded-Optimized Rendering**: Canvas-based drawing reduces bitmap memory usage compared to pre-rendered track images
- **Flexible Track Data**: Accepts raw coordinate arrays for maximum compatibility with various GPS and track data sources
- **Precision Coordinate Handling**: Uses 8-bit coordinate storage for compact track data while maintaining visual accuracy through canvas scaling

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest with TrackSummary and PolyLine
- `content/texts.xml`: Text database with labels, units, and templates
- `files/gui/include/gui/containers/TrackSummary.hpp`: Header file
- `files/gui/src/containers/TrackSummary.cpp`: Implementation file
- `files/gui/include/gui/containers/PolyLine.hpp`: PolyLine header file
- `files/gui/src/containers/PolyLine.cpp`: PolyLine implementation file
- `files/assets/fonts/`: 3 Poppins font variants (Medium, Regular, SemiBold)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- PolyLine custom container for track visualization
- `SDK::TrackMapScreen` structure for track data
- `App::Utils` utilities for unit conversions and time formatting
- Standard TouchGFX container and text components