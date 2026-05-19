#include <gui/menuintervalsruntime_screen/MenuIntervalsRunTimeView.hpp>
#include <gui/menuintervalsruntime_screen/MenuIntervalsRunTimePresenter.hpp>

MenuIntervalsRunTimePresenter::MenuIntervalsRunTimePresenter(MenuIntervalsRunTimeView& v)
    : view(v)
{

}

void MenuIntervalsRunTimePresenter::activate()
{
    view.setTime(model->getSettings().intervals.runTime);
    model->resetIdleTimer();
}

void MenuIntervalsRunTimePresenter::deactivate()
{

}

void MenuIntervalsRunTimePresenter::saveRunTime(uint32_t totalSeconds)
{
    Settings sett = model->getSettings();
    if (totalSeconds > 0) {
        sett.intervals.runMetric = Settings::Intervals::TIME;
        sett.intervals.runTime   = totalSeconds;
    } else {
        sett.intervals.runMetric = Settings::Intervals::OPEN;
        sett.intervals.runTime   = 0;
    }
    model->saveSettings(sett);
}
