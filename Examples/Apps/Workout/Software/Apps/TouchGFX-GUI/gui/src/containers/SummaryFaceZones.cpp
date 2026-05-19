#include <gui/containers/SummaryFaceZones.hpp>

#include <touchgfx/Color.hpp>

namespace {

/// Bar colours indexed by 0..4 (zone 1..5).  Matches the in-activity HR-zone
/// indicator palette: cyan -> green -> yellow -> orange -> red.
const touchgfx::colortype kZoneColors[5] = {
    touchgfx::Color::getColorFromRGB(  0, 192, 192), // Zone 1 - cyan
    touchgfx::Color::getColorFromRGB( 64, 192,   0), // Zone 2 - green
    touchgfx::Color::getColorFromRGB(192, 192,   0), // Zone 3 - yellow
    touchgfx::Color::getColorFromRGB(192, 128,   0), // Zone 4 - orange
    touchgfx::Color::getColorFromRGB(192,   0,   0), // Zone 5 - red
};

} // namespace

SummaryFaceZones::SummaryFaceZones()
{
}

void SummaryFaceZones::initialize()
{
    SummaryFaceZonesBase::initialize();
    title.set("HR ZONES");
}

void SummaryFaceZones::setZoneSummary(const ActivitySummary& s)
{
    std::time_t inZones = 0;
    for (std::time_t z : s.zoneTimeSec) {
        inZones += z;
    }

    // Active seconds with HR below the zone-1 threshold get folded into the
    // zone-1 row purely for display; the underlying telemetry is untouched.
    const std::time_t belowZone1 = (s.time > inZones) ? (s.time - inZones) : 0;
    const std::time_t displaySec[5] = {
        s.zoneTimeSec[0] + belowZone1,
        s.zoneTimeSec[1],
        s.zoneTimeSec[2],
        s.zoneTimeSec[3],
        s.zoneTimeSec[4],
    };

    const float totalSec = (s.time > 0) ? static_cast<float>(s.time) : 0.0f;

    ZoneInfo* const rows[5] = {
        &zoneInfo1, &zoneInfo2, &zoneInfo3, &zoneInfo4, &zoneInfo5,
    };

    for (size_t i = 0; i < 5; ++i) {
        const float pct = (totalSec > 0.0f)
            ? (100.0f * static_cast<float>(displaySec[i]) / totalSec)
            : 0.0f;
        rows[i]->setRow(static_cast<uint8_t>(i + 1), displaySec[i], pct, kZoneColors[i]);
    }
}
