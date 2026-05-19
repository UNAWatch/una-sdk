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
    model->saveSettings(sett);
}