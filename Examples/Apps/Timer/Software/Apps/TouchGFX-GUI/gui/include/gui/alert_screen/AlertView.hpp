#ifndef ALERTVIEW_HPP
#define ALERTVIEW_HPP

#include <gui_generated/alert_screen/AlertViewBase.hpp>
#include <gui/alert_screen/AlertPresenter.hpp>

/**
 * @brief Alert screen: choose the countdown's alert effect.
 *
 * A uniform OrbitMenu lists Beep / Vibrate / Beep & Vibrate (all white, smooth
 * scroll); a static teal pill and the "Alert" label sit on the screen behind
 * it, so whichever option is centred reads as selected. L1/L2 scroll, R1
 * confirms (store the effect, go to Menu), R2 returns to Edit.
 */
class AlertView : public AlertViewBase
{
public:
    AlertView();
    virtual ~AlertView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /** @brief Centre the wheel on the given effect. */
    void set(Timer::Effect effect);

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    void confirm();

    static const int16_t kCount = 3;
};

#endif // ALERTVIEW_HPP
