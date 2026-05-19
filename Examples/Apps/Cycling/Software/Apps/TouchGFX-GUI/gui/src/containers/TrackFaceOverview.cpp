#include <gui/containers/TrackFaceOverview.hpp>

TrackFaceOverview::TrackFaceOverview()
{

}

void TrackFaceOverview::initialize()
{
    TrackFaceOverviewBase::initialize();
}

void TrackFaceOverview::setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount)
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

void TrackFaceOverview::setAvgSpeed(float speed, bool isImperial)
{
    if (speed < App::Display::kMinSpeed) {
        Unicode::snprintf(avgSpeedValueBuffer, AVGSPEEDVALUE_SIZE, "---");
    } else if (speed < 10.0f) {
        Unicode::snprintfFloat(avgSpeedValueBuffer, AVGSPEEDVALUE_SIZE, "%.02f", speed);
    } else if (speed < 100.0f) {
        Unicode::snprintfFloat(avgSpeedValueBuffer, AVGSPEEDVALUE_SIZE, "%.01f", speed);
    } else {
        Unicode::snprintfFloat(avgSpeedValueBuffer, AVGSPEEDVALUE_SIZE, "%.0f", speed);
    }

    Unicode::snprintf(avgSpeedUnitsBuffer, AVGSPEEDUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_MI_PER_H : T_TEXT_KM_PER_H).getText());

    avgSpeedValue.invalidate();
    avgSpeedUnits.invalidate();
}

void TrackFaceOverview::setElevation(float elevation, bool isImperial)
{
    Unicode::snprintf(elevationValueBuffer, ELEVATIONVALUE_SIZE, "%d", static_cast<int32_t>(elevation));

    Unicode::snprintf(elevationUnitsBuffer, ELEVATIONUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    elevationValue.invalidate();
    elevationUnits.invalidate();
}

