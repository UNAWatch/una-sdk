/**
 ******************************************************************************
 * @file    WorkoutDetailsScreen.cpp
 * @brief   A workout in full (see the header).
 ******************************************************************************
 */

#include "gui/screens/WorkoutDetailsScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/WorkoutUi.hpp"

#include "SDK/Workout/StepLabel.hpp"

#include <cstdio>
#include <cstring>

namespace Color = SDK::GUI::Color;
using Style = WheelMenu::Item::Style;
using F     = Theme::Font;

namespace
{
constexpr int32_t kName1Y = 66;
constexpr int32_t kName2Y = 94;
} // namespace

WorkoutDetailsScreen::WorkoutDetailsScreen(Model& model)
    : Screen(model)
{
}

WorkoutDetailsScreen::~WorkoutDetailsScreen() = default;

void WorkoutDetailsScreen::build()
{
    mMenuBox = Theme::container(mRoot, 0, 0, 240, 240);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    // View only: R2 goes back.
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::WHITE);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "DETAILS");
    mInfo  = Theme::label(mRoot, F::Italic18, "", 35, 57, 170, LV_TEXT_ALIGN_CENTER, Color::TEAL);

    mName1.init(mRoot, F::SemiBold25, Color::WHITE);
    mName2.init(mRoot, F::SemiBold25, Color::WHITE);
    mTotals.init(mRoot, F::Italic18, Color::GRAY);
    mDivider = Theme::box(mRoot, 40, 0, 160, 3, Color::TEAL);
    for (WorkoutLabel& line : mDesc) {
        line.init(mRoot, F::Medium18, Color::WHITE);
    }
}

void WorkoutDetailsScreen::onShow()
{
    mModel.resetIdleTimer();
    mImperial = mModel.isUnitsImperial();
    showFirstPage(true);

    const SDK::Workout::Library* lib = mModel.getWorkoutLibrary();
    const uint16_t chosen = mModel.getChosenWorkout();
    if (lib != nullptr && chosen < lib->size()) {
        setEntry(lib->entry(chosen));
        // The steps follow once the service has read the file.
        mModel.requestWorkoutDetails(chosen);
    }
}

void WorkoutDetailsScreen::onWorkoutDetails()
{
    setProgram(mModel.getWorkoutDetails());
}

void WorkoutDetailsScreen::setEntry(const SDK::Workout::Library::Entry& entry)
{
    // The name over up to two lines, then the totals under the last one.
    char name[64];
    char line1[64];
    char line2[64];
    WorkoutUi::toText(entry.name, name, sizeof(name));
    WorkoutUi::wrapTwo(name, line1, line2, sizeof(line1), mName1.font(),
                       WorkoutUi::lineWidth(kName1Y - 15, kName1Y + 15),
                       WorkoutUi::lineWidth(kName2Y - 15, kName2Y + 15));
    mName1.setText(line1);
    mName2.setText(line2);
    const bool twoLines = line2[0] != '\0';
    mName1.setAnchor(120, kName1Y);
    mName2.setAnchor(120, kName2Y);

    const int32_t y = twoLines ? kName2Y : kName1Y;
    char totals[32];
    WorkoutUi::formatTotals(entry, mImperial, totals, sizeof(totals));
    mTotals.setAnchor(120, y + 26);
    mTotals.setText(totals);
    lv_obj_set_y(mDivider, y + 45);
    mDescY = y + 70;
    for (int k = 0; k < 3; ++k) {
        mDesc[k].setAnchor(120, mDescY + 24 * k);
    }
}

