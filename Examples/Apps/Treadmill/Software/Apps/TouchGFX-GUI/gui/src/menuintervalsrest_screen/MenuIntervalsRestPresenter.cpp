#include <gui/menuintervalsrest_screen/MenuIntervalsRestView.hpp>
#include <gui/menuintervalsrest_screen/MenuIntervalsRestPresenter.hpp>

using Menu = App::MenuNav::Root::Intervals::Metric;

MenuIntervalsRestPresenter::MenuIntervalsRestPresenter(MenuIntervalsRestView& v)
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

void MenuIntervalsRestPresenter::activate()
{
    const Settings::Intervals& iv = model->getSettings().intervals;
    view.setRestValue(iv, model->isUnitsImperial());
    view.setPositionId(metricToMenuId(iv.restMetric));
    model->resetIdleTimer();
}

void MenuIntervalsRestPresenter::deactivate()
{
    model->menu().intervals.rest.set(view.getPositionId());
}

void MenuIntervalsRestPresenter::saveOpenRest()
{
    Settings sett = model->getSettings();
    sett.intervals.restMetric = Settings::Intervals::OPEN;
    model->saveSettings(sett);
}
