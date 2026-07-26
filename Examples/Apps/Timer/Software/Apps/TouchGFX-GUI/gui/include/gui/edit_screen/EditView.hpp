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

private:
    enum Step { STEP_MINS, STEP_SECS };

    void updateActive();
    void confirm();

    Step         mStep = STEP_MINS;
    SpherePicker mMins;
    SpherePicker mSecs;
};

#endif // EDITVIEW_HPP
