#ifndef MENURUNPACELIMITVIEW_HPP
#define MENURUNPACELIMITVIEW_HPP

#include <gui_generated/menurunpacelimit_screen/MenuRunPaceLimitViewBase.hpp>
#include <gui/menurunpacelimit_screen/MenuRunPaceLimitPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Too Fast or Too Slow (Model::runAlertField()): Off, or Pace, which opens the
 * min:sec picker. As Intervals' Run and Rest open Time / Distance / Open.
 */
class MenuRunPaceLimitView : public MenuRunPaceLimitViewBase
{
public:
    MenuRunPaceLimitView();
    virtual ~MenuRunPaceLimitView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state, const char* name);
    /// Title and the Pace row's hint from the field; opens on Pace when it is set.
    void refresh();

protected:
    using Menu = App::MenuNav::Root::Settings::RunAlerts::PaceLimit;

    bool mGpsFix      = false;
    char mPaceTip[16] = "";

    touchgfx::Callback<MenuRunPaceLimitView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuRunPaceLimitView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void config(MenuItemConfig& cfg, int16_t index, bool center) const;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNPACELIMITVIEW_HPP
