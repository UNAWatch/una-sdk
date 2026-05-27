#include <gui/menusettings_screen/MenuSettingsView.hpp>
#include <gui/menusettings_screen/MenuSettingsPresenter.hpp>

MenuSettingsPresenter::MenuSettingsPresenter(MenuSettingsView& v)
    : view(v)
{

}

void MenuSettingsPresenter::activate()
{
    view.setPositionId(model->menu().settings.get());
    model->menu().settings.resetChildren();
    model->resetIdleTimer();

    view.setGpsFix(model->hasGpsFix());
    view.setPhoneNotif(model->getSettings().phoneNotifEn);
}

void MenuSettingsPresenter::deactivate()
{
    model->menu().settings.set(view.getPositionId());
}

void MenuSettingsPresenter::onGpsFix(bool acquired)
{
    view.setGpsFix(acquired);
}

void MenuSettingsPresenter::savePhoneNotif(bool state)
{
    Settings sett = model->getSettings();
    sett.phoneNotifEn = state;
    model->saveSettings(sett);
}
