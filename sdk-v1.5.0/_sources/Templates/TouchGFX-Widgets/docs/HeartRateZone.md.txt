# HeartRateZone

## Overview

The HeartRateZone widget displays a segmented arc indicator that highlights the current heart rate training zone. One of five zone images is shown at a time based on the current HR value measured against a configurable threshold array.

## Components

| Name | Type | Description |
|------|------|-------------|
| `group` | Image | `HeartRateZoneGroup.png` — full arc background (all zones inactive) |
| `hr1` | Image | Zone 1 segment (lowest, leftmost) |
| `hr2` | Image | Zone 2 segment |
| `hr3` | Image | Zone 3 segment (center) |
| `hr4` | Image | Zone 4 segment |
| `hr5` | Image | Zone 5 segment (highest, rightmost) |

Container size: **210×71 px**, positioned at X=15 (centered on a 240 px screen).

All zone images are hidden by default. `group` is always visible as the base arc.

## Zone Detection Logic

`setHR()` walks the threshold array from the highest value downward and activates the first zone whose lower-bound threshold is exceeded by the current HR. If HR is below all thresholds, no zone segment is shown (only the background group).

| Level | Active zone |
|-------|-------------|
| hr > thresholds[4] | hr5 (zone 5) |
| hr > thresholds[3] | hr4 |
| hr > thresholds[2] | hr3 |
| hr > thresholds[1] | hr2 |
| hr > thresholds[0] | hr1 |
| hr ≤ thresholds[0] | none |

## API

```cpp
void setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount);
```

- `hr` — current heart rate in bpm
- `thresholds` — ascending array of zone lower-bound values (bpm), up to 5 entries
- `thresholdCount` — number of entries in the array (clamped to 5)

## Usage in TouchGFX Designer

1. **Import the package**:
   - Open TouchGFX Designer
   - Go to `Edit` → `Import` → `Custom Containers`
   - Select `HeartRateZone.tpkg`

2. **Add to screen**: drag the `HeartRateZone` container onto your screen. Default position is X=15.

3. **No designer properties** — zone is controlled programmatically.

## Usage in C++ Code

```cpp
#include <gui/containers/HeartRateZone.hpp>

HeartRateZone hrZone;
add(hrZone);
hrZone.setPosition(15, y, 210, 71);

// Define zone thresholds (bpm), ascending order
static constexpr uint8_t kHrThresholds[] = { 90, 110, 130, 150, 170 };

// Update on each HR measurement
hrZone.setHR(currentBpm, kHrThresholds, 5);
```

## Files Included

- `manifest.xml` — package manifest (requires TouchGFX ≥ 4.26.1)
- `content/CustomContainerExport.touchgfx` — Designer container definition
- `content/CustomContainerManifest.xml` — container manifest
- `files/gui/include/gui/containers/HeartRateZone.hpp` — header
- `files/gui/src/containers/HeartRateZone.cpp` — implementation
- `files/assets/images/HeartRateZoneGroup.png` — background arc
- `files/assets/images/HeartRateZone1.png` … `HeartRateZone5.png` — zone segments

## Dependencies

- TouchGFX Framework (minimum version 4.26.1)
- 6 PNG image assets (bundled in package)
