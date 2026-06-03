#ifndef TRACKFACELAP_HPP
#define TRACKFACELAP_HPP

#include <gui_generated/containers/TrackFaceLapBase.hpp>

/**
 * @brief Track face showing current lap metrics and heart rate zone.
 *
 * Displays lap pace, lap distance, lap elapsed time and an HR zone bar.
 * All distance/pace values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceLap : public TrackFaceLapBase
{
public:
    TrackFaceLap();
    virtual ~TrackFaceLap() {}

    virtual void initialize();

    /**
     * @brief Display heart rate value and update the HR zone bar.
     * @param hr             Current heart rate in bpm.
     *                       Pass a value < App::Display::kMinHR to show "---" (no sensor data).
     * @param thresholds     Pointer to the HR zone threshold array (bpm boundaries).
     * @param thresholdCount Number of thresholds in the array.
     */
    void setHR(float hr, const uint8_t* thresholds, uint8_t thresholdCount);

    /**
     * @brief Display current lap pace.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < App::Display::kMinPace to show "---" (no GPS / no data).
     */
    void setPace(float pace);

    /**
     * @brief Display current lap distance.
     * @param dist Already-converted value in km or mi (view's responsibility).
     *             Pass a value < App::Display::kMinDist to show "---" (no GPS / no data).
     */
    void setDistance(float dist);

    /** @brief Display current lap elapsed time as "M:SS" (or "H:MM" when >= 1 h). */
    void setTimer(std::time_t sec);

protected:
};

#endif // TRACKFACELAP_HPP
