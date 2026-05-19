#include <gui/containers/TrackFaceTotal.hpp>

TrackFaceTotal::TrackFaceTotal()
{
}

void TrackFaceTotal::initialize()
{
    TrackFaceTotalBase::initialize();
}

void TrackFaceTotal::setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount)
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

void TrackFaceTotal::setCalories(float calories)
{
    Unicode::snprintfFloat(caloriesValueBuffer, CALORIESVALUE_SIZE, "%.0f", calories);
    caloriesValue.invalidate();
}

void TrackFaceTotal::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
