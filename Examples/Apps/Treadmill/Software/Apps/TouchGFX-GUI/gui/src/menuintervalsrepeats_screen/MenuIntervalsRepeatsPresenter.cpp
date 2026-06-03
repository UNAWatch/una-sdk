#include <gui/menuintervalsrepeats_screen/MenuIntervalsRepeatsView.hpp>
#include <gui/menuintervalsrepeats_screen/MenuIntervalsRepeatsPresenter.hpp>

MenuIntervalsRepeatsPresenter::MenuIntervalsRepeatsPresenter(MenuIntervalsRepeatsView& v)
    : view(v)
{

}

void MenuIntervalsRepeatsPresenter::activate()
{
    view.setPositionId(model->getSettings().intervals.repeatsNum);
    model->resetIdleTimer();
}

void MenuIntervalsRepeatsPresenter::deactivate()
{
    model->menu().intervals.repeats.set(view.getPositionId());
}

void MenuIntervalsRepeatsPresenter::saveRepeats(uint8_t repeatsNum)
{
    Settings sett = model->getSettings();
    sett.intervals.repeatsNum = repeatsNum;
    model->saveSettings(sett);
}
