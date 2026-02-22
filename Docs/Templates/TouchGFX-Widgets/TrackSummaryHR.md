# TrackSummaryHR

## Overview

The TrackSummaryHR widget is a custom container component for TouchGFX designed to display heart rate summary information including maximum and average heart rate values. It presents fitness tracking heart rate data in a clean, organized layout optimized for wearable device interfaces and activity summary screens.

## Purpose

The primary purpose of the TrackSummaryHR widget is to provide a focused overview of heart rate metrics from completed fitness activities in embedded applications. It displays key heart rate statistics (maximum and average BPM) with clear labeling and visual elements, making it ideal for post-activity summaries, workout history screens, and performance tracking interfaces in wearable fitness devices.

## Components

The widget consists of the following visual components arranged in a structured layout:

- **Maximum Heart Rate Section**:
  - `maxHrLabel`: Label "MAX HR"
  - `maxHrValue`: Maximum heart rate value in numeric format (e.g., "165")

- **Average Heart Rate Section**:
  - `avgHrLabel`: Label "AVG HR"
  - `avgHrValue`: Average heart rate value in numeric format (e.g., "117")

- **Heart Icon**: Visual heart symbol (Heart.png) for intuitive heart rate representation

All text elements use Poppins fonts in Regular and SemiBold weights for optimal readability on small screens.

## Functionality

The widget's core functionality centers on heart rate data display:

- **Maximum HR Display**: Accepts maximum heart rate as a float value, formats as integer BPM (beats per minute)
- **Average HR Display**: Accepts average heart rate as a float value, formats as integer BPM

The widget automatically invalidates and redraws text areas when data changes, ensuring immediate visual updates. Heart rate values are displayed with no decimal places for clean, readable presentation.

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TrackSummaryHR package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TrackSummaryHR custom container from the Custom Containers panel onto your screen
   - Position as needed (recommended size accommodates all components)

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All data (max HR, avg HR) is controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TrackSummaryHR.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TrackSummaryHR trackSummaryHR;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(trackSummaryHR);
   trackSummaryHR.setPosition(x, y, width, height);
   ```

4. **Update Maximum Heart Rate**:
   ```cpp
   // Set maximum heart rate in BPM
   trackSummaryHR.setMaxHR(165.0f); // Displays "165"
   ```

5. **Update Average Heart Rate**:
   ```cpp
   // Set average heart rate in BPM
   trackSummaryHR.setAvgHR(117.0f); // Displays "117"
   ```

6. **Complete Example**:
   ```cpp
   void MainView::updateHeartRateSummary(float maxHR, float avgHR) {
       trackSummaryHR.setMaxHR(maxHR);
       trackSummaryHR.setAvgHR(avgHR);
   }
   ```

## Advanced Features and Hidden Gems

- **Unicode-Compatible Formatting**: Uses TouchGFX's Unicode::snprintfFloat for consistent formatting across locales with minimal memory overhead
- **Integer BPM Display**: Formats heart rate values as integers (no decimals) for clean, medical-grade presentation typical of heart rate monitors
- **Efficient Text Invalidation**: Automatically invalidates only changed text areas for optimal rendering performance
- **Localization Ready**: Text labels support internationalization through TouchGFX's text database system
- **Embedded-Optimized Design**: Minimal memory footprint with vector fonts and efficient text rendering
- **Wearable-Focused UI**: Clean, high-contrast design optimized for small screens and quick readability during activities

## Files Included

- `manifest.xml`: Package manifest
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database with labels and templates
- `files/gui/include/gui/containers/TrackSummaryHR.hpp`: Header file
- `files/gui/src/containers/TrackSummaryHR.cpp`: Implementation file
- `files/assets/images/Heart.png`: Heart icon image
- `files/assets/fonts/`: 2 Poppins font variants (Regular, SemiBold)

## Dependencies

- TouchGFX Framework (minimum version 4.26.0)
- Standard TouchGFX container and text components