#ifndef MENURUNLIMITSVIEW_HPP
#define MENURUNLIMITSVIEW_HPP

#include <gui_generated/menurunlimits_screen/MenuRunLimitsViewBase.hpp>
#include <gui/menurunlimits_screen/MenuRunLimitsPresenter.hpp>
#include <gui/containers/MenuItemConfig.hpp>

/**
 * Settings > Run Alerts > Pace (Too Fast, Too Slow) or Heart Rate (High,
 * Low): two limits, each OFF or its value as the hint. One screen for both
 * submenus; which one follows the field being edited (Model::runAlertField()).
 * A pace limit opens Off / Pace, a heart-rate limit the bpm list.
 */
class MenuRunLimitsView : public MenuRunLimitsViewBase
{
public:
    MenuRunLimitsView();
    virtual ~MenuRunLimitsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state, const char* name);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();
    /// Refresh the title and hints from the field and the settings.
    void refresh();

protected:
    static constexpr int16_t kRows = 2;  ///< Upper limit first: Too Fast / High

    RunAlerts::Field field(int16_t row) const;

    bool mGpsFix = false;
    bool mPace   = true;
    bool mOn[kRows]       = {};
    char mTips[kRows][16] = {};

    touchgfx::Callback<MenuRunLimitsView, MainMenuItem&, int16_t>       mUpdateItemCb;
    touchgfx::Callback<MenuRunLimitsView, MainMenuCenterItem&, int16_t> mUpdateCenterItemCb;

    void updateItem(MainMenuItem& item, int16_t index);
    void updateCenterItem(MainMenuCenterItem& item, int16_t index);
    void config(MenuItemConfig& cfg, int16_t index, bool center) const;

    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // MENURUNLIMITSVIEW_HPP
