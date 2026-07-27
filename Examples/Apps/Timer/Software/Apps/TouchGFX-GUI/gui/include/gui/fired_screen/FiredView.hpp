#ifndef FIREDVIEW_HPP
#define FIREDVIEW_HPP

#include <gui_generated/fired_screen/FiredViewBase.hpp>
#include <gui/fired_screen/FiredPresenter.hpp>

/**
 * @brief Fired screen: the countdown reached zero and the alert is sounding.
 *
 * An amber "Timer" label with two static actions: Repeat (R1) silences the
 * alert and restarts the countdown (-> Running); Stop (R2) silences it and
 * returns to Main.
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
};

#endif // FIREDVIEW_HPP
