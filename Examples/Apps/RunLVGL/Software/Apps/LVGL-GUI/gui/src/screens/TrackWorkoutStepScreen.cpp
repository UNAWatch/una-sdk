/**
 ******************************************************************************
 * @file    TrackWorkoutStepScreen.cpp
 * @brief   The step card (see the header).
 ******************************************************************************
 */

#include "gui/screens/TrackWorkoutStepScreen.hpp"
#include "gui/screens/ScreenManager.hpp"

TrackWorkoutStepScreen::TrackWorkoutStepScreen(Model& model)
    : Screen(model)
{
}

TrackWorkoutStepScreen::~TrackWorkoutStepScreen()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void TrackWorkoutStepScreen::build()
{
    mCard = std::make_unique<WorkoutStepCard>(mRoot);
}

void TrackWorkoutStepScreen::onShow()
{
    mTimer = lv_timer_create(&TrackWorkoutStepScreen::dismissCb, kShowMs, this);
    lv_timer_set_repeat_count(mTimer, 1);
    lv_timer_set_auto_delete(mTimer, false);
    showStep();
}

void TrackWorkoutStepScreen::showStep()
{
    mModel.takeStepCard();
    mCard->showCurrent(mModel.getWorkoutStep(false));
    // A new step shows for the full 3 s.
    lv_timer_reset(mTimer);
    lv_timer_set_repeat_count(mTimer, 1);
    lv_timer_resume(mTimer);
}

void TrackWorkoutStepScreen::onWorkoutStep(bool next)
{
    if (!next) {
        showStep();
    }
}

void TrackWorkoutStepScreen::onIntervalsWorkoutCompleted()
{
    ScreenManager::instance().goTo(ScreenId::TrackIntervalsCompleted);
}

void TrackWorkoutStepScreen::dismissCb(lv_timer_t* timer)
{
    (void)timer;
    ScreenManager::instance().goTo(ScreenId::Track);
}
