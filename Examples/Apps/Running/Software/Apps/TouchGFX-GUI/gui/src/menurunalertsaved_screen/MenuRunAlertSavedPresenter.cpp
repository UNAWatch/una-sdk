#include <gui/menurunalertsaved_screen/MenuRunAlertSavedView.hpp>
#include <gui/menurunalertsaved_screen/MenuRunAlertSavedPresenter.hpp>

MenuRunAlertSavedPresenter::MenuRunAlertSavedPresenter(MenuRunAlertSavedView& v)
    : view(v)
{
}

void MenuRunAlertSavedPresenter::activate()
{
    char value[16];
    const RunAlerts::Field f = model->runAlertField();
    RunAlerts::fieldText(f, model->getSettings().runAlerts, model->isUnitsImperial(), value, sizeof(value));
    view.setSaved(f, value);
}

void MenuRunAlertSavedPresenter::deactivate()
{
}
