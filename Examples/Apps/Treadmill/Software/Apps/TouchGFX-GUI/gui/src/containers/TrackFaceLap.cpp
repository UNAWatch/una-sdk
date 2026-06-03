#include <gui/containers/TrackFaceLap.hpp>

TrackFaceLap::TrackFaceLap()
{
}

void TrackFaceLap::initialize()
{
    TrackFaceLapBase::initialize();
}

void TrackFaceLap::setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount)
{
    if (hr < App::Display::kMinHR) {
        Unicode::snprintf(hrValueBuffer, HRVALUE_SIZE, "---");
        hrValue.invalidate();
        hrZone.setHR(0, thresholds, thresholdCount);
        return;
    }

    Unicode::snprintfFloat(hrValueBuffer, HRVALUE_SIZE, "%.0f", hr);
    hrValue.invalidate();

    hrZone.setHR(static_cast<uint8_t>(hr), thresholds, thresholdCount);
}

void TrackFaceLap::setPace(float pace)
{
    if (pace < App::Display::kMinPace) {
        Unicode::snprintf(lapPaceValueBuffer, LAPPACEVALUE_SIZE, "---");
    } else {
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(pace));
        if (hms.h > 0) {
            Unicode::snprintf(lapPaceValueBuffer, LAPPACEVALUE_SIZE, "%u:%02u", hms.h, hms.m);
        } else {
            Unicode::snprintf(lapPaceValueBuffer, LAPPACEVALUE_SIZE, "%u:%02u", hms.m, hms.s);
        }
    }
    lapPaceValue.invalidate();
}

void TrackFaceLap::setDistance(float dist)
{
    if (dist < App::Display::kMinDist) {
        Unicode::snprintf(lapDistValueBuffer, LAPDISTVALUE_SIZE, "---");
    } else if (dist < 10.0f) {
        Unicode::snprintfFloat(lapDistValueBuffer, LAPDISTVALUE_SIZE, "%.02f", dist);
    } else if (dist < 100.0f) {
        Unicode::snprintfFloat(lapDistValueBuffer, LAPDISTVALUE_SIZE, "%.01f", dist);
    } else {
        Unicode::snprintfFloat(lapDistValueBuffer, LAPDISTVALUE_SIZE, "%.0f", dist);
    }
    lapDistValue.invalidate();
}

void TrackFaceLap::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(lapTimerValueBuffer, LAPTIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    lapTimerValue.invalidate();
}
