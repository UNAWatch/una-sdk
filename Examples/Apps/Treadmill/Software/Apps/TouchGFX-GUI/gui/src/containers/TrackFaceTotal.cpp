#include <gui/containers/TrackFaceTotal.hpp>
#include <texts/TextKeysAndLanguages.hpp>

TrackFaceTotal::TrackFaceTotal()
{
}

void TrackFaceTotal::initialize()
{
    TrackFaceTotalBase::initialize();
}

void TrackFaceTotal::setPace(float pace)
{
    if (pace < App::Display::kMinPace) {
        Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "---");
    } else {
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(pace));
        if (hms.h > 0) {
            Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "%u:%02u", hms.h, hms.m);
        } else {
            Unicode::snprintf(paceValueBuffer, PACEVALUE_SIZE, "%u:%02u", hms.m, hms.s);
        }
    }
    paceValue.invalidate();
}

void TrackFaceTotal::setDistance(float dist, bool isImperial)
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

void TrackFaceTotal::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
