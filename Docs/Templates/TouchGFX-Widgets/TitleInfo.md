# TitleInfo

## Overview

The TitleInfo widget is a custom TouchGFX container that displays a title text with an optional value below it, accompanied by a horizontal line for visual separation. It provides flexibility in layout with options for short or long lines.

## Purpose

This widget is designed to display informational content with a clear title and optional associated value, making it ideal for settings screens, data displays, or any UI where labeled information needs to be presented in a structured format. The optional value feature allows for dynamic content display, while the line length option adapts to different screen widths.

## Components

The widget consists of the following visual elements:

- **Title Text (title)**: Displays the main title text
- **Value Text (value)**: Optional secondary text displayed below the title (can be hidden)
- **Long Line (line)**: Horizontal line for full-width separation
- **Short Line (lineShort)**: Shorter horizontal line for compact layouts

## Functionality

### Text Setting Methods

- `setTitle(TypedTextId msgId)`: Sets the title text using a TouchGFX TypedText resource ID for localization support
- `setValue(touchgfx::Unicode::UnicodeChar *msg)`: Sets the value text directly from a Unicode character array; pass `nullptr` to hide the value
- `setShortLine(bool shortLine)`: Toggles between long line (`false`) and short line (`true`) for layout flexibility

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Open TouchGFX Designer
   - Go to `Custom Containers` → `Import`
   - Select the TitleInfo package file (manifest.xml)

2. **Add to Screen**:
   - Drag the TitleInfo custom container from the Custom Containers panel onto your screen
   - Position as needed for information display

3. **Configure Properties**:
   - The widget has no configurable properties in Designer beyond standard container properties
   - All text content and line style are controlled programmatically

## Usage in C++ Code

1. **Include the Header**:
   ```cpp
   #include <gui/containers/TitleInfo.hpp>
   ```

2. **Declare in View**:
   In your View class (e.g., `MainView.hpp`):
   ```cpp
   TitleInfo titleInfoWidget;
   ```

3. **Initialize in Constructor/Setup**:
   ```cpp
   add(titleInfoWidget);
   titleInfoWidget.setPosition(x, y, width, height);
   ```

4. **Set Content**:
   ```cpp
   // Set title using TypedText resource
   titleInfoWidget.setTitle(T_MY_TITLE);
   
   // Set value (optional)
   titleInfoWidget.setValue(myValueString);
   
   // Or hide value
   titleInfoWidget.setValue(nullptr);
   
   // Set line style
   titleInfoWidget.setShortLine(true); // Use short line
   ```

5. **Example Integration**:
   ```cpp
   void MainView::updateInfoDisplay(const char* titleId, const char* value) {
       titleInfoWidget.setTitle(static_cast<TypedTextId>(titleId));
       titleInfoWidget.setValue(reinterpret_cast<touchgfx::Unicode::UnicodeChar*>(const_cast<char*>(value)));
       titleInfoWidget.invalidate();
   }
   ```

## Advanced Features and Hidden Gems

- **Optional Value Display**: The value text can be completely hidden by passing `nullptr` to `setValue()`, allowing the widget to function as a simple title with line or as a title-value pair.

- **Flexible Line Length**: The `setShortLine()` method provides two line styles - long for full-width emphasis and short for compact layouts, adapting to different UI designs.

- **Localization Support**: Title text uses TypedTextId, enabling easy internationalization and text management through TouchGFX's text system.

- **Buffer-Based Text Handling**: Uses internal buffers for text storage, ensuring consistent memory usage and avoiding dynamic allocation concerns.

- **TouchGFX 4.26.0+ Compatibility**: Built for modern TouchGFX versions with custom container architecture.

- **Efficient Updates**: Text changes trigger appropriate invalidation for smooth UI updates without unnecessary redraws.

## Files Included

- `manifest.xml`: Package manifest with TouchGFX 4.26.0+ requirement
- `content/CustomContainerExport.touchgfx`: Designer export definition
- `content/CustomContainerManifest.xml`: Container manifest
- `content/texts.xml`: Text database
- `files/gui/include/gui/containers/TitleInfo.hpp`: Header file
- `files/gui/src/containers/TitleInfo.cpp`: Implementation file
- `files/assets/fonts/`: Font files (Poppins variants) for consistent typography