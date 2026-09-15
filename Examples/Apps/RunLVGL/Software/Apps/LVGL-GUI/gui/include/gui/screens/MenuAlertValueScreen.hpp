/**
 ******************************************************************************
 * @file    MenuAlertValueScreen.hpp
 * @brief   Auto-lap value lists: the distance list (OFF, 1..10 km/mi) or the
 *          time list (OFF, 1..30 min).
 *
 * One class for the Run app's MenuDistanceView and MenuTimeView. Saving one
 * kind of alert switches the other off, as auto-laps are exclusive.
 ******************************************************************************
 */

#ifndef MENU_ALERT_VALUE_SCREEN_HPP
#define MENU_ALERT_VALUE_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuAlertValueScreen : public Screen
{
public:
    enum class Kind : uint8_t { Distance, Time };

    MenuAlertValueScreen(Model& model, Kind kind);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }

protected:
    void build() override;

private:
    using DistanceMenu = Settings::Alerts::Distance;
    using TimeMenu     = Settings::Alerts::Time;
    static constexpr uint16_t kMaxCount = 7;   // both lists have 7 entries

    void save();

    Kind mKind;
    WheelMenu::Item mItems[kMaxCount] {};
    char mCenterTexts[kMaxCount][16] {};
    char mItemTexts[kMaxCount][16] {};

    std::unique_ptr<Widgets::Title>   mTitle;
    std::unique_ptr<Widgets::Buttons> mButtons;
    std::unique_ptr<WheelMenu>        mMenu;
};

#endif // MENU_ALERT_VALUE_SCREEN_HPP
