#include <gui/trackintervalsworkoutcompleted_screen/TrackIntervalsWorkoutCompletedView.hpp>
#include <SDK/GUI/Config.hpp>
#include <SDK/Utils/Utils.hpp>

static constexpr uint16_t kAutoAdvanceTicks = SDK::Utils::secToTicks(5, App::Config::kFrameRate);

TrackIntervalsWorkoutCompletedView::TrackIntervalsWorkoutCompletedView()
    : mAutoAdvanceTimerCb(this, &TrackIntervalsWorkoutCompletedView::onAutoAdvanceTimerFired)
{
}

void TrackIntervalsWorkoutCompletedView::setupScreen()
{
    TrackIntervalsWorkoutCompletedViewBase::setupScreen();

    mAutoAdvanceTimer.setDuration(kAutoAdvanceTicks);
    mAutoAdvanceTimer.setCallback(mAutoAdvanceTimerCb);
    mAutoAdvanceTimer.start();
}

void TrackIntervalsWorkoutCompletedView::tearDownScreen()
{
    mAutoAdvanceTimer.stop();
    TrackIntervalsWorkoutCompletedViewBase::tearDownScreen();
}

void TrackIntervalsWorkoutCompletedView::onAutoAdvanceTimerFired()
{
    // Return to live tracking after the notification: the session continues so
    // the user can record more laps and end it manually (it is no longer in
    // intervals mode, so the track screen shows the normal faces).
    application().gotoTrackScreenNoTransition();
}
