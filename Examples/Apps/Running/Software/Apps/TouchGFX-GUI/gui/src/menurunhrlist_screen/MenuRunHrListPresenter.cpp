#include <gui/menurunhrlist_screen/MenuRunHrListView.hpp>
#include <gui/menurunhrlist_screen/MenuRunHrListPresenter.hpp>

MenuRunHrListPresenter::MenuRunHrListPresenter(MenuRunHrListView& v)
    : view(v)
{
}

void MenuRunHrListPresenter::activate()
{
    model->resetIdleTimer();

    const RunAlerts::Field     f        = model->runAlertField();
    const Settings::RunAlerts& ra       = model->getSettings().runAlerts;
    const bool                 imperial = model->isUnitsImperial();
    const RunAlerts::Bounds    b        = RunAlerts::pickerBounds(f, ra, imperial);
    view.setList(f, b.empty(), b.min, b.max,
                 RunAlerts::pickerSeed(f, ra, imperial, model->getHrThresholds(), model->getHrThresholdsCount()));
    view.setGpsFix(model->hasGpsFix());
    view.setAccessoryStatus(model->getAccessoryState(), "");
}

void MenuRunHrListPresenter::deactivate()
{
}

void MenuRunHrListPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuRunHrListPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

void MenuRunHrListPresenter::save(uint32_t value)
{
    model->saveRunAlert(value);
}