void WorkoutDetailsScreen::setProgram(const SDK::Workout::Program* program)
{
    mProgram  = program;
    mRowCount = 0;
    mMenu.reset();
    lv_obj_clean(mMenuBox);
    if (mProgram == nullptr) {
        // The file could not be read: the first page keeps only what the
        // list knew, and there are no steps to go to.
        for (WorkoutLabel& line : mDesc) {
            line.setText("");
        }
        return;
    }

    // The description, word-wrapped over three lines and cut after the last:
    // no more of it than three lines can hold.
    char text[160];
    WorkoutUi::toText(mProgram->description, text, sizeof(text));
    const char* rest = text;
    for (int k = 0; k < 3; ++k) {
        const int32_t cy = mDescY + 24 * k;
        const int32_t w  = WorkoutUi::lineWidth(cy - 11, cy + 11);
        while (*rest == ' ') {
            ++rest;
        }
        char line[64];
        if (k == 2) {
            std::strncpy(line, rest, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
            WorkoutUi::fitWidth(line, sizeof(line), mDesc[k].font(), w);
        } else {
            // wrapTwo lays the first line; what is left carries on below.
            char after[64];
            rest += WorkoutUi::wrapTwo(rest, line, after, sizeof(line), mDesc[k].font(), w, 32767);
        }
        mDesc[k].setText(line);
    }

    // Rows in reading order: each repeat heads the steps it repeats, outer
    // repeats first; a repeat step itself comes after its block in the file.
    const uint16_t n = mProgram->stepCount;
    for (uint16_t i = 0; i < n && mRowCount < SDK::Workout::kMaxSteps; ++i) {
        for (uint16_t r = n; r-- > i;) {
            const SDK::Workout::Step& s = mProgram->steps[r];
            if (s.end == SDK::Workout::StepEnd::Repeat && s.repeatFrom == i && mRowCount < SDK::Workout::kMaxSteps) {
                mRows[mRowCount++] = Row{ r, true };
            }
        }
        if (mProgram->steps[i].end != SDK::Workout::StepEnd::Repeat && mRowCount < SDK::Workout::kMaxSteps) {
            mRows[mRowCount++] = Row{ i, false };
        }
    }
    if (mRowCount == 0) {
        return;
    }

    for (uint16_t row = 0; row < mRowCount; ++row) {
        RowText& t = mText[row];
        rowText(row, t.itemLabel, t.itemTip, sizeof(t.itemLabel));
        std::memcpy(t.label, t.itemLabel, sizeof(t.label));
        std::memcpy(t.tip, t.itemTip, sizeof(t.tip));
        WorkoutUi::fitWidth(t.label, sizeof(t.label), Theme::font(F::SemiBold30), 200);
        WorkoutUi::fitWidth(t.tip, sizeof(t.tip), Theme::font(F::Italic18), 200);
        mItems[row] = { Style::Tip, t.label, t.itemLabel, nullptr, t.tip, Color::TEAL };
        mItems[row].itemTip = t.itemTip;
    }
    mMenu = std::make_unique<WheelMenu>(mMenuBox, mItems, mRowCount);
    // The steps run from first to last: nothing shows past either end.
    mMenu->setCircular(false);
    mMenu->setSlideMidCallback(
        [](void* ctx, uint16_t target) { static_cast<WorkoutDetailsScreen*>(ctx)->showRow(target); }, this);
    showFirstPage(mFirstPage);
}

void WorkoutDetailsScreen::showFirstPage(bool show)
{
    mFirstPage = show;
    Theme::setHidden(mMenuBox, show);
    mName1.setVisible(show);
    mName2.setVisible(show);
    mTotals.setVisible(show);
    Theme::setHidden(mDivider, !show);
    for (WorkoutLabel& line : mDesc) {
        line.setVisible(show);
    }
    if (show) {
        Theme::setHidden(mInfo, true);
        mTitle->setText("DETAILS");
    } else if (mMenu) {
        showRow(mMenu->selected());
    }
}

void WorkoutDetailsScreen::showRow(uint16_t row)
{
    if (mProgram == nullptr || row >= mRowCount) {
        return;
    }
    snprintf(mTitleText, sizeof(mTitleText), "%u/%u", static_cast<unsigned>(row + 1),
             static_cast<unsigned>(mRowCount));
    mTitle->setText(mTitleText);

    // The info line: the step's notes, else the repeats it is in.
    const Row& r = mRows[row];
    const SDK::Workout::Step& s = mProgram->steps[r.step];
    const uint16_t from = r.repeat ? s.repeatFrom : r.step;
    char context[32] = "";
    size_t len = 0;
    for (uint16_t k = r.repeat ? static_cast<uint16_t>(r.step + 1) : 0; k < mProgram->stepCount; ++k) {
        const SDK::Workout::Step& o = mProgram->steps[k];
        // A repeat encloses this row if its block starts at or before it and
        // the repeat step itself comes after it.
        const bool encloses = o.end == SDK::Workout::StepEnd::Repeat && o.repeatFrom <= from && k > r.step;
        if (!encloses || len + 8 >= sizeof(context)) {
            continue;
        }
        if (o.repeatCount == SDK::Workout::kRepeatForever) {
            len += static_cast<size_t>(snprintf(context + len, sizeof(context) - len, "%sopen",
                                                len ? " in " : r.repeat ? "in " : ""));
        } else {
            len += static_cast<size_t>(snprintf(context + len, sizeof(context) - len, "%sx%lu",
                                                len ? " in " : r.repeat ? "in " : "",
                                                static_cast<unsigned long>(o.repeatCount)));
        }
    }
    char info[64];
    if (!r.repeat && s.notes[0] != '\0') {
        WorkoutUi::toText(s.notes, info, sizeof(info));
    } else {
        snprintf(info, sizeof(info), "%s", context);
    }
    WorkoutUi::fitWidth(info, sizeof(info), Theme::font(F::Italic18), 170);
    lv_label_set_text(mInfo, info);
    Theme::setHidden(mInfo, info[0] == '\0');
}

void WorkoutDetailsScreen::rowText(uint16_t row, char* label, char* tip, size_t cap) const
{
    label[0] = '\0';
    tip[0]   = '\0';
    if (mProgram == nullptr || row >= mRowCount) {
        return;
    }
    const Row& r = mRows[row];
    const SDK::Workout::Step& s = mProgram->steps[r.step];

    if (r.repeat) {
        // "Repeat x5", and how many steps it repeats.
        if (s.repeatCount == SDK::Workout::kRepeatForever) {
            snprintf(label, cap, "Repeat");
            snprintf(tip, cap, "Open");
        } else {
            // The steps it repeats, not counting the repeats inside it.
            unsigned steps = 0;
            for (uint16_t k = s.repeatFrom; k < r.step; ++k) {
                steps += mProgram->steps[k].end != SDK::Workout::StepEnd::Repeat ? 1u : 0u;
            }
            snprintf(label, cap, "Repeat x%u", static_cast<unsigned>(s.repeatCount));
            snprintf(tip, cap, "%u steps", steps);
        }
        return;
    }

    // The duration, or the kind of step when it has none.
    char text[40];
    SDK::Workout::stepDuration(s, mImperial, text, sizeof(text));
    const bool hasDuration = text[0] != '\0';
    snprintf(label, cap, "%s", hasDuration ? text : WorkoutUi::kindLabel(static_cast<uint8_t>(s.intensity)));

    // The target, else the kind of step, else Open.
    SDK::Workout::stepTarget(s, mImperial, text, sizeof(text));
    if (text[0] != '\0') {
        snprintf(tip, cap, "%s", text);
    } else if (hasDuration) {
        snprintf(tip, cap, "%s", WorkoutUi::kindLabel(static_cast<uint8_t>(s.intensity)));
    } else {
        snprintf(tip, cap, "Open");
    }
}

void WorkoutDetailsScreen::back()
{
    ScreenManager::instance().goTo(mModel.isDetailsFromToday() ? ScreenId::WorkoutToday : ScreenId::WorkoutMenu);
}

void WorkoutDetailsScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L2:
            if (mFirstPage) {
                if (mMenu) {
                    mMenu->select(0);
                    showFirstPage(false);
                }
            } else if (mMenu) {
                mMenu->next();  // stops at the last step
            }
            break;
        case Btn::L1:
            if (!mFirstPage && mMenu) {
                if (mMenu->selected() == 0) {
                    showFirstPage(true);
                } else {
                    mMenu->prev();
                }
            }
            break;
        case Btn::R2:
            back();
            break;
        default:
            break;
    }
}
