#include <gui/menurunalerts_screen/MenuRunAlertsView.hpp>
#include <gui/menurunalerts_screen/MenuRunAlertsPresenter.hpp>

MenuRunAlertsPresenter::MenuRunAlertsPresenter(MenuRunAlertsView& v)
    : view(v)
{
}

void MenuRunAlertsPresenter::activate()
{
    view.setPositionId(model->menu().settings.runAlerts.get());
    model->menu().settings.runAlerts.resetChildren();
    model->resetIdleTimer();

    view.refresh();
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuRunAlertsPresenter::deactivate()
{
    model->menu().settings.runAlerts.set(view.getPositionId());
}

void MenuRunAlertsPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuRunAlertsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

const Settings::RunAlerts& MenuRunAlertsPresenter::runAlerts() const
{
    return model->getSettings().runAlerts;
}

bool MenuRunAlertsPresenter::isImperial() const
{
    return model->isUnitsImperial();
}

void MenuRunAlertsPresenter::toggleBackInTap()
{
    Settings sett = model->getSettings();
    sett.runAlerts.backInTap = !sett.runAlerts.backInTap;
    model->saveSettings(sett);
}

void MenuRunAlertsPresenter::openLimits(RunAlerts::Field field)
{
    model->setRunAlertField(field);
}
