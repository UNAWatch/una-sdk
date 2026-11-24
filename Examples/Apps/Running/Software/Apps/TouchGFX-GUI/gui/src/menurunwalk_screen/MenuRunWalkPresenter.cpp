#include <gui/menurunwalk_screen/MenuRunWalkView.hpp>
#include <gui/menurunwalk_screen/MenuRunWalkPresenter.hpp>

MenuRunWalkPresenter::MenuRunWalkPresenter(MenuRunWalkView& v)
    : view(v)
{
}

void MenuRunWalkPresenter::activate()
{
    view.setPositionId(model->menu().settings.runAlerts.runWalk.get());
    model->resetIdleTimer();

    view.refresh();
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuRunWalkPresenter::deactivate()
{
    model->menu().settings.runAlerts.runWalk.set(view.getPositionId());
}

void MenuRunWalkPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuRunWalkPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

const Settings::RunAlerts& MenuRunWalkPresenter::runAlerts() const
{
    return model->getSettings().runAlerts;
}

void MenuRunWalkPresenter::toggleRunWalk()
{
    Settings sett = model->getSettings();
    sett.runAlerts.runWalkEn = !sett.runAlerts.runWalkEn;
    model->saveSettings(sett);
}

void MenuRunWalkPresenter::edit(RunAlerts::Field field)
{
    model->setRunAlertField(field);
}
