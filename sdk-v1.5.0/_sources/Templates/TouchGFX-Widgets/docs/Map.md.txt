# Map

## Overview

The Map widget renders a GPS route as a colored polyline with start and end dot markers inside a 150×150 px canvas. It takes pre-built map data from `SDK::TrackMapBuilder::build()` and handles centering the route within the container automatically.

## Components

| Name | Type | Description |
|------|------|-------------|
| `track` | PolyLine | Route path, amber `#C08000`, 3 px width, inserted behind dots |
| `dotStart` | Circle (filled) | Green `#00C040`, 8×8 px — route start point |
| `dotEnd` | Circle (filled) | Red `#C00000`, 8×8 px — route end point |

Container size: **150×150 px**.

The PolyLine draws within a 142×142 px inset area (4 px margin on all sides) so that 8×8 dot markers placed at extreme coordinates are never clipped by the container boundary.

## PolyLine

`PolyLine` is a `CanvasWidget` subclass bundled with the Map package. It draws a series of line segments with rounded caps using the TouchGFX Canvas renderer.

```cpp
// Point array format: [x0, y0, x1, y1, ..., xN, yN] (uint8_t pairs)
void setPoints(const uint8_t* points, uint32_t num);  // num = point count (not byte count)
void setLineWidth(int16_t w);
void setColor(touchgfx::colortype color);
```

`PolyLine` is not used directly — it is a protected member of `Map` and managed internally.

## API

```cpp
void setMap(const SDK::TrackMapScreen& map);
```

`SDK::TrackMapScreen` is produced by `SDK::TrackMapBuilder::build()`. It contains:
- `points` — pre-scaled coordinate array fit to the drawing area
- `minx`, `miny`, `maxx`, `maxy` — bounding box of the scaled route

Calling `setMap()` with an empty point array is a no-op.

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `Map.tpkg`

2. **Add to screen**: drag the `Map` container onto your screen and position it.

3. **No designer properties** — map data is set programmatically.

## Usage in C++ Code

```cpp
#include <gui/containers/Map.hpp>
#include <SDK/TrackMap/TrackMapBuilder.hpp>

Map map;
add(map);
map.setPosition(x, y, 150, 150);

// Build map from GPS track data (SDK helper)
SDK::TrackMapScreen mapScreen = SDK::TrackMapBuilder::build(
    trackPoints,      // std::vector<GPS::Point>
    trackPointCount,
    142, 142          // drawing area dimensions
);

// Render
map.setMap(mapScreen);
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/Map.hpp` — Map header
- `files/gui/src/containers/Map.cpp` — Map implementation
- `files/gui/include/gui/containers/PolyLine.hpp` — PolyLine header
- `files/gui/src/containers/PolyLine.cpp` — PolyLine implementation

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- `SDK::TrackMapBuilder` / `SDK::TrackMapScreen` (UNA SDK)
- No image assets — fully vector-based
