#ifndef TRACKFACEOVERVIEW_HPP
#define TRACKFACEOVERVIEW_HPP

#include <gui_generated/containers/TrackFaceOverviewBase.hpp>

/**
 * @brief Track face showing average speed, elevation and heart rate zone.
 *
 * Displays average speed, elevation and an HR zone bar.
 * All distance/speed values are display-ready -- unit conversion is the view's responsibility.
 */
class TrackFaceOverview : public TrackFaceOverviewBase
{
public:
    TrackFaceOverview();
    virtual ~TrackFaceOverview() {}

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
     * @brief Display average speed with unit label.
     * @param speed      Already-converted value in km/h or mi/h (view's responsibility).
     * @param isImperial Selects unit label: "mi/h" if true, "km/h" if false.
     */
    void setAvgSpeed(float speed, bool isImperial);

    /**
     * @brief Display elevation.
     * @param elevation  Already-converted value in m or ft (view's responsibility).
     * @param isImperial Selects unit label: "ft" if true, "m" if false.
     */
    void setElevation(float elevation, bool isImperial);

protected:
};

#endif // TRACKFACEOVERVIEW_HPP
