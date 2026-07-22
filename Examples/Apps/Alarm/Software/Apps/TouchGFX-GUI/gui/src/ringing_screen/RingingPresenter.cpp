#include <gui/ringing_screen/RingingView.hpp>
#include <gui/ringing_screen/RingingPresenter.hpp>

RingingPresenter::RingingPresenter(RingingView& v)
    : view(v)
{

}

void RingingPresenter::activate()
{
    play();
}

void RingingPresenter::deactivate()
{
    // If the alarm is not turned off by the user and the screen has
    // switched to a higher priority, then stop it
    if (model->getActiveAlarm().on) {
        model->stopAlarm();
    }
}

void RingingPresenter::play()
{
    model->playAlarm();
}

void RingingPresenter::stop()
{
    model->stopAlarm();
    model->switchToNextPriorityScreen();
}

void RingingPresenter::snooze()
{
    model->snoozeAlarm();
    model->switchToNextPriorityScreen();
}
