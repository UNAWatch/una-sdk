#ifndef FIREDVIEW_HPP
#define FIREDVIEW_HPP

#include <gui_generated/fired_screen/FiredViewBase.hpp>
#include <gui/fired_screen/FiredPresenter.hpp>

/**
 * @brief Fired screen: the countdown reached zero and the alert is sounding.
 *
 * An amber "Timer" label with two static actions: Done (R1) silences and ends
 * the timer (-> Main, or exits when the alert opened the GUI); Repeat (R2)
 * silences and re-arms the countdown (-> Running).
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
