#include <gui/menu_screen/MenuView.hpp>
#include <gui/menu_screen/MenuPresenter.hpp>

MenuPresenter::MenuPresenter(MenuView& v)
    : view(v)
{

}

void MenuPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();

    size_t id = model->getAlarmEditId();

    if (id < model->getAlarmList().size()) {
        view.setAlarmState(id, model->getAlarmList()[id].on);
    }
}

void MenuPresenter::deactivate()
{

}

void MenuPresenter::onAlarmListUpdated(const std::vector<Alarm>& list)
{
    size_t id = model->getAlarmEditId();

    if (id < model->getAlarmList().size()) {
        view.setAlarmState(id, model->getAlarmList()[id].on);
    }
}

void MenuPresenter::save(size_t id, bool state)
{
    if (id < model->getAlarmList().size()) {
        Alarm alarm = model->getAlarmList()[id];
        alarm.on = state;
        model->saveAlarm(id, alarm);
    }
}
