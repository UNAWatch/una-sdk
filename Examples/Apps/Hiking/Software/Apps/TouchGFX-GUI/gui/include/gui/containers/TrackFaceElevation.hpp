#ifndef TRACKFACEELEVATION_HPP
#define TRACKFACEELEVATION_HPP

#include <gui_generated/containers/TrackFaceElevationBase.hpp>

/**
 * @brief Track face showing lap/total ascent and current elevations.
 *
 * All values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceElevation : public TrackFaceElevationBase
{
public:
    TrackFaceElevation();
    virtual ~TrackFaceElevation() {}

    virtual void initialize();

    /**
     * @brief Display lap ascent.
     * @param ascent     Already-converted value in m or ft (view's responsibility).
     * @param isImperial Selects unit label: "ft" if true, "m" if false.
     */
    void setLapAscent(float ascent, bool isImperial);

    /**
     * @brief Display total ascent.
     * @param ascent     Already-converted value in m or ft (view's responsibility).
     * @param isImperial Selects unit label: "ft" if true, "m" if false.
     */
    void setTotalAscent(float ascent, bool isImperial);

    /**
     * @brief Display elevation.
     * @param elevation  Already-converted value in m or ft (view's responsibility).
     * @param isImperial Selects unit label: "ft" if true, "m" if false.
     */
    void setElevation(float elevation, bool isImperial);

protected:
};

#endif // TRACKFACEELEVATION_HPP
