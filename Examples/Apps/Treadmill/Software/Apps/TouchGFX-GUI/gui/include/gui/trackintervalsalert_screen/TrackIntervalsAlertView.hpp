#ifndef TRACKINTERVALSALERTVIEW_HPP
#define TRACKINTERVALSALERTVIEW_HPP

#include <gui_generated/trackintervalsalert_screen/TrackIntervalsAlertViewBase.hpp>
#include <gui/trackintervalsalert_screen/TrackIntervalsAlertPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>
#include <ctime>

class TrackIntervalsAlertView : public TrackIntervalsAlertViewBase
{
public:
    TrackIntervalsAlertView();
    virtual ~TrackIntervalsAlertView() {}

    virtual void setupScreen();
    virtual void tearDownScreen();

    // -------------------------------------------------------------------------
    // Called by the presenter in activate()
    // -------------------------------------------------------------------------

    /**
     * @brief Apply title, accent color and repeat counter for the given phase.
     *
     * @param phase        Current interval phase.
     * @param repeat       1-based repeat index (shown as "n/total" during RUN/REST).
     * @param totalRepeats Total number of RUN-REST cycles.
     */
    void setPhase(Track::IntervalsPhase phase, uint8_t repeat, uint8_t totalRepeats);

    /**
     * @brief Update the timer -- always shows remaining time, no description, no line.
     * @param sec Seconds remaining in the current phase, clamped to 59:59.
     */
    void setPhaseTime(std::time_t sec);

    /**
     * @brief Update the timer for a distance-based phase -- shows distance remaining.
     * @param distDisplay Already-converted display value in km or mi.
     * @param isImperial  Selects unit label.
     */
    void setPhaseDistance(float distDisplay, bool isImperial);

private:
    void onDismissTimerFired();

    CountdownTimer                              mDismissTimer;
    touchgfx::Callback<TrackIntervalsAlertView> mDismissTimerCb;
};

#endif // TRACKINTERVALSALERTVIEW_HPP
