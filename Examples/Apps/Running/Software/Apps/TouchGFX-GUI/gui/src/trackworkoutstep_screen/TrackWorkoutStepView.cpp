#include <gui/trackworkoutstep_screen/TrackWorkoutStepView.hpp>

// Shown for 3 seconds.
static constexpr uint16_t kDismissTicksDuration = SDK::Utils::secToTicks(3, App::Config::kFrameRate);

TrackWorkoutStepView::TrackWorkoutStepView()
    : mDismissTimerCb(this, &TrackWorkoutStepView::onDismissTimerFired)
{
}

void TrackWorkoutStepView::setupScreen()
{
    TrackWorkoutStepViewBase::setupScreen();

    // The card has its own title.
    title.setVisible(false);
    add(mCard);

    mDismissTimer.setDuration(kDismissTicksDuration);
    mDismissTimer.setCallback(mDismissTimerCb);
    mDismissTimer.start();
}

void TrackWorkoutStepView::tearDownScreen()
{
    mDismissTimer.stop();
    TrackWorkoutStepViewBase::tearDownScreen();
}

void TrackWorkoutStepView::setStep(const Model::WorkoutStepInfo& step)
{
    mCard.showCurrent(step);
    mDismissTimer.stop();
    mDismissTimer.start();
}

void TrackWorkoutStepView::onDismissTimerFired()
{
    application().gotoTrackScreenNoTransition();
}
