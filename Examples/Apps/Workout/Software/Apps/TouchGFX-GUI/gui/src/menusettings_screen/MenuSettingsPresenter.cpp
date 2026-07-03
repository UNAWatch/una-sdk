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

    view.setAccessoryStatus(model->getAccessoryState(), "");
    view.setPhoneNotif(model->getSettings().phoneNotifEn);
}

void MenuSettingsPresenter::deactivate()
{
    model->menu().settings.set(view.getPositionId());
}

void MenuSettingsPresenter::onAccessoryStatus(uint8_t state, const char* name)
{
    view.setAccessoryStatus(state, name);
}

void MenuSettingsPresenter::savePhoneNotif(bool state)
{
    Settings sett = model->getSettings();
    sett.phoneNotifEn = state;
    model->saveSettings(sett);
}
