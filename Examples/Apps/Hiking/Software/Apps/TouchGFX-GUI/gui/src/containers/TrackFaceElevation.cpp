#include <gui/containers/TrackFaceElevation.hpp>

TrackFaceElevation::TrackFaceElevation()
{

}

void TrackFaceElevation::initialize()
{
    TrackFaceElevationBase::initialize();
}

void TrackFaceElevation::setLapAscent(float ascent, bool isImperial)
{
    Unicode::snprintf(lapAscentValueBuffer, LAPASCENTVALUE_SIZE, "%d", static_cast<int32_t>(ascent));

    Unicode::snprintf(lapAscentUnitsBuffer, LAPASCENTUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    lapAscentValue.invalidate();
    lapAscentUnits.invalidate();
}

void TrackFaceElevation::setTotalAscent(float ascent, bool isImperial)
{
    Unicode::snprintf(totalAscentValueBuffer, TOTALASCENTVALUE_SIZE, "%d", static_cast<int32_t>(ascent));

    Unicode::snprintf(totalAscentUnitsBuffer, TOTALASCENTUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    totalAscentValue.invalidate();
    totalAscentUnits.invalidate();
}

void TrackFaceElevation::setElevation(float elevation, bool isImperial)
{
    Unicode::snprintf(elevationValueBuffer, ELEVATIONVALUE_SIZE, "%d", static_cast<int32_t>(elevation));

    Unicode::snprintf(elevationUnitsBuffer, ELEVATIONUNITS_SIZE, "%s",
        touchgfx::TypedText(isImperial ? T_TEXT_FT : T_TEXT_M).getText());

    elevationValue.invalidate();
    elevationUnits.invalidate();
}
