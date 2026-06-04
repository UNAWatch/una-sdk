#ifndef TRACKCALIBRATEINTROVIEW_HPP
#define TRACKCALIBRATEINTROVIEW_HPP

#include <gui_generated/trackcalibrateintro_screen/TrackCalibrateIntroViewBase.hpp>
#include <gui/trackcalibrateintro_screen/TrackCalibrateIntroPresenter.hpp>

/**
 * Static informational screen shown after a treadmill run (when Calibrate & Save
 * is offered) and just before the distance picker. It explains what the next
 * screen is for and is dismissed with the OK button (R1), which advances to the
 * distance entry.
 */
class TrackCalibrateIntroView : public TrackCalibrateIntroViewBase
{
public:
    TrackCalibrateIntroView() {}
    virtual ~TrackCalibrateIntroView() {}
    virtual void setupScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKCALIBRATEINTROVIEW_HPP
