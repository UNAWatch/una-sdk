# SideBarBig

## Overview

The SideBarBig widget is a custom TouchGFX container that provides a curved sidebar visual indicator for menu or list navigation. It displays a handle that moves along an arc-shaped rail to represent the current position among multiple items, offering smooth animated transitions between positions.

## Purpose

This widget serves as a visual position indicator in scrollable menus, lists, or multi-page interfaces. The curved design provides an elegant way to show progress through a sequence of items, with the handle smoothly animating along the arc to indicate the active item. It's particularly useful in wearable or embedded UI designs where space is limited and visual feedback needs to be compact yet informative.

## Components

The widget consists of the following visual elements:

- **Rail (rail)**: Background image displaying the curved track (SideBarBig.png)
- **Handle (handle)**: Circular arc segment representing the current position
- **Overflow Handle (handleOvf)**: Secondary arc segment used during wrap-around animations

## Functionality

### Position Management

- `setCount(uint16_t cnt)`: Sets the total number of items in the sequence
- `setActiveId(uint16_t p)`: Sets the current active position (0-based index)
- `getActiveId()`: Returns the current active position
- `animateToId(int16_t p, int16_t animationSteps = -1)`: Animates the handle to a new position with optional step count

### Animation Control

- `handleTickEvent()`: Advances animation frames (called automatically via timer registration)
- Automatic timer management for smooth animations
- Wrap-around support when animating beyond bounds

## Usage in TouchGFX Designer

1. **Import the Custom Container Package**:
   - Import the SideBarBig package into your TouchGFX project

2. **Add to Screen/Container**:
   - Drag the SideBarBig custom container onto your screen or parent container
   - Position it appropriately (typically on the right side for vertical navigation)

3. **Configuration**:
   - All configuration is done programmatically in C++ code
   - The widget appears as a 36x146 pixel curved sidebar

## Usage in C++ Code

### Basic Setup

```cpp
#include <gui/containers/SideBarBig.hpp>

SideBarBig sideBar;

// Initialize with 5 items
sideBar.setCount(5);

// Set current position to first item
sideBar.setActiveId(0);
```

### Position Updates

```cpp
// Immediate position change
sideBar.setActiveId(2);

// Animated transition (default steps)
sideBar.animateToId(3);

// Animated with custom duration
sideBar.animateToId(1, 20); // 20 animation steps

// Get current position
uint16_t currentPos = sideBar.getActiveId();
```

### Integration with Menu Systems

```cpp
// In a menu or list container
class MyMenu : public Container
{
private:
    SideBarBig sideBar;
    // ... other components

public:
    void setupItems(uint16_t itemCount)
    {
        sideBar.setCount(itemCount);
        sideBar.setActiveId(0);
    }

    void selectItem(uint16_t index)
    {
        sideBar.animateToId(index);
        // Update other UI elements...
    }
};
```

### Dynamic Count Changes

```cpp
// Update item count dynamically
sideBar.setCount(newItemCount);

// Handle position if it exceeds new count
if (sideBar.getActiveId() >= newItemCount) {
    sideBar.setActiveId(newItemCount - 1);
}
```

## Advanced Features and Hidden Gems

- **Smooth Animation**: Timer-based animation system with configurable step count for fluid transitions
- **Wrap-Around Animation**: Seamless animation when moving beyond sequence bounds (e.g., from last to first item)
- **Overflow Visual Effect**: Secondary handle appears during wrap-around for enhanced visual feedback
- **Automatic Handle Hiding**: Handle automatically hides when there's only one item (no navigation needed)
- **Angle-Based Positioning**: Precise arc positioning using degree calculations (232° to 308° range)
- **Memory Efficient**: No dynamic memory allocation, uses fixed-size components
- **Timer Integration**: Automatic registration/unregistration with TouchGFX timer system
- **Invalidation Management**: Proper invalidation of visual components during updates

## Technical Details

- **Base Class**: Inherits from `SideBarBigBase` (generated container)
- **Minimum TouchGFX Version**: 4.26.0
- **Dimensions**: 36x146 pixels (configurable in designer)
- **Animation Range**: 76° arc (232° to 308°)
- **Handle Length**: 18° arc segment
- **Timer Widget**: Implements `handleTickEvent()` for animation
- **Wrap-Around Logic**: Handles negative indices and overflow by wrapping to opposite end

## Best Practices

1. **Item Count**: Always call `setCount()` before setting positions
2. **Bounds Checking**: Verify position is within valid range before calling `setActiveId()`
3. **Animation Steps**: Use appropriate step counts (10-30) for smooth but not sluggish animation
4. **Position Synchronization**: Keep sidebar position synchronized with actual menu/list selection
5. **Single Item Handling**: Widget gracefully handles single-item scenarios by hiding the handle
6. **Performance**: Avoid excessive animation calls; use immediate updates for rapid changes

## Limitations

- Fixed arc geometry (232°-308° range)
- Designed for vertical curved sidebar layout
- No built-in touch interaction (position controlled programmatically)
- Animation steps must be positive integers
- Handle visibility tied to item count (>1)
- Requires manual synchronization with data models

This widget provides an elegant solution for position indication in curved, space-constrained interfaces, offering smooth animations and intuitive visual feedback for navigation through item sequences.