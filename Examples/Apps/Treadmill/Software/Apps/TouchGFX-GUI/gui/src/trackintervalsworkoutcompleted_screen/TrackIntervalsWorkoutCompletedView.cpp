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
    application().gotoTrackSummaryScreenNoTransition();
}
