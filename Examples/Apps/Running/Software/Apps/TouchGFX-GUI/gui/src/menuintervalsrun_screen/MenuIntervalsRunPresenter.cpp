#include <gui/menuintervalsrun_screen/MenuIntervalsRunView.hpp>
#include <gui/menuintervalsrun_screen/MenuIntervalsRunPresenter.hpp>

using Menu = App::MenuNav::Root::Intervals::Metric;

MenuIntervalsRunPresenter::MenuIntervalsRunPresenter(MenuIntervalsRunView& v)
    : view(v)
{

}

static uint16_t metricToMenuId(Settings::Intervals::Metric metric)
{
    switch (metric) {
    case Settings::Intervals::TIME:     return Menu::ID_TIME;
    case Settings::Intervals::DISTANCE: return Menu::ID_DISTANCE;
    case Settings::Intervals::OPEN:
    default:                            return Menu::ID_OPEN;
    }
}

void MenuIntervalsRunPresenter::activate()
{
    const Settings::Intervals& iv = model->getSettings().intervals;
    view.setRunValue(iv, model->isUnitsImperial());
    view.setPositionId(metricToMenuId(iv.runMetric));
    model->resetIdleTimer();
}

void MenuIntervalsRunPresenter::deactivate()
{
    model->menu().intervals.run.set(view.getPositionId());
}

void MenuIntervalsRunPresenter::saveOpenRun()
{
    Settings sett = model->getSettings();
    sett.intervals.runMetric = Settings::Intervals::OPEN;
    model->saveSettings(sett);
}
