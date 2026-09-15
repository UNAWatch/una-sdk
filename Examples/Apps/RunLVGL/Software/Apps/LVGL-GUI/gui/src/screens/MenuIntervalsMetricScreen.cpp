/**
 ******************************************************************************
 * @file    MenuIntervalsMetricScreen.cpp
 * @brief   Run / Rest phase menu (see MenuIntervalsMetricScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuIntervalsMetricScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

using Style = WheelMenu::Item::Style;

namespace
{
uint16_t metricToMenuId(Settings::Intervals::Metric metric)
{
    using Menu = App::MenuNav::Root::Intervals::Metric;
    switch (metric) {
        case Settings::Intervals::TIME:     return Menu::ID_TIME;
        case Settings::Intervals::DISTANCE: return Menu::ID_DISTANCE;
        default:                            return Menu::ID_OPEN;
    }
}
} // namespace

MenuIntervalsMetricScreen::MenuIntervalsMetricScreen(Model& model, Phase phase)
    : Screen(model)
    , mPhase(phase)
{
}

void MenuIntervalsMetricScreen::build()
{
    mItems[Menu::ID_TIME]     = { Style::Tip,    "Time",     nullptr, &poppins_semibold_25, mTimeTip };
    mItems[Menu::ID_DISTANCE] = { Style::Tip,    "Distance", nullptr, &poppins_semibold_25, mDistTip };
    mItems[Menu::ID_OPEN]     = { Style::Simple, "Open" };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, mPhase == Phase::Run ? "RUN" : "REST");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuIntervalsMetricScreen::applySettings(const Settings::Intervals& iv)
{
    const bool     imperial = mModel.isUnitsImperial();
    const uint32_t time     = mPhase == Phase::Run ? iv.runTime : iv.restTime;
    const float    dist     = mPhase == Phase::Run ? iv.runDistance : iv.restDistance;
    // Each hint shows its own metric's stored value, "Open" when unset.
    Fmt::intervalsPhaseTip(mTimeTip, sizeof(mTimeTip), Settings::Intervals::TIME, time, 0.0f, imperial);
    Fmt::intervalsPhaseTip(mDistTip, sizeof(mDistTip), Settings::Intervals::DISTANCE, 0, dist, imperial);
    mMenu->refresh();
}

void MenuIntervalsMetricScreen::onShow()
{
    const Settings::Intervals& iv = mModel.getSettings().intervals;
    applySettings(iv);
    mMenu->select(metricToMenuId(mPhase == Phase::Run ? iv.runMetric : iv.restMetric));
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuIntervalsMetricScreen::onHide()
{
    if (mPhase == Phase::Run) {
        mModel.menu().intervals.run.set(mMenu->selected());
    } else {
        mModel.menu().intervals.rest.set(mMenu->selected());
    }
}

void MenuIntervalsMetricScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::MenuIntervals); break;
        default: break;
    }
}

void MenuIntervalsMetricScreen::confirm()
{
    const bool run = mPhase == Phase::Run;
    switch (mMenu->selected()) {
        case Menu::ID_TIME:
            ScreenManager::instance().goTo(run ? ScreenId::MenuIntervalsRunTime : ScreenId::MenuIntervalsRestTime);
            break;
        case Menu::ID_DISTANCE:
            ScreenManager::instance().goTo(run ? ScreenId::MenuIntervalsRunDistance
                                               : ScreenId::MenuIntervalsRestDistance);
            break;
        case Menu::ID_OPEN: {
            Settings sett = mModel.getSettings();
            (run ? sett.intervals.runMetric : sett.intervals.restMetric) = Settings::Intervals::OPEN;
            mModel.saveSettings(sett);
            ScreenManager::instance().goTo(ScreenId::MenuIntervals);
            break;
        }
        default:
            break;
    }
}

void MenuIntervalsMetricScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuIntervalsMetricScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
