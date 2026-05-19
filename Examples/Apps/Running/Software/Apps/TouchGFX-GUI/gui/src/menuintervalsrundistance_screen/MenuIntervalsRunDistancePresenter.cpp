#include <gui/menuintervalsrundistance_screen/MenuIntervalsRunDistanceView.hpp>
#include <gui/menuintervalsrundistance_screen/MenuIntervalsRunDistancePresenter.hpp>

MenuIntervalsRunDistancePresenter::MenuIntervalsRunDistancePresenter(MenuIntervalsRunDistanceView& v)
    : view(v)
{

}

void MenuIntervalsRunDistancePresenter::activate()
{
    const Settings::Intervals& iv = model->getSettings().intervals;
    view.setDistance(iv.runDistance, model->isUnitsImperial());
    model->resetIdleTimer();
}

void MenuIntervalsRunDistancePresenter::deactivate()
{

}

void MenuIntervalsRunDistancePresenter::saveRunDistance(float meters)
{
    Settings sett = model->getSettings();
    if (meters > 0.0f) {
        sett.intervals.runMetric   = Settings::Intervals::DISTANCE;
        sett.intervals.runDistance = meters;
    } else {
        sett.intervals.runMetric   = Settings::Intervals::OPEN;
        sett.intervals.runDistance = 0.0f;
    }
    model->saveSettings(sett);
}
