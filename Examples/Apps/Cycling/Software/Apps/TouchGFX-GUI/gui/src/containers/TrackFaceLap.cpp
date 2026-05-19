#include <gui/containers/TrackFaceLap.hpp>

TrackFaceLap::TrackFaceLap()
{

}

void TrackFaceLap::initialize()
{
    TrackFaceLapBase::initialize();
}

void TrackFaceLap::setSpeed(float speed, bool isImperial)
{
    if (speed < App::Display::kMinSpeed) {
        Unicode::snprintf(speedValueBuffer, SPEEDVALUE_SIZE, "---");
    } else if (speed < 10.0f) {
        Unicode::snprintfFloat(speedValueBuffer, SPEEDVALUE_SIZE, "%.02f", speed);
    } else if (speed < 100.0f) {
        Unicode::snprintfFloat(speedValueBuffer, SPEEDVALUE_SIZE, "%.01f", speed);
    } else {
        Unicode::snprintfFloat(speedValueBuffer, SPEEDVALUE_SIZE, "%.0f", speed);
    }

    Unicode::snprintf(speedUnitsBuffer, SPEEDUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_MI_PER_H : T_TEXT_KM_PER_H).getText());

    speedValue.invalidate();
    speedUnits.invalidate();
}

void TrackFaceLap::setDistance(float dist, bool isImperial)
{
    if (dist < App::Display::kMinDist) {
        Unicode::snprintf(distanceValueBuffer, DISTANCEVALUE_SIZE, "---");
    } else if (dist < 100.0f) {
        Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.02f", dist);
    } else {
        Unicode::snprintfFloat(distanceValueBuffer, DISTANCEVALUE_SIZE, "%.01f", dist);
    }

    Unicode::snprintf(distanceUnitsBuffer, DISTANCEUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_MI : T_TEXT_KM).getText());

    distanceValue.invalidate();
    distanceUnits.invalidate();
}

void TrackFaceLap::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
