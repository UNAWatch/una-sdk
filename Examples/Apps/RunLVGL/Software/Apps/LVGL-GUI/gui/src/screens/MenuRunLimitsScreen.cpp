/**
 ******************************************************************************
 * @file    MenuRunLimitsScreen.cpp
 * @brief   Settings > Run Alerts > Pace / Heart Rate (see MenuRunLimitsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuRunLimitsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"

using namespace SDK::GUI;
using Style = WheelMenu::Item::Style;

MenuRunLimitsScreen::MenuRunLimitsScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
{
}

RunAlerts::Field MenuRunLimitsScreen::field(uint16_t row) const
{
    if (mKind == Kind::Pace) {
        return row == 0 ? RunAlerts::Field::PaceFast : RunAlerts::Field::PaceSlow;
    }
    return row == 0 ? RunAlerts::Field::HrHigh : RunAlerts::Field::HrLow;
}

void MenuRunLimitsScreen::build()
{
    const bool pace = mKind == Kind::Pace;
    mItems[0] = { Style::Tip, pace ? "Too Fast" : "High", nullptr, &poppins_semibold_30, mTips[0] };
    mItems[1] = { Style::Tip, pace ? "Too Slow" : "Low",  nullptr, &poppins_semibold_30, mTips[1] };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, kRows);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, pace ? "PACE" : "HEART RATE");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuRunLimitsScreen::onShow()
{
    auto& nav = mModel.menu().settings.runAlerts;
    mMenu->select(mKind == Kind::Pace ? nav.pace.get() : nav.hr.get());
    mModel.resetIdleTimer();
    onSettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuRunLimitsScreen::onHide()
{
    auto& nav = mModel.menu().settings.runAlerts;
    if (mKind == Kind::Pace) {
        nav.pace.set(mMenu->selected());
    } else {
        nav.hr.set(mMenu->selected());
    }
}

void MenuRunLimitsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1:
            mModel.setRunAlertField(field(mMenu->selected()));
            ScreenManager::instance().goTo(mKind == Kind::Pace ? ScreenId::MenuRunPaceLimit
                                                               : ScreenId::MenuRunHrList);
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::MenuRunAlerts);
            break;
        default:
            break;
    }
}

void MenuRunLimitsScreen::onSettings(const Settings& settings)
{
    const bool imperial = mModel.isUnitsImperial();
    for (uint16_t row = 0; row < kRows; ++row) {
        const RunAlerts::Field f = field(row);
        RunAlerts::fieldText(f, settings.runAlerts, imperial, mTips[row], sizeof(mTips[row]));
        mItems[row].tipColor = RunAlerts::fieldValue(f, settings.runAlerts, imperial) > 0
                                   ? Color::YELLOW_DARK : Color::TEAL;
    }
    mMenu->refresh();
}

void MenuRunLimitsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuRunLimitsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
