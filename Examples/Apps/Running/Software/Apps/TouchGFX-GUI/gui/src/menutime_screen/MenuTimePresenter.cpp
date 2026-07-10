#include <gui/menutime_screen/MenuTimeView.hpp>
#include <gui/menutime_screen/MenuTimePresenter.hpp>

MenuTimePresenter::MenuTimePresenter(MenuTimeView& v)
    : view(v)
{

}

void MenuTimePresenter::activate()
{
    // Set current menu position
    view.setTime(model->getSettings().alertTimeId);

    // Reset idle timer
    model->resetIdleTimer();
}

void MenuTimePresenter::deactivate()
{

}

void MenuTimePresenter::saveTime(Settings::Alerts::Time::Id id)
{
    Settings sett = model->getSettings();
    sett.alertTimeId = id;
    // Distance and time auto-laps are mutually exclusive: enabling a time alert
    // disables any active distance alert.
    if (id != Settings::Alerts::Time::ID_OFF) {
        sett.alertDistanceId = Settings::Alerts::Distance::ID_OFF;
    }
    model->saveSettings(sett);
}
