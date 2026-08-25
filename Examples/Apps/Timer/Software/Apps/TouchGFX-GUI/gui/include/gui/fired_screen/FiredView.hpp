#ifndef FIREDVIEW_HPP
#define FIREDVIEW_HPP

#include <gui_generated/fired_screen/FiredViewBase.hpp>
#include <gui/fired_screen/FiredPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * @brief Fired screen: the countdown reached zero and the alert is sounding.
 *
 * An amber "Timer" label with two static actions: Done (R1) silences and ends
 * the timer (-> Main, or exits when the alert opened the GUI); Repeat (R2)
 * silences and re-arms the countdown (-> Running).
 *
 * Two countdowns run while the screen is up (the Alarm pattern):
 *   - mPlayTimer    re-plays the alert effect every kPlayTicks, so the
 *                   indication keeps going until the user acknowledges it;
 *   - mTimeoutTimer auto-acknowledges (as Done) after kTimeoutTicks, so an
 *                   unattended alert stops itself after a minute.
 */
class FiredView : public FiredViewBase
{
public:
    FiredView();
    virtual ~FiredView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;

    /** @brief Fired by mPlayTimer to repeat the alert. */
    void onPlay();
    /** @brief Fired by mTimeoutTimer to auto-acknowledge the alert as Done. */
    void onTimeout();

    /** @brief Silence + end the timer and leave the screen (shared by R1 + timeout). */
    void finishDone();

    SDK::GUI::CountdownTimer      mPlayTimer;    ///< Periodic alert replay.
    touchgfx::Callback<FiredView> mPlayCb;

    SDK::GUI::CountdownTimer      mTimeoutTimer; ///< Auto-Done after the max ring time.
    touchgfx::Callback<FiredView> mTimeoutCb;
};

#endif // FIREDVIEW_HPP
