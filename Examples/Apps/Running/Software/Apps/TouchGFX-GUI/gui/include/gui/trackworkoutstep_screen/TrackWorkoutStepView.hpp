#ifndef TRACKWORKOUTSTEPVIEW_HPP
#define TRACKWORKOUTSTEPVIEW_HPP

#include <gui_generated/trackworkoutstep_screen/TrackWorkoutStepViewBase.hpp>
#include <gui/trackworkoutstep_screen/TrackWorkoutStepPresenter.hpp>
#include <gui/containers/WorkoutStepCard.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

/**
 * The next-step card: shown for a few seconds when a workout step starts,
 * then back to the run. Keys are ignored while it shows, as on the Intervals
 * phase alert.
 */
class TrackWorkoutStepView : public TrackWorkoutStepViewBase
{
public:
    TrackWorkoutStepView();
    virtual ~TrackWorkoutStepView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// Show @p step, and start the few seconds again.
    void setStep(const Model::WorkoutStepInfo& step);

protected:
    void onDismissTimerFired();

    WorkoutStepCard                          mCard;
    SDK::GUI::CountdownTimer                 mDismissTimer;
    touchgfx::Callback<TrackWorkoutStepView> mDismissTimerCb;
};

#endif // TRACKWORKOUTSTEPVIEW_HPP
