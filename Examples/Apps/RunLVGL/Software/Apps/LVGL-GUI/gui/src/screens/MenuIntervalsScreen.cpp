/**
 ******************************************************************************
 * @file    MenuIntervalsScreen.cpp
 * @brief   Intervals menu (see MenuIntervalsScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MenuIntervalsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/Assets.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Format.hpp"

using Style = WheelMenu::Item::Style;

MenuIntervalsScreen::MenuIntervalsScreen(Model& model)
    : Screen(model)
{
}

void MenuIntervalsScreen::build()
{
    // Same items and fonts as MenuIntervalsView::setupItems().
    mItems[Menu::ID_START]     = { Style::Simple, "Start",     nullptr,     &poppins_semibold_35 };
    mItems[Menu::ID_REPEATS]   = { Style::Tip,    "Repeats",   nullptr,     &poppins_semibold_25, mRepeatsTip };
    mItems[Menu::ID_RUN]       = { Style::Tip,    "Run",       nullptr,     &poppins_semibold_25, mRunTip };
    mItems[Menu::ID_REST]      = { Style::Tip,    "Rest",      nullptr,     &poppins_semibold_25, mRestTip };
    mItems[Menu::ID_WARM_UP]   = { Style::Toggle, "Warm Up",   nullptr,     &poppins_semibold_25 };
    mItems[Menu::ID_COOL_DOWN] = { Style::Toggle, "Cool\nDown", "Cool Down", &poppins_semibold_25 };
    mItems[Menu::ID_LAST_REST] = { Style::Toggle, "Last\nRest", "Last Rest", &poppins_semibold_25 };

    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, "INTERVALS");
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);
}

void MenuIntervalsScreen::onShow()
{
    mMenu->select(mModel.menu().intervals.get());
    mModel.menu().intervals.resetChildren();
    mModel.resetIdleTimer();
    applySettings(mModel.getSettings());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void MenuIntervalsScreen::onHide()
{
    mModel.menu().intervals.set(mMenu->selected());
}

void MenuIntervalsScreen::applySettings(const Settings& settings)
{
    const Settings::Intervals& iv = settings.intervals;
    const bool imperial = mModel.isUnitsImperial();
    Fmt::intervalsRepeats(mRepeatsTip, sizeof(mRepeatsTip), iv.repeatsNum);
    Fmt::intervalsPhaseTip(mRunTip, sizeof(mRunTip), iv.runMetric, iv.runTime, iv.runDistance, imperial);
    Fmt::intervalsPhaseTip(mRestTip, sizeof(mRestTip), iv.restMetric, iv.restTime, iv.restDistance, imperial);
    mItems[Menu::ID_WARM_UP].toggleState   = iv.warmUp;
    mItems[Menu::ID_COOL_DOWN].toggleState = iv.coolDown;
    mItems[Menu::ID_LAST_REST].toggleState = iv.lastRest;
    mMenu->refresh();
}

void MenuIntervalsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2: ScreenManager::instance().goTo(ScreenId::Main); break;
        default: break;
    }
}

void MenuIntervalsScreen::confirm()
{
    const uint16_t idx = mMenu->selected();
    switch (idx) {
        case Menu::ID_START:   startIntervals(); break;
        case Menu::ID_REPEATS: ScreenManager::instance().goTo(ScreenId::MenuIntervalsRepeats); break;
        case Menu::ID_RUN:     ScreenManager::instance().goTo(ScreenId::MenuIntervalsRun); break;
        case Menu::ID_REST:    ScreenManager::instance().goTo(ScreenId::MenuIntervalsRest); break;
        case Menu::ID_WARM_UP:
        case Menu::ID_COOL_DOWN:
        case Menu::ID_LAST_REST:
            saveToggle(idx, !mItems[idx].toggleState);
            break;
        default:
            break;
    }
}

void MenuIntervalsScreen::saveToggle(uint16_t index, bool state)
{
    Settings sett = mModel.getSettings();
    switch (index) {
        case Menu::ID_WARM_UP:   sett.intervals.warmUp   = state; break;
        case Menu::ID_COOL_DOWN: sett.intervals.coolDown = state; break;
        default:                 sett.intervals.lastRest = state; break;
    }
    // saveSettings() updates the model's copy; the service echoes it back later.
    mModel.saveSettings(sett);
    applySettings(sett);
}

void MenuIntervalsScreen::startIntervals()
{
    // With a fix, straight to the countdown; without one, via the warning.
    if (mModel.hasGpsFix()) {
        ScreenManager::instance().goTo(ScreenId::TrackIntervalsCountdown);
    } else {
        mModel.setPendingIntervalsMode(true);
        ScreenManager::instance().goTo(ScreenId::TrackStartConfirm);
    }
}

void MenuIntervalsScreen::onIdleTimeout()
{
    if (mMenu->selected() != Menu::ID_START) {
        mModel.exitApp();
    }
}

void MenuIntervalsScreen::onSettings(const Settings& settings)
{
    applySettings(settings);
}

void MenuIntervalsScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void MenuIntervalsScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
