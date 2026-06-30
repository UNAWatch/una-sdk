#ifndef TRACKHOLDCONFIRMATIONVIEW_HPP
#define TRACKHOLDCONFIRMATIONVIEW_HPP

#include <gui_generated/trackholdconfirmation_screen/TrackHoldConfirmationViewBase.hpp>
#include <gui/trackholdconfirmation_screen/TrackHoldConfirmationPresenter.hpp>

/**
 * @brief Shared hold-to-confirm screen for ending an activity.
 *
 * The hold begins on the action menu: pressing-and-holding R1 on Save/Discard
 * navigates here and the countdown starts immediately, running while R1 stays
 * held. Releasing R1 before it completes returns to the action menu; holding the
 * full duration navigates to the result screen (which performs the save/discard).
 * Finish = green, Discard = red. The countdown fills in kHoldMs while the centre
 * number counts 3 -> 2 -> 1.
 */
class TrackHoldConfirmationView : public TrackHoldConfirmationViewBase
{
public:
    TrackHoldConfirmationView();
    virtual ~TrackHoldConfirmationView() {}
    virtual void setupScreen() override;
    virtual void tearDownScreen() override;

protected:
    virtual void handleKeyEvent(uint8_t key) override;

    void onTimerValueChanged(int32_t value);
    void onTimerComplete(int32_t value);
    void updateCountdownText(uint32_t number);

    static const uint32_t kHoldMs = 1500;  ///< Hold duration (1.5 s); counts 3 -> 2 -> 1.

    touchgfx::Callback<TrackHoldConfirmationView, int32_t> mTimerValueChangedCb;
    touchgfx::Callback<TrackHoldConfirmationView, int32_t> mTimerCompleteCb;

    Model::HoldConfirmMode mMode  = Model::HoldConfirmMode::Discard;
    bool                   mFired = false;
    uint32_t               mLastDisplayedNumber = 3;
};

#endif // TRACKHOLDCONFIRMATIONVIEW_HPP
