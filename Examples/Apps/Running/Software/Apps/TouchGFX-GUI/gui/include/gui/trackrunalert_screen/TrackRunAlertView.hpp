#ifndef TRACKRUNALERTVIEW_HPP
#define TRACKRUNALERTVIEW_HPP

#include <gui_generated/trackrunalert_screen/TrackRunAlertViewBase.hpp>
#include <gui/trackrunalert_screen/TrackRunAlertPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * Run alert pop-up: "Too fast", "Too slow", "HR high" or "HR low", with the
 * pace or heart rate that set it off, shown for three seconds, then back to
 * the track faces.
 */
class TrackRunAlertView : public TrackRunAlertViewBase
{
public:
    TrackRunAlertView();
    virtual ~TrackRunAlertView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setAlert(Track::RunAlertKind kind, float value, bool imperial);

private:
    void onDismiss();

    SDK::GUI::CountdownTimer              mDismissTimer;
    touchgfx::Callback<TrackRunAlertView> mDismissCb;
};

#endif // TRACKRUNALERTVIEW_HPP
