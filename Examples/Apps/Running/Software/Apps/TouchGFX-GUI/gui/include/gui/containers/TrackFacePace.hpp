#ifndef TRACKFACEPACE_HPP
#define TRACKFACEPACE_HPP

#include <gui_generated/containers/TrackFacePaceBase.hpp>

/**
 * @brief Track face showing this lap's average pace beside the whole run's.
 *
 * Displays total distance, the current lap's average pace ("Avg Lap"), the
 * activity's average pace ("Avg Pace") and the total elapsed time.
 * All distance/pace values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFacePace : public TrackFacePaceBase
{
public:
    TrackFacePace();
    virtual ~TrackFacePace() {}

    virtual void initialize();

    /**
     * @brief Display total distance with unit label.
     * @param dist       Already-converted value in km or mi (view's responsibility).
     *                   Pass a value < App::Display::kMinDist to show "---" (no GPS / no data).
     * @param isImperial Selects unit label: "mi" if true, "km" if false.
     */
    void setDistance(float dist, bool isImperial);

    /**
     * @brief Display the current lap's average pace.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < App::Display::kMinPace to show "---" (no GPS / no data).
     */
    void setAvgLap(float pace);

    /**
     * @brief Display the activity's average pace.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < App::Display::kMinPace to show "---" (no GPS / no data).
     */
    void setAvgPace(float pace);

    /** @brief Display total elapsed time as "H:MM:SS". */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACEPACE_HPP
