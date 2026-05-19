#ifndef RINGINGVIEW_HPP
#define RINGINGVIEW_HPP

#include <gui_generated/ringing_screen/RingingViewBase.hpp>
#include <gui/ringing_screen/RingingPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

/**
 * @brief Screen displayed while an alarm is ringing.
 *
 * Two timers run in parallel:
 *   - mSnoozeTimer : auto-snoozes the alarm when it expires (after kSnoozeTicks).
 *   - mPlayTimer   : repeats the alarm sound every kPlayTicks.
 *
 * R1 stops the alarm; R2 snoozes it immediately.
 */
class RingingView : public RingingViewBase
{
public:
    RingingView();
    virtual ~RingingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setTime(uint8_t h, uint8_t );

protected:
    virtual void handleKeyEvent(uint8_t key) override;

    /** @brief Fired by mSnoozeTimer when the auto-snooze timeout elapses. */
    void onSnooze();
    /** @brief Fired by mPlayTimer to repeat the alarm sound. */
    void onPlay();

    CountdownTimer                   mSnoozeTimer;  ///< Auto-snooze countdown
    touchgfx::Callback<RingingView>  mSnoozeCb;

    CountdownTimer                   mPlayTimer;    ///< Periodic playback trigger
    touchgfx::Callback<RingingView>  mPlayCb;
};

#endif // RINGINGVIEW_HPP
