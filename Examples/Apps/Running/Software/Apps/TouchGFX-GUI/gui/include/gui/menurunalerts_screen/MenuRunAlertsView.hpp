#ifndef MENURUNALERTSVIEW_HPP
#define MENURUNALERTSVIEW_HPP

#include <gui_generated/menurunalerts_screen/MenuRunAlertsViewBase.hpp>
#include <gui/menurunalerts_screen/MenuRunAlertsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Settings > Run Alerts: Run/Walk, Pace and Heart Rate (each a submenu, its
 * state as the hint) and the Back in Range toggle.
 */
class MenuRunAlertsView : public MenuRunAlertsViewBase
{
public:
    MenuRunAlertsView();
    virtual ~MenuRunAlertsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state, const char* name);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();
    /// Refresh the hints from the settings.
    void refresh();

protected:
    using Menu = App::MenuNav::Root::Settings::RunAlerts;

    bool mGpsFix      = false;
    bool mRunWalkOn   = false;
    bool mBackInTap   = false;
    char mPaceTip[16] = "";
    char mHrTip[16]   = "";

    touchgfx::Callback<MenuRunAlertsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuRunAlertsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void config(MenuItemConfig& cfg, int16_t index, bool center) const;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNALERTSVIEW_HPP
