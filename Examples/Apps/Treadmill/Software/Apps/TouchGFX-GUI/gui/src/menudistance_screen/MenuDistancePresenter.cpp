#include <gui/menudistance_screen/MenuDistanceView.hpp>
#include <gui/menudistance_screen/MenuDistancePresenter.hpp>

MenuDistancePresenter::MenuDistancePresenter(MenuDistanceView& v)
    : view(v)
{

}

void MenuDistancePresenter::activate()
{
    // Set current menu position and units
    view.setDistanceUnits(model->getSettings().alertDistanceId, model->isUnitsImperial());

    // Reset idle timer
    model->resetIdleTimer();
}

void MenuDistancePresenter::deactivate()
{

}

void MenuDistancePresenter::saveDistance(Settings::Alerts::Distance::Id id)
{
    Settings sett = model->getSettings();
    sett.alertDistanceId = id;
    model->saveSettings(sett);
}