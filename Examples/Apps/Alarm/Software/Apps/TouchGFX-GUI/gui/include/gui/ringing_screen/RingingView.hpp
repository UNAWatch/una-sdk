#ifndef RINGINGVIEW_HPP
#define RINGINGVIEW_HPP

#include <gui_generated/ringing_screen/RingingViewBase.hpp>
#include <gui/ringing_screen/RingingPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * @brief Screen displayed while an alarm is ringing.
 *
 * Two timers run in parallel:
 *   - mSnoozeTimer : auto-snoozes the alarm when it expires (after kSnoozeTicks).
 *   - mPlayTimer   : repeats the alarm sound every kPlayTicks.
 *
 * R1 (top-right) snoozes the alarm; R2 (bottom-right) stops it.
 */
class RingingView : public RingingViewBase
{
public:
    RingingView();
    virtual ~RingingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;

    // Large "Alarm" title shown in place of the time/icon (see setupScreen).
    touchgfx::TextAreaWithOneWildcard mAlarmTitle;
    static const uint16_t ALARM_TITLE_SIZE = 8;
    touchgfx::Unicode::UnicodeChar mAlarmTitleBuffer[ALARM_TITLE_SIZE];

    /** @brief Fired by mSnoozeTimer when the auto-snooze timeout elapses. */
    void onSnooze();
    /** @brief Fired by mPlayTimer to repeat the alarm sound. */
    void onPlay();

    SDK::GUI::CountdownTimer         mSnoozeTimer;  ///< Auto-snooze countdown
    touchgfx::Callback<RingingView>  mSnoozeCb;

    SDK::GUI::CountdownTimer         mPlayTimer;    ///< Periodic playback trigger
    touchgfx::Callback<RingingView>  mPlayCb;
};

#endif // RINGINGVIEW_HPP
