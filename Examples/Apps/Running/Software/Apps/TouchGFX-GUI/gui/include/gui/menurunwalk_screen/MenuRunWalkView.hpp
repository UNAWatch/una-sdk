#ifndef MENURUNWALKVIEW_HPP
#define MENURUNWALKVIEW_HPP

#include <gui_generated/menurunwalk_screen/MenuRunWalkViewBase.hpp>
#include <gui/menurunwalk_screen/MenuRunWalkPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Settings > Run Alerts > Run/Walk: the on/off toggle (its label reads "Off"
 * or "On") and the Run Time and Walk Time pickers.
 */
class MenuRunWalkView : public MenuRunWalkViewBase
{
public:
    MenuRunWalkView();
    virtual ~MenuRunWalkView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state, const char* name);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();
    /// Refresh the hints from the settings.
    void refresh();

protected:
    using Menu = App::MenuNav::Root::Settings::RunAlerts::RunWalk;

    bool mGpsFix      = false;
    bool mOn          = false;
    char mRunTip[16]  = "";
    char mWalkTip[16] = "";

    touchgfx::Callback<MenuRunWalkView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuRunWalkView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void config(MenuItemConfig& cfg, int16_t index, bool center) const;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNWALKVIEW_HPP
