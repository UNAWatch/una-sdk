#include <gui/menuintervals_screen/MenuIntervalsView.hpp>
#include <gui/menuintervals_screen/MenuIntervalsPresenter.hpp>

MenuIntervalsPresenter::MenuIntervalsPresenter(MenuIntervalsView& v)
    : view(v)
{

}

void MenuIntervalsPresenter::activate()
{
    view.setPositionId(model->menu().intervals.get());
    model->menu().intervals.resetChildren();
    model->resetIdleTimer();

    view.setIntervals(model->getSettings().intervals, model->isUnitsImperial());
}

void MenuIntervalsPresenter::deactivate()
{
    model->menu().intervals.set(view.getPositionId());
}

void MenuIntervalsPresenter::onIdleTimeout()
{
    if (view.getPositionId() != App::MenuNav::Root::Intervals::ID_START) {
        model->exitApp();
    }
}

void MenuIntervalsPresenter::saveWarmUp(bool enable)
{
    Settings sett = model->getSettings();
    sett.intervals.warmUp = enable;
    model->saveSettings(sett);
}

void MenuIntervalsPresenter::saveCoolDown(bool enable)
{
    Settings sett = model->getSettings();
    sett.intervals.coolDown = enable;
    model->saveSettings(sett);
}