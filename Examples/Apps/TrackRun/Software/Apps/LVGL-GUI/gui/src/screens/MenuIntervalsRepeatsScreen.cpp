/**
 ******************************************************************************
 * @file    MenuIntervalsRepeatsScreen.cpp
 * @brief   Repeats picker (see MenuIntervalsRepeatsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuIntervalsRepeatsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Format.hpp"

MenuIntervalsRepeatsScreen::MenuIntervalsRepeatsScreen(Model& model)
    : Screen(model)
{
}

void MenuIntervalsRepeatsScreen::build()
{
    for (uint16_t i = 0; i < Menu::kMaxCount; ++i) {
        Fmt::intervalsRepeats(mTexts[i], sizeof(mTexts[i]), static_cast<uint8_t>(i));
        mItems[i].style = WheelMenu::Item::Style::Simple;
        mItems[i].text  = mTexts[i];
    }
    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::kMaxCount);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "REPEATS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuIntervalsRepeatsScreen::onShow()
{
    // The wheel position is the repeat count itself (0 = Open).
    uint16_t pos = mModel.getSettings().intervals.repeatsNum;
    if (pos >= Menu::kMaxCount) {
        pos = Menu::kMaxCount - 1;
    }
    mMenu->select(pos);
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuIntervalsRepeatsScreen::onHide()
{
    mModel.menu().intervals.repeats.set(mMenu->selected());
}

void MenuIntervalsRepeatsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            Settings sett = mModel.getSettings();
            sett.intervals.repeatsNum = static_cast<uint8_t>(mMenu->selected());
            mModel.saveSettings(sett);
            ScreenManager::instance().goTo(ScreenId::MenuIntervals);
            break;
        }
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuIntervals);
            break;
        default:
            break;
    }
}

void MenuIntervalsRepeatsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuIntervalsRepeatsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
