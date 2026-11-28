/**
 ******************************************************************************
 * @file    MenuRunHrListScreen.hpp
 * @brief   HR High or HR Low (Model::runAlertField()): a list of Off, then
 *          the bpm values RunAlerts::pickerBounds() allows (40 to 220,
 *          narrowed by the other limit), as Intervals' Repeats list starts
 *          with Open.
 *
 * It opens at the current value, or when Off at RunAlerts::pickerSeed()'s
 * suggestion (the start of zone 5 for High, zone 3 for Low). The rows are
 * built when the screen opens, sized to the bounds (at most 182).
 ******************************************************************************
 */

#ifndef MENU_RUN_HR_LIST_SCREEN_HPP
#define MENU_RUN_HR_LIST_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WheelMenu.hpp"

class MenuRunHrListScreen : public Screen
{
public:
    explicit MenuRunHrListScreen(Model& model);

    void onShow() override;
    void onKey(uint8_t code) override;
    /// Idle on a menu screen leaves the app, as the TouchGFX presenter does.
    void onIdleTimeout() override { mModel.exitApp(); }
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;

protected:
    void build() override;

private:
    static constexpr uint16_t kTextLen = 8;  ///< "220 bpm"

    RunAlerts::Bounds mBounds {0, 0};
    uint16_t          mRows = 0;
    std::unique_ptr<WheelMenu::Item[]>   mItems;
    std::unique_ptr<char[][kTextLen]>    mTexts;

    std::unique_ptr<Widgets::Title>           mTitle;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;
    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<WheelMenu>                mMenu;
};

#endif // MENU_RUN_HR_LIST_SCREEN_HPP
