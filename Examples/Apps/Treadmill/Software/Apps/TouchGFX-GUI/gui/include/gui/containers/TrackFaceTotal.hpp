#ifndef TRACKFACETOTAL_HPP
#define TRACKFACETOTAL_HPP

#include <gui_generated/containers/TrackFaceTotalBase.hpp>

/**
 * @brief Track face showing overall session metrics: pace, total distance and elapsed time.
 *
 * All values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceTotal : public TrackFaceTotalBase
{
public:
    TrackFaceTotal();
    virtual ~TrackFaceTotal() {}

    virtual void initialize();

    /**
     * @brief Display current pace.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < App::Display::kMinPace to show "---" (no GPS / no data).
     */
    void setPace(float pace);

    /**
     * @brief Display total distance with unit label.
     * @param dist       Already-converted value in km or mi (view's responsibility).
     *                   Pass a value < App::Display::kMinDist to show "---" (no GPS / no data).
     * @param isImperial Selects unit label: "mi" if true, "km" if false.
     */
    void setDistance(float dist, bool isImperial);

    /** @brief Display total elapsed time as "H:MM:SS". */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACETOTAL_HPP
