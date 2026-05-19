#include <gui/edit_screen/EditView.hpp>
#include <gui/edit_screen/EditPresenter.hpp>

EditPresenter::EditPresenter(EditView& v)
    : view(v)
{

}

void EditPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();
    size_t id = model->getAlarmEditId();
    if (id < model->getAlarmList().size()) {
        const Alarm& alarm = model->getAlarmList()[id];
        view.set(alarm.timeHours, alarm.timeMinutes, alarm.repeat, alarm.effect);
    } else {
        // Start with default values for new alarm
        view.set(0, 0, Alarm::Repeat::REPEAT_NO, Alarm::Effect::EFFECT_BEEP_AND_VIBRO);
    }
}

void EditPresenter::deactivate()
{

}

void EditPresenter::exitScreen()
{
    model->switchToNextPriorityScreen();
}

void EditPresenter::save(uint8_t h, uint8_t m, Alarm::Repeat repeat, Alarm::Effect effect)
{
    Alarm alarm = {};

    alarm.timeHours = h;
    alarm.timeMinutes = m;
    alarm.repeat = repeat;
    alarm.effect = effect;
    alarm.on = true;
    model->saveAlarm(model->getAlarmEditId(), alarm);

    model->application().gotoSavedScreenNoTransition();
}