#include <gui/menuintervalsrestdistance_screen/MenuIntervalsRestDistanceView.hpp>
#include <gui/menuintervalsrestdistance_screen/MenuIntervalsRestDistancePresenter.hpp>

MenuIntervalsRestDistancePresenter::MenuIntervalsRestDistancePresenter(MenuIntervalsRestDistanceView& v)
    : view(v)
{

}

void MenuIntervalsRestDistancePresenter::activate()
{
    const Settings::Intervals& iv = model->getSettings().intervals;
    view.setDistance(iv.restDistance, model->isUnitsImperial());
    model->resetIdleTimer();
}

void MenuIntervalsRestDistancePresenter::deactivate()
{

}

void MenuIntervalsRestDistancePresenter::saveRestDistance(float meters)
{
    Settings sett = model->getSettings();
    if (meters > 0.0f) {
        sett.intervals.restMetric   = Settings::Intervals::DISTANCE;
        sett.intervals.restDistance = meters;
    } else {
        sett.intervals.restMetric   = Settings::Intervals::OPEN;
        sett.intervals.restDistance = 0.0f;
    }
    model->saveSettings(sett);
}
