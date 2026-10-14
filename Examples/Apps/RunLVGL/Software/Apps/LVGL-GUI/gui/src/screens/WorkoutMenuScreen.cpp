/**
 ******************************************************************************
 * @file    WorkoutMenuScreen.cpp
 * @brief   The menu for one workout (see the header).
 ******************************************************************************
 */

#include "gui/screens/WorkoutMenuScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include <cstdio>

using Style = WheelMenu::Item::Style;

WorkoutMenuScreen::WorkoutMenuScreen(Model& model, Kind kind)
    : Screen(model)
    , mKind(kind)
    , mCount(kind == Kind::Today ? static_cast<uint16_t>(Menu::ID_COUNT) : static_cast<uint16_t>(Menu::ID_SKIP))
{
}

uint16_t WorkoutMenuScreen::workout() const
{
    return mKind == Kind::Today ? mModel.getTodayWorkout() : mModel.getChosenWorkout();
}

void WorkoutMenuScreen::build()
{
    // The title says when it is planned for, or that it is from the library.
    char title[16] = "TODAY";
    const SDK::Workout::Library* lib = mModel.getWorkoutLibrary();
    const uint16_t index = workout();
    if (lib != nullptr && index < lib->size()) {
        const SDK::Workout::Library::Entry& e = lib->entry(index);
        WorkoutUi::toText(e.name, mName, sizeof(mName));
        WorkoutUi::fitWidth(mName, sizeof(mName), Theme::font(Theme::Font::Italic18), 170);
        WorkoutUi::formatTotals(e, mModel.isUnitsImperial(), mTotals, sizeof(mTotals));
        if (mKind == Kind::Chosen && !(e.date != 0 && e.date == mModel.getTodayYmd())) {
            if (e.date != 0) {
                WorkoutUi::formatDate(e.date, title, sizeof(title));
            } else {
                snprintf(title, sizeof(title), "WORKOUTS");
            }
        }
    }

    // Start carries the totals as its hint, shown only when it is selected.
    mItems[Menu::ID_START]   = { Style::Tip, "Start" };
    mItems[Menu::ID_START].tip     = mTotals;
    mItems[Menu::ID_START].itemTip = "";
    mItems[Menu::ID_DETAILS] = { Style::Simple, "Details" };
    mItems[Menu::ID_SKIP]    = { Style::Simple, "Skip" };

    mMenu    = std::make_unique<WheelMenu>(mRoot, mItems, mCount);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle   = std::make_unique<Widgets::Title>(mRoot, title);
    mInfo    = Theme::label(mRoot, Theme::Font::Italic18, mName, 35, 57, 170);
}

void WorkoutMenuScreen::onShow()
{
    mModel.resetIdleTimer();
    const uint16_t position = mModel.menu().workouts.menu.get();
    mMenu->select(position < mCount ? position : 0);
}

void WorkoutMenuScreen::onHide()
{
    mModel.menu().workouts.menu.set(mMenu->selected());
}

void WorkoutMenuScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1: mMenu->prev(); break;
        case Btn::L2: mMenu->next(); break;
        case Btn::R1: confirm(); break;
        case Btn::R2:
            ScreenManager::instance().goTo(mKind == Kind::Today ? ScreenId::Main : ScreenId::WorkoutList);
            break;
        default: break;
    }
}

void WorkoutMenuScreen::confirm()
{
    switch (mMenu->selected()) {
        case Menu::ID_START:
            mModel.armWorkout(static_cast<int16_t>(workout()));
            mModel.menu().set(App::MenuNav::Root::ID_START);
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        case Menu::ID_DETAILS:
            mModel.setChosenWorkout(workout());
            mModel.setDetailsFromToday(mKind == Kind::Today);
            ScreenManager::instance().goTo(ScreenId::WorkoutDetails);
            break;
        default:  // Skip
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
    }
}
