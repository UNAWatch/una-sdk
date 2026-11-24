#include <gui/containers/TrackFacePace.hpp>
#include <texts/TextKeysAndLanguages.hpp>

namespace
{
// The lap and totals faces' pace format: "---" below the display minimum,
// otherwise m:ss rounded to the nearest second.
void formatPace(Unicode::UnicodeChar* buffer, uint16_t size, float pace)
{
    if (pace < App::Display::kMinPace) {
        Unicode::snprintf(buffer, size, "---");
    } else {
        // Round to the nearest second (matches the lap-alert popup / summary
        // pace; truncating would read a second low for the same grid lap).
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(pace + 0.5f));
        // Under an hour: the service sends no pace below 0.5 m/s.
        Unicode::snprintf(buffer, size, "%u:%02u", hms.m, hms.s);
    }
}
} // namespace

TrackFacePace::TrackFacePace()
{
}

void TrackFacePace::initialize()
{
    TrackFacePaceBase::initialize();
}

void TrackFacePace::setDistance(float dist, bool isImperial)
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

void TrackFacePace::setAvgLap(float pace)
{
    formatPace(avgLapValueBuffer, AVGLAPVALUE_SIZE, pace);
    avgLapValue.invalidate();
}

void TrackFacePace::setAvgPace(float pace)
{
    formatPace(avgPaceValueBuffer, AVGPACEVALUE_SIZE, pace);
    avgPaceValue.invalidate();
}

void TrackFacePace::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
