/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Pre-activity menu (see MainScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Strings.hpp"
#include "gui/WorkoutUi.hpp"

#define LOG_MODULE_PRX      "MainScreen"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

using namespace SDK::GUI;

namespace
{
// Same items and geometry as MainView::setupItems() in the TouchGFX app.
using Style = WheelMenu::Item::Style;
const WheelMenu::Item kItems[App::MenuNav::Root::ID_COUNT] = {
    // ID_START: a Tip naming the workout when one is armed (onWorkoutList)
    { Style::Simple, "Start", nullptr, &poppins_semibold_35 },
    // ID_WORKOUTS
    { Style::Simple, "Workouts" },
    // ID_INTERVALS: icon beside left-aligned text, in both slots
    { Style::Icon, "Intervals", nullptr, &poppins_semibold_30, nullptr, Color::WHITE, false,
      &img_intervals_40x43, { 30, 10, 87, 140 },
      &img_intervals_24x26, { 62, 17, 97, 130 } },
    // ID_SETTINGS
    { Style::Simple, "Settings" },
};
} // namespace

MainScreen::MainScreen(Model& model)
    : Screen(model)
{
    for (uint16_t i = 0; i < Menu::ID_COUNT; ++i) {
        mItems[i] = kItems[i];
    }
}

void MainScreen::build()
{
    mMenu      = std::make_unique<WheelMenu>(mRoot, mItems, Menu::ID_COUNT);
    // As in MainView::onAnimationMiddle: the lens and R1 hint change half way
    // through the slide, when the incoming item is about to take the centre.
    mMenu->setSlideMidCallback(
        [](void* ctx, uint16_t) { static_cast<MainScreen*>(ctx)->updateBackground(); }, this);
    mButtons   = std::make_unique<Widgets::Buttons>(mRoot);
    mTitle     = std::make_unique<Widgets::Title>(mRoot, Strings::kAppNameUc);
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(mRoot, 0, 52, 240, 24);

    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
}

void MainScreen::onShow()
{
    mMenu->select(mModel.menu().get());
    mModel.menu().resetChildren();
    mModel.resetIdleTimer();
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
    onWorkoutList();
}

void MainScreen::onWorkoutList()
{
    // The armed workout's name under Start, shown when Start is selected.
    const SDK::Workout::Library* lib = mModel.getWorkoutLibrary();
    const int16_t armed = mModel.getArmedWorkout();
    mArmed = lib != nullptr && armed >= 0 && static_cast<size_t>(armed) < lib->size();
    WheelMenu::Item& start = mItems[Menu::ID_START];
    if (mArmed) {
        WorkoutUi::toText(lib->entry(static_cast<size_t>(armed)).name, mArmedName, sizeof(mArmedName));
        WorkoutUi::fitWidth(mArmedName, sizeof(mArmedName), Theme::font(Theme::Font::Italic18), 180);
        start.style   = Style::Tip;
        start.tip     = mArmedName;
        start.itemTip = "";
    } else {
        start.style   = Style::Simple;
        start.tip     = nullptr;
        start.itemTip = nullptr;
    }
    mMenu->refresh();
    checkTodayPrompt();
}

void MainScreen::checkTodayPrompt()
{
    if (mModel.getArmedWorkout() == Model::kNoWorkout && mModel.takeTodayPrompt()) {
        mModel.setChosenWorkout(mModel.getTodayWorkout());
        ScreenManager::instance().goTo(ScreenId::WorkoutToday);
    }
}

void MainScreen::onHide()
{
    mModel.menu().set(mMenu->selected());
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;   // lens follows at the slide midpoint
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2: mModel.exitApp(); break;
        default: break;
    }
}

void MainScreen::confirm()
{
    switch (mMenu->selected()) {
        case Menu::ID_START:
            if (mGpsFix && mArmed) {
                // A workout starts with the countdown, as Intervals do.
                mModel.setPendingIntervalsMode(false);
                ScreenManager::instance().goTo(ScreenId::TrackIntervalsCountdown);
            } else if (mGpsFix) {
                mModel.trackStart(false);
                ScreenManager::instance().goTo(ScreenId::Track);
            } else {
                mModel.setPendingIntervalsMode(false);
                ScreenManager::instance().goTo(ScreenId::TrackStartConfirm);
            }
            break;
        case Menu::ID_WORKOUTS:
            ScreenManager::instance().goTo(ScreenId::WorkoutList);
            break;
        case Menu::ID_INTERVALS:
            // No GPS-fix check here: the intervals menu is always reachable so
            // the workout can be configured indoors. The check happens on Start.
            ScreenManager::instance().goTo(ScreenId::MenuIntervals);
            break;
        case Menu::ID_SETTINGS:
            ScreenManager::instance().goTo(ScreenId::MenuSettings);
            break;
        default:
            break;
    }
}

void MainScreen::updateBackground()
{
    // Start without a fix is greyed out and R1 hidden; everything else is live.
    const bool startBlocked = mMenu->selected() == Menu::ID_START && !mGpsFix;
    mMenu->setBackground(startBlocked ? Color::GRAY_DARK : Color::TEAL_DARK);
    mButtons->setR1(startBlocked ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);
}

void MainScreen::onIdleTimeout()
{
    if (mMenu->selected() != Menu::ID_START) {
        mModel.exitApp();
    }
}

void MainScreen::onGpsFix(bool acquired)
{
    mGpsFix = acquired;
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
    updateBackground();
}

void MainScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrState(state));
}
