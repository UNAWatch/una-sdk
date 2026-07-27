#ifndef EDITVIEW_HPP
#define EDITVIEW_HPP

#include <gui_generated/edit_screen/EditViewBase.hpp>
#include <gui/edit_screen/EditPresenter.hpp>
#include <gui/containers/SpherePicker.hpp>

/**
 * @brief Edit (Set Timer) screen: two sphere-picker columns for Mins : Secs.
 *
 * The active column is teal and scrolls (L1/L2); the other shows its value.
 * R1 advances Mins -> Secs -> confirm (store the duration, go to Alert); R2
 * steps back, or leaves to Main from the minutes step.
 */
class EditView : public EditViewBase
{
public:
    EditView();
    virtual ~EditView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Seed the pickers from a duration in seconds. */
    void set(uint16_t durationSec);

protected:
    virtual void handleKeyEvent(uint8_t key) override;
    virtual void handleTickEvent() override;

private:
    enum Step { STEP_MINS, STEP_SECS };

    void updateActive();
    void syncConfirmButton();   ///< Hide R1 when the duration is 00:00.
    void confirm();

    /**
     * @brief Step the active column one place.
     * @param snap Collapse the sphere animation to the new value at once. Used
     *        by the fast auto-repeat so the shown number tracks the committed
     *        one exactly (no trailing, so releasing never overshoots); a single
     *        tap leaves it false to keep the smooth roll.
     */
    void scrollActive(bool forward, bool snap = false);

    Step         mStep = STEP_MINS;
    SpherePicker mMins;
    SpherePicker mSecs;

    // Hold-to-repeat: while L1/L2 is held past a threshold the active column
    // auto-scrolls, accelerating from a slow rate to the fastest step. A short
    // tap moves once via the click event and never crosses the threshold.
    uint8_t  mHeldDir       = 0;   ///< Held button (L1/L2), or 0 when released.
    uint32_t mHoldTicks     = 0;   ///< Ticks since the button went down.
    int16_t  mRepeatCountdown = 0; ///< Ticks left until the next auto-scroll.
    int16_t  mRepeatInterval  = 0; ///< Ticks between auto-scrolls (shrinks).
};

#endif // EDITVIEW_HPP
