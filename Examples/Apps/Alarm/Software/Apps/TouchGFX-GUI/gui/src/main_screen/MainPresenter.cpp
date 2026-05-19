#include <gui/main_screen/MainView.hpp>
#include <gui/main_screen/MainPresenter.hpp>

MainPresenter::MainPresenter(MainView& v)
    : view(v)
{

}

void MainPresenter::activate()
{
    // Reset idle timer
    model->resetIdleTimer();

    view.updateAlarmList(model->getAlarmList());
    view.setSelectedAlarm(model->getAlarmEditId());
}

void MainPresenter::deactivate()
{

}

void MainPresenter::onAlarmListUpdated(const std::vector<Alarm>& list) 
{
    view.updateAlarmList(list);
}

void MainPresenter::setAlarmEditId(size_t id)
{
    model->setAlarmEditId(id);
}

void MainPresenter::exitApp()
{
    model->exitApp();
}