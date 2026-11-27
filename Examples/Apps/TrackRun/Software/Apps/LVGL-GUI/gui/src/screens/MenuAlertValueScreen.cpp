/**
 ******************************************************************************
 * @file    MenuAlertValueScreen.cpp
 * @brief   Auto-lap value lists (see MenuAlertValueScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuAlertValueScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Format.hpp"

static_assert(Settings::Alerts::Distance::ID_COUNT == 7 && Settings::Alerts::Time::ID_COUNT == 7,
              "alert value lists are sized for 7 entries");

MenuAlertValueScreen::MenuAlertValueScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
{
}

void MenuAlertValueScreen::build()
{
    const bool imperial = mModel.isUnitsImperial();
    for (uint16_t i = 0; i < kMaxCount; ++i) {
        if (mKind == Kind::Distance) {
            Fmt::alertDistance(mCenterTexts[i], sizeof(mCenterTexts[i]), static_cast<DistanceMenu::Id>(i), imperial);
            mItems[i] = { WheelMenu::Item::Style::Simple, mCenterTexts[i] };
        } else {
            // The selected item spells "minutes" out; the surrounding one keeps "min".
            Fmt::alertTime(mCenterTexts[i], sizeof(mCenterTexts[i]), static_cast<TimeMenu::Id>(i), true);
            Fmt::alertTime(mItemTexts[i], sizeof(mItemTexts[i]), static_cast<TimeMenu::Id>(i), false);
            mItems[i] = { WheelMenu::Item::Style::Simple, mCenterTexts[i], mItemTexts[i] };
        }
    }
    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, kMaxCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, mKind == Kind::Distance ? "DISTANCE" : "TIME");
}

void MenuAlertValueScreen::onShow()
{
    const Settings& s = mModel.getSettings();
    mMenu->select(mKind == Kind::Distance ? static_cast<uint16_t>(s.alertDistanceId)
                                          : static_cast<uint16_t>(s.alertTimeId));
    mModel.resetIdleTimer();
}

void MenuAlertValueScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            save();
            ScreenManager::instance().goTo(mKind == Kind::Distance ? ScreenId::MenuAlertDistanceSaved
                                                                   : ScreenId::MenuAlertTimeSaved);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuAlerts);
            break;
        default:
            break;
    }
}

void MenuAlertValueScreen::save()
{
    Settings sett = mModel.getSettings();
    // Distance and time auto-laps are mutually exclusive.
    if (mKind == Kind::Distance) {
        sett.alertDistanceId = static_cast<DistanceMenu::Id>(mMenu->selected());
        if (sett.alertDistanceId != DistanceMenu::ID_OFF) {
            sett.alertTimeId = TimeMenu::ID_OFF;
        }
    } else {
        sett.alertTimeId = static_cast<TimeMenu::Id>(mMenu->selected());
        if (sett.alertTimeId != TimeMenu::ID_OFF) {
            sett.alertDistanceId = DistanceMenu::ID_OFF;
        }
    }
    mModel.saveSettings(sett);
}
