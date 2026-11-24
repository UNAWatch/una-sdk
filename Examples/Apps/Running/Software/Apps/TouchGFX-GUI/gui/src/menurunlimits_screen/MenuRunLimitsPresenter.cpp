#include <gui/menurunlimits_screen/MenuRunLimitsView.hpp>
#include <gui/menurunlimits_screen/MenuRunLimitsPresenter.hpp>

MenuRunLimitsPresenter::MenuRunLimitsPresenter(MenuRunLimitsView& v)
    : view(v)
{
}

void MenuRunLimitsPresenter::activate()
{
    auto& nav = model->menu().settings.runAlerts;
    view.setPositionId(RunAlerts::isPace(model->runAlertField()) ? nav.pace.get() : nav.hr.get());
    model->resetIdleTimer();

    view.refresh();
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuRunLimitsPresenter::deactivate()
{
    auto& nav = model->menu().settings.runAlerts;
    if (RunAlerts::isPace(model->runAlertField())) {
        nav.pace.set(view.getPositionId());
    } else {
        nav.hr.set(view.getPositionId());
    }
}

void MenuRunLimitsPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuRunLimitsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

const Settings::RunAlerts& MenuRunLimitsPresenter::runAlerts() const
{
    return model->getSettings().runAlerts;
}

bool MenuRunLimitsPresenter::isImperial() const
{
    return model->isUnitsImperial();
}

RunAlerts::Field MenuRunLimitsPresenter::field() const
{
    return model->runAlertField();
}

void MenuRunLimitsPresenter::edit(RunAlerts::Field field)
{
    model->setRunAlertField(field);
}
