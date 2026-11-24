/**
 ******************************************************************************
 * @file    MenuRunHrListScreen.cpp
 * @brief   HR High / HR Low list (see MenuRunHrListScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuRunHrListScreen.hpp"
#include "gui/screens/ScreenManager.hpp"

#include <cstdio>

MenuRunHrListScreen::MenuRunHrListScreen(Model& model)
    : Screen(model)
{
}

void MenuRunHrListScreen::build()
{
    // Off, then one row per bpm within the bounds.
    mBounds = RunAlerts::pickerBounds(mModel.runAlertField(), mModel.getSettings().runAlerts,
                                      mModel.isUnitsImperial());
    const uint16_t values = mBounds.empty() ? 0 : static_cast<uint16_t>(mBounds.max - mBounds.min + 1u);
    mRows  = static_cast<uint16_t>(values + 1u);
    mItems = std::make_unique<WheelMenu::Item[]>(mRows);
    mTexts = std::make_unique<char[][kTextLen]>(mRows);

    snprintf(mTexts[0], kTextLen, "Off");
    for (uint16_t i = 1; i < mRows; ++i) {
        snprintf(mTexts[i], kTextLen, "%u bpm", static_cast<unsigned>(mBounds.min + i - 1u));
    }
    for (uint16_t i = 0; i < mRows; ++i) {
        mItems[i].style = WheelMenu::Item::Style::Simple;
        mItems[i].text  = mTexts[i];
    }

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems.get(), mRows);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, RunAlerts::fieldTitle(mModel.runAlertField()));
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuRunHrListScreen::onShow()
{
    uint16_t pos = 0;
    if (!mBounds.empty()) {
        const uint32_t seed = RunAlerts::pickerSeed(mModel.runAlertField(), mModel.getSettings().runAlerts,
                                                    mModel.isUnitsImperial(), mModel.getHrThresholds(),
                                                    mModel.getHrThresholdsCount());
        pos = static_cast<uint16_t>(seed - mBounds.min + 1u);
    }
    mMenu->select(pos);
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuRunHrListScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: {
            const uint16_t row = mMenu->selected();
            mModel.saveRunAlert(row == 0 ? 0u : mBounds.min + row - 1u);
            ScreenManager::instance().goTo(ScreenId::MenuRunAlertSaved);
            break;
        }
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuRunHr);
            break;
        default:
            break;
    }
}

void MenuRunHrListScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuRunHrListScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
