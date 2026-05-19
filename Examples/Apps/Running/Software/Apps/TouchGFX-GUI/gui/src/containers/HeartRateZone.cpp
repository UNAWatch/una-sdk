#include <gui/containers/HeartRateZone.hpp>

HeartRateZone::HeartRateZone()
    : mZones{&hr1, &hr2, &hr3, &hr4, &hr5}
{
}

void HeartRateZone::initialize()
{
    HeartRateZoneBase::initialize();
}

void HeartRateZone::setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount)
{
    if (thresholds == nullptr || thresholdCount == 0) {
        return;
    }

    if (thresholdCount > kZoneCount) {
        thresholdCount = kZoneCount;
    }

    for (touchgfx::Image* zone : mZones) {
        zone->setVisible(false);
    }

    // Walk thresholds from highest to lowest; the first threshold that hr exceeds
    // determines the active zone. activeZone starts at the top and decrements
    // until a matching threshold is found (or goes negative -- no zone shown).
    int activeZone = kZoneCount - 1;
    for (int i = thresholdCount - 1; i >= 0; --i) {
        if (hr > thresholds[i]) {
            break;
        }
        --activeZone;
    }

    if (activeZone >= 0) {
        mZones[activeZone]->setVisible(true);
    }

    for (touchgfx::Image* zone : mZones) {
        zone->invalidate();
    }
}
