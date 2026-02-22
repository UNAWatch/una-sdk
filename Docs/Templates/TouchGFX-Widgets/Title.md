# Title

## Overview

The Title widget is a simple custom TouchGFX container that displays a text title with a horizontal underline, providing a clean section header appearance for organizing UI content.

## Purpose

This widget serves as a basic text display component with visual emphasis through an underline, ideal for creating section headers, titles, or labels in TouchGFX applications. It offers a minimalist design that integrates well with various UI themes while providing programmatic control over the displayed text.

## Components

The widget consists of the following visual elements:

- **Text Area (text)**: Displays the title text in italic font with gray color
- **Underline (line)**: Horizontal line positioned below the text for visual separation

## Functionality

### Text Setting

- `set(TypedTextId msgId)`: Sets the title text using a TouchGFX TypedText resource ID
- `set(touchgfx::Unicode::UnicodeChar *msg)`: Sets the title text directly from a Unicode character array

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Import the Title package into your TouchGFX project

2. **Add to Screen/Container**:
   - Drag the Title custom container onto your screen or parent container
   - Position it at the top of sections or as needed for content organization

3. **Configuration**:
   - The widget has a fixed size (140x41 pixels) with predefined styling
   - All text content is set programmatically in C++ code

## Usage in C++ Code

### Basic Setup

```cpp
#include <gui/containers/Title.hpp>

Title title;

// Set title using TypedText resource
title.set(T_MY_TITLE_TEXT);

// Set title using Unicode string
const touchgfx::Unicode::UnicodeChar* myTitle = (touchgfx::Unicode::UnicodeChar*) "My Section";
title.set(myTitle);
```

### Dynamic Text Updates

```cpp
// Update title based on application state
void updateSectionTitle(int sectionId)
{
    switch (sectionId) {
        case 0:
            title.set(T_SECTION_ONE);
            break;
        case 1:
            title.set(T_SECTION_TWO);
            break;
        default:
            title.set((touchgfx::Unicode::UnicodeChar*) "Unknown");
            break;
    }
}
```

### Integration with Screens

```cpp
class MyScreen : public MyScreenBase
{
private:
    Title sectionTitle;

public:
    MyScreen()
    {
        // Add title to screen
        add(sectionTitle);

        // Position title at top
        sectionTitle.setXY(10, 10);

        // Set initial title
        sectionTitle.set(T_WELCOME_TITLE);
    }

    void updateTitleForContent(int contentType)
    {
        // Update title based on displayed content
        sectionTitle.set(getTitleForContentType(contentType));
    }
};
```

## Advanced Features and Hidden Gems

- **Buffer Management**: Uses a 16-character buffer for text display (supports up to 15 characters + null terminator)
- **Italic Typography**: Employs italic font styling for visual distinction
- **Color Scheme**: Consistent gray color palette (text: RGB 192,192,192; line: RGB 64,64,64)
- **Round Line Endings**: Underline features rounded caps for polished appearance
- **Wildcard Support**: Text area configured with wildcard buffer for dynamic content
- **Fixed Layout**: Predefined positioning ensures consistent visual hierarchy
- **Memory Efficient**: No dynamic allocation, uses static buffer and components
- **Unicode Compatible**: Supports international character sets through Unicode

## Technical Details

- **Base Class**: Inherits from `TitleBase` (generated container)
- **Minimum TouchGFX Version**: 4.26.0
- **Dimensions**: 140x41 pixels (fixed size)
- **Text Buffer Size**: 16 characters maximum
- **Font**: TMP_ITALIC_18 (18pt italic)
- **Text Color**: Light gray (RGB 192,192,192)
- **Line Color**: Dark gray (RGB 64,64,64)
- **Line Width**: 10 pixels with round endings

## Best Practices

1. **Text Length**: Keep titles concise (under 16 characters) to avoid truncation
2. **TypedText Usage**: Prefer TypedText IDs for localization support
3. **Positioning**: Place at section tops for clear content organization
4. **Color Consistency**: Works well with gray-themed UIs
5. **Null Safety**: Always check Unicode pointers before calling set()
6. **Localization**: Use TypedText for multi-language applications
7. **Visual Hierarchy**: Use consistently across similar sections

## Limitations

- Fixed dimensions (140x41 pixels)
- Maximum 15 characters displayable
- Predefined color scheme (not customizable)
- Italic font only
- No built-in animation or interaction
- Requires manual text buffer management
- No automatic text wrapping

This widget provides a simple yet effective solution for adding labeled sections to TouchGFX interfaces, offering clean typography and consistent visual styling for content organization.