#ifndef SUMMARYFACEMAP_HPP
#define SUMMARYFACEMAP_HPP

#include <gui_generated/containers/SummaryFaceMapBase.hpp>
#include <SDK/TrackMap/TrackMapScreen.hpp>

/**
 * @brief Summary face showing the GPS track map and total distance.
 *
 * One of four swipeable faces on the TrackSummary screen.
 * All values are display-ready -- unit conversion is the view's responsibility.
 */
class SummaryFaceMap : public SummaryFaceMapBase
{
public:
    SummaryFaceMap();
    virtual ~SummaryFaceMap() {}

    virtual void initialize();

    /**
     * @brief Display total distance with unit label.
     * @param dist       Already-converted value in km or mi (view's responsibility).
     *                   Pass a negative value (< 0) to show "---" (no GPS / no data).
     * @param isImperial Selects unit label: "mi" if true, "km" if false.
     */
    void setDistance(float dist, bool isImperial);

    /**
     * @brief Render the recorded GPS route on the embedded Map container.
     * @param map Pre-scaled map screen data produced by SDK::TrackMapBuilder::build().
     *            Passing empty map data is a no-op.
     */
    void setMap(const SDK::TrackMapScreen& map);

protected:
};

#endif // SUMMARYFACEMAP_HPP
