/**
 ******************************************************************************
 * @file    WorkoutListScreen.cpp
 * @brief   The workouts on the watch (see the header).
 ******************************************************************************
 */

#include "gui/screens/WorkoutListScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include <cstdio>
#include <cstring>

namespace Color = SDK::GUI::Color;
using Style = WheelMenu::Item::Style;
using F     = Theme::Font;

namespace
{
/// A label centred on (120, @p cy).
lv_obj_t* centred(lv_obj_t* parent, F font, const char* text, int32_t cy, uint32_t color)
{
    lv_obj_t* l = Theme::label(parent, font, text, 0, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT, color);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, cy - SDK::LVGL::kCy);
    return l;
}
} // namespace

WorkoutListScreen::WorkoutListScreen(Model& model)
    : Screen(model)
{
}

WorkoutListScreen::~WorkoutListScreen() = default;

void WorkoutListScreen::build()
{
    mMenuBox = Theme::container(mRoot, 0, 0, 240, 240);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::AMBER, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "WORKOUTS");
    mInfo  = Theme::label(mRoot, F::Italic18, "", 35, 57, 170, LV_TEXT_ALIGN_CENTER, Color::TEAL);

    // Shown instead of the list when there are no workout files.
    mEmpty[0] = centred(mRoot, F::SemiBold25, "No workouts", 104, Color::WHITE);
    mEmpty[1] = centred(mRoot, F::Medium18, "Copy .fit files to", 142, Color::GRAY);
    mEmpty[2] = centred(mRoot, F::Medium18, "the Workouts folder", 165, Color::GRAY);
}

void WorkoutListScreen::onShow()
{
    mModel.resetIdleTimer();
    mModel.menu().workouts.resetChildren();
    setList(mModel.menu().workouts.get());
}

void WorkoutListScreen::onHide()
{
    if (mMenu) {
        mModel.menu().workouts.set(mMenu->selected());
    }
}

void WorkoutListScreen::onWorkoutList()
{
    setList(mMenu ? mMenu->selected() : mModel.menu().workouts.get());
}

int16_t WorkoutListScreen::workoutAt(uint16_t row) const
{
    return mFreeRunRow ? static_cast<int16_t>(row - 1) : static_cast<int16_t>(row);
}

void WorkoutListScreen::groupOf(uint16_t row, char* buf, size_t cap) const
{
    buf[0] = '\0';
    const int16_t i = workoutAt(row);
    if (mLibrary == nullptr || i < 0 || static_cast<size_t>(i) >= mLibrary->size()) {
        return;
    }
    const SDK::Workout::Library::Entry& e = mLibrary->entry(static_cast<size_t>(i));
    if (e.date != 0 && e.date == mModel.getTodayYmd()) {
        snprintf(buf, cap, "Today");
    } else if (e.date != 0) {
        WorkoutUi::formatDate(e.date, buf, cap);
        // "SAT 10 OCT" reads as "Sat 10 Oct" in a sentence.
        for (char* p = buf + 1; *p != '\0'; ++p) {
            if (p[-1] != ' ' && *p >= 'A' && *p <= 'Z') {
                *p = static_cast<char>(*p - 'A' + 'a');
            }
        }
    } else {
        snprintf(buf, cap, "Library");
    }
}

void WorkoutListScreen::showGroup(uint16_t row)
{
    const bool show = mRows > 0 && workoutAt(row) >= 0;
    Theme::setHidden(mInfo, !show);
    if (show) {
        lv_label_set_text(mInfo, mText[row].group);
    }
}

void WorkoutListScreen::setList(uint16_t position)
{
    // The wheel is built for a fixed number of rows: start it afresh.
    mMenu.reset();
    lv_obj_clean(mMenuBox);
    // The last list's rows, now the wheel that pointed into them is gone, so
    // the old and new rows never exist together.
    mItems.reset();
    mText.reset();

    mLibrary   = mModel.getWorkoutLibrary();
    mFreeRunRow = mModel.getArmedWorkout() != Model::kNoWorkout;
    const size_t workouts = mLibrary != nullptr ? mLibrary->size() : 0;
    mRows = static_cast<uint16_t>(workouts + (mFreeRunRow ? 1 : 0));
    if (mRows > kMaxRows) {
        mRows = kMaxRows;
    }
    mItems = std::make_unique<WheelMenu::Item[]>(mRows);
    mText  = std::make_unique<RowText[]>(mRows);

    const bool imperial = mModel.isUnitsImperial();
    for (uint16_t row = 0; row < mRows; ++row) {
        WheelMenu::Item& item = mItems[row];
        RowText&         text = mText[row];
        const int16_t    i    = workoutAt(row);
        if (i < 0) {
            item = { Style::Tip, "Free run" };
            item.tip     = "Clear the workout";
            item.itemTip = "";
            continue;
        }
        const SDK::Workout::Library::Entry& e = mLibrary->entry(static_cast<size_t>(i));
        WorkoutUi::toText(e.name, text.text, sizeof(text.text));
        std::memcpy(text.itemText, text.text, sizeof(text.itemText));
        WorkoutUi::fitWidth(text.text, sizeof(text.text), Theme::font(F::SemiBold25), 200);
        WorkoutUi::fitWidth(text.itemText, sizeof(text.itemText), Theme::font(F::Medium18), 190);

        // The totals; "!" in amber when a step runs in a simplified form.
        char totals[32];
        WorkoutUi::formatTotals(e, imperial, totals, sizeof(totals));
        snprintf(text.tip, sizeof(text.tip), "%s%s", e.degraded ? "! " : "", totals);
        groupOf(row, text.group, sizeof(text.group));

        item = { Style::Tip, text.text, text.itemText, Theme::font(F::SemiBold25), text.tip, Color::TEAL };
        item.itemTip        = text.group;
        item.centerTipColor = e.degraded ? Color::YELLOW_DARK : Color::WHITE;
    }

    const bool empty = mRows == 0;
    Theme::setHidden(mMenuBox, empty);
    for (lv_obj_t* l : mEmpty) {
        Theme::setHidden(l, !empty);
    }
    mButtons->setR1(empty ? Widgets::Buttons::NONE : Widgets::Buttons::AMBER);
    if (empty) {
        showGroup(0);
        return;
    }

    mMenu = std::make_unique<WheelMenu>(mMenuBox, mItems.get(), mRows);
    // A lone row would wrap round to show again below itself.
    mMenu->setCircular(mRows > 1);
    // The line above the wheel changes half way through the slide.
    mMenu->setSlideMidCallback(
        [](void* ctx, uint16_t target) { static_cast<WorkoutListScreen*>(ctx)->showGroup(target); }, this);
    mMenu->select(position < mRows ? position : 0);
    showGroup(mMenu->selected());
}

void WorkoutListScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            if (mMenu) {
                mMenu->prev();
            }
            break;
        case Btn::L2:
            if (mMenu) {
                mMenu->next();
            }
            break;
        case Btn::R1:
            if (mMenu) {
                const int16_t i = workoutAt(mMenu->selected());
                if (i < 0) {
                    mModel.armWorkout(Model::kNoWorkout);
                    mModel.menu().workouts.reset();
                    ScreenManager::instance().goTo(ScreenId::Main);
                } else {
                    mModel.setChosenWorkout(static_cast<uint16_t>(i));
                    ScreenManager::instance().goTo(ScreenId::WorkoutMenu);
                }
            }
            break;
        case Btn::R2:
            ScreenManager::instance().goTo(ScreenId::Main);
            break;
        default:
            break;
    }
}
