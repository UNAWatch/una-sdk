#include <gui/containers/SummaryFaceMap.hpp>
#include <texts/TextKeysAndLanguages.hpp>

SummaryFaceMap::SummaryFaceMap()
{
}

void SummaryFaceMap::initialize()
{
    SummaryFaceMapBase::initialize();
    title.set(T_TEXT_MAP_UC);
}

void SummaryFaceMap::setDistance(float dist, bool isImperial)
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

void SummaryFaceMap::setMap(const SDK::TrackMapScreen& mapData)
{
    map.setMap(mapData);
}
