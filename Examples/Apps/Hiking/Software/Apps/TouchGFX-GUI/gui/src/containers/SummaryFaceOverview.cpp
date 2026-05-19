#include <gui/containers/SummaryFaceOverview.hpp>
#include <texts/TextKeysAndLanguages.hpp>

SummaryFaceOverview::SummaryFaceOverview()
{
}

void SummaryFaceOverview::initialize()
{
    SummaryFaceOverviewBase::initialize();
    title.set(T_TEXT_SUMMARY_UC);
}

void SummaryFaceOverview::setDistance(float dist, bool isImperial)
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

void SummaryFaceOverview::setElevation(float elevation, bool isImperial)
{
    Unicode::snprintf(elevationValueBuffer, ELEVATIONVALUE_SIZE, "%d", static_cast<int32_t>(elevation));

    Unicode::snprintf(elevationUnitsBuffer, ELEVATIONUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    elevationValue.invalidate();
    elevationUnits.invalidate();
}

void SummaryFaceOverview::setTimer(std::time_t sec)
{
    auto hms = SDK::Utils::toHMS(sec);
    Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
        "%u:%02u:%02u", hms.h, hms.m, hms.s);
    timerValue.invalidate();
}
