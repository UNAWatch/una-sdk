#ifndef TRACKFACEINTERVALS_HPP
#define TRACKFACEINTERVALS_HPP

#include <gui_generated/containers/TrackFaceIntervalsBase.hpp>
#include <ctime>
#include "touchgfx/Color.hpp"

class TrackFaceIntervals : public TrackFaceIntervalsBase
{
public:
    TrackFaceIntervals();
    virtual ~TrackFaceIntervals() {}
    virtual void initialize();

    // -------------------------------------------------------------------------
    // Phase appearance
    // Call once when the phase changes.
    // -------------------------------------------------------------------------

    /**
     * @brief Apply title, accent color and icon visibility for the given phase.
     *
     * @param phase        Current phase.
     * @param repeat       1-based repeat index (shown as "n/total" during RUN/REST).
     * @param totalRepeats Total number of RUN-REST cycles.
     */
    void setPhase(Track::IntervalsPhase phase, uint8_t repeat, uint8_t totalRepeats);

    // -------------------------------------------------------------------------
    // Timer area  (call each tick from the presenter)
    // -------------------------------------------------------------------------

    /**
     * @brief Update the timer for a time-based metric (TIME_OPEN / TIME_REMAINING / TIME_ELAPSED).
     * @param sec    Seconds value, clamped to 59:59.
     * @param metric Determines the description label.
     */
    void setPhaseTime(std::time_t sec, Track::IntervalsMetric metric);

    /**
     * @brief Update the timer for distance metric -- always shows "km/mi remaining".
     * @param dist       Already-converted value in km or mi (view's responsibility).
     * @param isImperial Selects unit label.
     */
    void setPhaseDistance(float dist, bool isImperial);

    // -------------------------------------------------------------------------
    // Bottom row  (call each tick from the presenter)
    // -------------------------------------------------------------------------

    /**
     * @brief Update pace -- visible only during RUN phase.
     * @param pace Already-converted value in sec/km or sec/mi (view's responsibility).
     *             Pass a value < App::Display::kMinPace to show "---" (no GPS / no data).
     */
    void setPace(float pace);

    /**
     * @brief Update heart rate -- visible during REST phase.
     * @param hr Heart rate in bpm. Pass a value < App::Display::kMinHR to show "---".
     */
    void setHR(float hr);

    /**
     * @brief Override the default accent colours for each phase.
     *
     * Call before the first setPhase() if non-default colours are needed.
     */
    void setPhaseColors(touchgfx::colortype neutral,
                        touchgfx::colortype run,
                        touchgfx::colortype rest);

private:
    touchgfx::colortype mColorNeutral = 0xC0C0C0;  ///< WARM_UP / COOL_DOWN
    touchgfx::colortype mColorRun     = 0x40C0C0;  ///< RUN
    touchgfx::colortype mColorRest    = 0xC08000;  ///< REST

};


#endif // TRACKFACEINTERVALS_HPP
