#include <gui/menudistance_screen/MenuDistanceView.hpp>
#include <gui/menudistance_screen/MenuDistancePresenter.hpp>

MenuDistancePresenter::MenuDistancePresenter(MenuDistanceView& v)
    : view(v)
{

}

void MenuDistancePresenter::activate()
{
    view.setDistanceUnits(model->getSettings().alertDistanceId, model->isUnitsImperial());
    model->resetIdleTimer();
}

void MenuDistancePresenter::deactivate()
{

}

void MenuDistancePresenter::saveDistance(Settings::Alerts::Distance::Id id)
{
    Settings sett = model->getSettings();
    sett.alertDistanceId = id;
    // Distance and time auto-laps are mutually exclusive: enabling a distance
    // alert disables any active time alert.
    if (id != Settings::Alerts::Distance::ID_OFF) {
        sett.alertTimeId = Settings::Alerts::Time::ID_OFF;
    }
    model->saveSettings(sett);
}
