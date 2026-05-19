#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimeView.hpp>
#include <gui/menuintervalsresttime_screen/MenuIntervalsRestTimePresenter.hpp>

MenuIntervalsRestTimePresenter::MenuIntervalsRestTimePresenter(MenuIntervalsRestTimeView& v)
    : view(v)
{

}

void MenuIntervalsRestTimePresenter::activate()
{
    view.setTime(model->getSettings().intervals.restTime);
    model->resetIdleTimer();
}

void MenuIntervalsRestTimePresenter::deactivate()
{

}

void MenuIntervalsRestTimePresenter::saveRestTime(uint32_t totalSeconds)
{
    Settings sett = model->getSettings();
    if (totalSeconds > 0) {
        sett.intervals.restMetric = Settings::Intervals::TIME;
        sett.intervals.restTime   = totalSeconds;
    } else {
        sett.intervals.restMetric = Settings::Intervals::OPEN;
        sett.intervals.restTime   = 0;
    }
    model->saveSettings(sett);
}
