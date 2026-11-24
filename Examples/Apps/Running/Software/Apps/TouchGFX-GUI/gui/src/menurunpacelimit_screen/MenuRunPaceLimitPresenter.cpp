#include <gui/menurunpacelimit_screen/MenuRunPaceLimitView.hpp>
#include <gui/menurunpacelimit_screen/MenuRunPaceLimitPresenter.hpp>

MenuRunPaceLimitPresenter::MenuRunPaceLimitPresenter(MenuRunPaceLimitView& v)
    : view(v)
{
}

void MenuRunPaceLimitPresenter::activate()
{
    model->resetIdleTimer();
    view.refresh();
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuRunPaceLimitPresenter::deactivate()
{
}

void MenuRunPaceLimitPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuRunPaceLimitPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

const Settings::RunAlerts& MenuRunPaceLimitPresenter::runAlerts() const
{
    return model->getSettings().runAlerts;
}

bool MenuRunPaceLimitPresenter::isImperial() const
{
    return model->isUnitsImperial();
}

RunAlerts::Field MenuRunPaceLimitPresenter::field() const
{
    return model->runAlertField();
}

bool MenuRunPaceLimitPresenter::canPick() const
{
    return !RunAlerts::pickerBounds(model->runAlertField(), model->getSettings().runAlerts,
                                    model->isUnitsImperial()).empty();
}

void MenuRunPaceLimitPresenter::save(uint32_t value)
{
    model->saveRunAlert(value);
}
