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

void TrackFaceOverview::setPace(float pace)
{
    if (pace < App::Display::kMinPace) {
        Unicode::snprintf(avgPaceValueBuffer, AVGPACEVALUE_SIZE, "---");
    } else {
        auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(pace));
        // Under an hour: the service sends no pace below 0.5 m/s.
        Unicode::snprintf(avgPaceValueBuffer, AVGPACEVALUE_SIZE, "%u:%02u", hms.m, hms.s);
    }
    avgPaceValue.invalidate();
}

void TrackFaceOverview::setElevation(float elevation, bool isImperial)
{
    Unicode::snprintf(elevationValueBuffer, ELEVATIONVALUE_SIZE, "%d", static_cast<int32_t>(elevation));

    Unicode::snprintf(elevationUnitsBuffer, ELEVATIONUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    elevationValue.invalidate();
    elevationUnits.invalidate();
}
