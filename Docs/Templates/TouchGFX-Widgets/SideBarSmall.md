# SideBarSmall

## Overview

The SideBarSmall widget is a compact custom TouchGFX container that provides a curved sidebar visual indicator for menu or list navigation. It displays a handle that moves along a smaller arc-shaped rail to represent the current position among multiple items, offering smooth animated transitions between positions.

## Purpose

This widget serves as a space-efficient visual position indicator in scrollable menus, lists, or multi-page interfaces. The compact curved design provides an elegant way to show progress through a sequence of items, with the handle smoothly animating along the arc to indicate the active item. It's particularly useful in wearable or embedded UI designs where space is at a premium and visual feedback needs to be both compact and informative.

## Components

The widget consists of the following visual elements:

- **Rail (rail)**: Background image displaying the curved track (SideBarSmall.png)
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
   - Import the SideBarSmall package into your TouchGFX project

2. **Add to Screen/Container**:
   - Drag the SideBarSmall custom container onto your screen or parent container
   - Position it appropriately (typically on the right side for vertical navigation)

3. **Configuration**:
   - All configuration is done programmatically in C++ code
   - The widget appears as a compact curved sidebar

## Usage in C++ Code

### Basic Setup

```cpp
#include <gui/containers/SideBarSmall.hpp>

SideBarSmall sideBar;

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
    SideBarSmall sideBar;
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
- **Angle-Based Positioning**: Precise arc positioning using degree calculations (252° to 288° range)
- **Memory Efficient**: No dynamic memory allocation, uses fixed-size components
- **Timer Integration**: Automatic registration/unregistration with TouchGFX timer system
- **Invalidation Management**: Proper invalidation of visual components during updates
- **Compact Design**: Smaller footprint compared to SideBarBig, ideal for constrained layouts

## Technical Details

- **Base Class**: Inherits from `SideBarSmallBase` (generated container)
- **Minimum TouchGFX Version**: 4.26.0
- **Animation Range**: 36° arc (252° to 288°)
- **Handle Length**: 10° arc segment
- **Timer Widget**: Implements `handleTickEvent()` for animation
- **Wrap-Around Logic**: Handles negative indices and overflow by wrapping to opposite end

## Best Practices

1. **Item Count**: Always call `setCount()` before setting positions
2. **Bounds Checking**: Verify position is within valid range before calling `setActiveId()`
3. **Animation Steps**: Use appropriate step counts (10-30) for smooth but not sluggish animation
4. **Position Synchronization**: Keep sidebar position synchronized with actual menu/list selection
5. **Single Item Handling**: Widget gracefully handles single-item scenarios by hiding the handle
6. **Performance**: Avoid excessive animation calls; use immediate updates for rapid changes
7. **Space Constraints**: Use SideBarSmall when vertical space is limited compared to SideBarBig

## Limitations

- Fixed arc geometry (252°-288° range)
- Designed for vertical curved sidebar layout
- No built-in touch interaction (position controlled programmatically)
- Animation steps must be positive integers
- Handle visibility tied to item count (>1)
- Requires manual synchronization with data models

This widget provides an elegant, space-efficient solution for position indication in curved, space-constrained interfaces, offering smooth animations and intuitive visual feedback for navigation through item sequences.