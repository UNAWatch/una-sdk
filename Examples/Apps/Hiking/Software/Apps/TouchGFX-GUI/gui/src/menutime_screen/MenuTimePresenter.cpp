#include <gui/menutime_screen/MenuTimeView.hpp>
#include <gui/menutime_screen/MenuTimePresenter.hpp>

MenuTimePresenter::MenuTimePresenter(MenuTimeView& v)
    : view(v)
{

}

void MenuTimePresenter::activate()
{
    view.setTime(model->getSettings().alertTimeId);
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
