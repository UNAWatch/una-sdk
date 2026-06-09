#ifndef TRACKCALIBRATEVIEW_HPP
#define TRACKCALIBRATEVIEW_HPP

#include <gui_generated/trackcalibrate_screen/TrackCalibrateViewBase.hpp>
#include <gui/trackcalibrate_screen/TrackCalibratePresenter.hpp>
#include <gui/containers/PickerLogic.hpp>

/**
 * Post-run "Calibrate & Save" distance entry (§5). Uses the shared TwoTonePicker:
 * stage 1 edits the whole part, stage 2 the 0.01-unit fraction. R1 confirms /
 * advances; R2 steps back a stage, or (from stage 1) backs out to the pause menu
 * without saving (the activity stays paused until R1 confirms).
 */
class TrackCalibrateView : public TrackCalibrateViewBase
{
public:
    TrackCalibrateView() {}
    virtual ~TrackCalibrateView() {}
    virtual void setupScreen();

    /// Seed the picker from a distance in metres, in the user's display units.
    void setDistance(float meters, bool isImperial);

protected:
    using Menu = App::MenuNav::Root::Intervals::CalibratePicker;

    PickerLogic::Distance<Menu> mLogic;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKCALIBRATEVIEW_HPP
