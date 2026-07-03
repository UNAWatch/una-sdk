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

    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuIntervalsRepeatsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
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
