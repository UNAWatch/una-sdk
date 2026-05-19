#ifndef TRACKFACETOTAL_HPP
#define TRACKFACETOTAL_HPP

#include <gui_generated/containers/TrackFaceTotalBase.hpp>

/**
 * @brief Track face showing overall session metrics: speed, total distance and elapsed time.
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
     * @brief Display current speed with unit label.
     * @param speed      Already-converted value in km/h or mi/h (view's responsibility).
     * @param isImperial Selects unit label: "mi/h" if true, "km/h" if false.
     */
    void setSpeed(float speed, bool isImperial);

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
