#ifndef MAP_HPP
#define MAP_HPP

#include <gui_generated/containers/MapBase.hpp>
#include <gui/containers/PolyLine.hpp>
#include <SDK/TrackMap/TrackMapScreen.hpp>

/**
 * @brief Canvas container that renders a GPS track as a PolyLine with start/end dot markers.
 *
 * Draws the recorded route as a coloured polyline centred inside a 150x150 area.
 * A 4 px inset on every side ensures that 8x8 dot markers placed at extreme
 * coordinates (0, 0) are never clipped by the container boundary.
 *
 * Usage:
 * @code
 *   map.setMap(summaryData.map);
 * @endcode
 */
class Map : public MapBase
{
public:
    Map();
    virtual ~Map() {}

    virtual void initialize();

    /**
     * @brief Render a track from pre-built map screen data.
     *
     * Positions the PolyLine and start/end dot markers so the route content is
     * centred inside the container regardless of the track's bounding box aspect
     * ratio.  Calling with an empty @p map is a no-op.
     *
     * @param map Pre-scaled map screen data produced by SDK::TrackMapBuilder::build().
     */
    void setMap(const SDK::TrackMapScreen& map);

protected:
    PolyLine track;
};

#endif // MAP_HPP
