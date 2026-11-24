#include <gui/menurunalertpicker_screen/MenuRunAlertPickerView.hpp>
#include <gui/menurunalertpicker_screen/MenuRunAlertPickerPresenter.hpp>

MenuRunAlertPickerPresenter::MenuRunAlertPickerPresenter(MenuRunAlertPickerView& v)
    : view(v)
{
}

void MenuRunAlertPickerPresenter::activate()
{
    model->resetIdleTimer();

    const RunAlerts::Field     f        = model->runAlertField();
    const Settings::RunAlerts& ra       = model->getSettings().runAlerts;
    const bool                 imperial = model->isUnitsImperial();
    const RunAlerts::Bounds    b        = RunAlerts::pickerBounds(f, ra, imperial);
    view.setValue(f, imperial,
                  RunAlerts::pickerSeed(f, ra, imperial, model->getHrThresholds(), model->getHrThresholdsCount()),
                  b.min, b.max);
}

void MenuRunAlertPickerPresenter::deactivate()
{
}

void MenuRunAlertPickerPresenter::save(uint32_t value)
{
    model->saveRunAlert(value);
}
