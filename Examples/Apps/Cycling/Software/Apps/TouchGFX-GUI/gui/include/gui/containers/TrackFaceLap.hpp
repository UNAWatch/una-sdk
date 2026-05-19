#ifndef TRACKFACELAP_HPP
#define TRACKFACELAP_HPP

#include <gui_generated/containers/TrackFaceLapBase.hpp>

/**
 * @brief Track face showing current lap metrics.
 *
 * Displays lap speed, lap distance, lap elapsed time.
 * All distance/speed values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceLap : public TrackFaceLapBase
{
public:
    TrackFaceLap();
    virtual ~TrackFaceLap() {}

    virtual void initialize();

    /**
     * @brief Display average lap speed with unit label.
     * @param speed      Already-converted value in km/h or mi/h (view's responsibility).
     * @param isImperial Selects unit label: "mi/h" if true, "km/h" if false.
     */
    void setSpeed(float speed, bool isImperial);

    /**
     * @brief Display total distance with unit label.
     * @param dist       Already-converted value in km or mi (view's responsibility).
     * @param isImperial Selects unit label: "mi" if true, "km" if false.
     */
    void setDistance(float dist, bool isImperial);

    /** @brief Display total elapsed active time as "H:MM:SS". */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACELAP_HPP
