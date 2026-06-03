#ifndef MENUSETTINGSVIEW_HPP
#define MENUSETTINGSVIEW_HPP

#include <gui_generated/menusettings_screen/MenuSettingsViewBase.hpp>
#include <gui/menusettings_screen/MenuSettingsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

class MenuSettingsView : public MenuSettingsViewBase
{
public:
    MenuSettingsView();
    virtual ~MenuSettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setPhoneNotif(bool state);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

protected:
    using Menu = App::MenuNav::Root::Settings;

    bool mGpsFix     = false;
    bool mPhoneNotif = false;

    touchgfx::Callback<MenuSettingsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuSettingsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENUSETTINGSVIEW_HPP
