#include <gui/workoutdetails_screen/WorkoutDetailsView.hpp>
#include <gui/common/WorkoutUi.hpp>
#include "SDK/Workout/StepLabel.hpp"

#include <cstdio>
#include <cstring>

namespace
{
constexpr int16_t kName1Y = 66;
constexpr int16_t kName2Y = 94;
} // namespace

WorkoutDetailsView::WorkoutDetailsView() :
    mUpdateItemCb(this, &WorkoutDetailsView::updateItem),
    mUpdateCenterItemCb(this, &WorkoutDetailsView::updateCenterItem),
    mAnimationMiddleCb(this, &WorkoutDetailsView::onAnimationMiddle)
{
}

void WorkoutDetailsView::setupScreen()
{
    WorkoutDetailsViewBase::setupScreen();

    menuLayout.setAnimationSteps(App::Config::kMenuAnimationSteps);
    menuLayout.setUpdateItemCallback(mUpdateItemCb);
    menuLayout.setUpdateCenterItemCallback(mUpdateCenterItemCb);
    menuLayout.setAnimationMiddleCallback(mAnimationMiddleCb);
    menuLayout.getButtons().setR1(Buttons::NONE);   // view only
    menuLayout.getButtons().setR2(Buttons::WHITE);

    mName1.init(T_TMP_SEMIBOLD_25_L, SDK::GUI::Color::WHITE);
    mName2.init(T_TMP_SEMIBOLD_25_L, SDK::GUI::Color::WHITE);
    mTotals.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::GRAY);
    mDesc1.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mDesc2.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mDesc3.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mDivider.setColor(SDK::GUI::Color::TEAL);
    add(mName1);
    add(mName2);
    add(mTotals);
    add(mDivider);
    add(mDesc1);
    add(mDesc2);
    add(mDesc3);

    showFirstPage(true);
}

void WorkoutDetailsView::tearDownScreen()
{
    WorkoutDetailsViewBase::tearDownScreen();
}

void WorkoutDetailsView::setEntry(const SDK::Workout::Library::Entry& entry, bool imperial)
{
    mImperial = imperial;

    // The name over up to two lines, then the totals under the last one.
    touchgfx::Unicode::UnicodeChar name[64];
    WorkoutUi::toText(entry.name, name, 64);
    WorkoutUi::wrapTwo(name, mName1.buffer(), mName2.buffer(), 32, T_TMP_SEMIBOLD_25_L,
                       WorkoutUi::lineWidth(kName1Y - 15, kName1Y + 15),
                       WorkoutUi::lineWidth(kName2Y - 15, kName2Y + 15));
    const bool twoLines = mName2.buffer()[0] != 0;
    mName1.setAnchor(120, kName1Y);
    mName2.setAnchor(120, kName2Y);

    const int16_t y = twoLines ? kName2Y : kName1Y;
    char totals[32];
    WorkoutUi::formatTotals(entry, imperial, totals, sizeof(totals));
    mTotals.setAnchor(120, static_cast<int16_t>(y + 26));
    mTotals.setText(totals);
    mDivider.setPosition(40, static_cast<int16_t>(y + 45), 160, 3);
    mDescY = static_cast<int16_t>(y + 70);
    mDesc1.setAnchor(120, static_cast<int16_t>(y + 70));
    mDesc2.setAnchor(120, static_cast<int16_t>(y + 94));
    mDesc3.setAnchor(120, static_cast<int16_t>(y + 118));

    char title[16];
    touchgfx::Unicode::toUTF8(touchgfx::TypedText(T_TEXT_DETAILS_UC).getText(),
                              reinterpret_cast<uint8_t*>(title), sizeof(title));
    menuLayout.setTitle(title);
}

void WorkoutDetailsView::setProgram(const SDK::Workout::Program* program, bool imperial)
{
    mProgram  = program;
    mImperial = imperial;
    mRowCount = 0;
    if (mProgram == nullptr) {
        return;
    }

    // The description, word-wrapped over three lines and cut after the last:
    // no more of it than three lines can hold.
    constexpr uint16_t kShown = 160;
    touchgfx::Unicode::UnicodeChar text[kShown];
    WorkoutUi::toText(mProgram->description, text, kShown);
    WorkoutLabel<48>* lines[] = { &mDesc1, &mDesc2, &mDesc3 };
    const touchgfx::Unicode::UnicodeChar* rest = text;
    for (int k = 0; k < 3; ++k) {
        WorkoutLabel<48>* line = lines[k];
        touchgfx::Unicode::UnicodeChar after[48];
        const int16_t cy = static_cast<int16_t>(mDescY + 24 * k);
        const int16_t w  = WorkoutUi::lineWidth(static_cast<int16_t>(cy - 11), static_cast<int16_t>(cy + 11));
        while (*rest == ' ') {
            ++rest;
        }
        if (k == 2) {
            touchgfx::Unicode::strncpy(line->buffer(), rest, 47);
            line->buffer()[47] = 0;
            WorkoutUi::fitWidth(line->buffer(), T_TMP_MEDIUM_18_L, w);
        } else {
            // wrapTwo lays the first line; what is left carries on below.
            WorkoutUi::wrapTwo(rest, line->buffer(), after, 48, T_TMP_MEDIUM_18_L, w, 32767);
            rest += touchgfx::Unicode::strlen(line->buffer());
            while (*rest == ' ') {
                ++rest;
            }
        }
        line->place();
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
    if (mRowCount > 0) {
        menuLayout.setNumberOfItems(static_cast<int16_t>(mRowCount));
        menuLayout.selectItem(0);
    }
    menuLayout.invalidate();
}

void WorkoutDetailsView::showFirstPage(bool show)
{
    mFirstPage = show;
    menuLayout.getMenu().setVisible(!show);
    menuLayout.showBackground(!show);
    mName1.setVisible(show);
    mName2.setVisible(show);
    mTotals.setVisible(show);
    mDivider.setVisible(show);
    mDesc1.setVisible(show);
    mDesc2.setVisible(show);
    mDesc3.setVisible(show);
    if (show) {
        menuLayout.setInfoText(nullptr);
        char title[16];
        touchgfx::Unicode::toUTF8(touchgfx::TypedText(T_TEXT_DETAILS_UC).getText(),
                                  reinterpret_cast<uint8_t*>(title), sizeof(title));
        menuLayout.setTitle(title);
    } else {
        showRow(static_cast<int16_t>(menuLayout.getSelectedItem()));
    }
    invalidate();
}

void WorkoutDetailsView::showRow(int16_t row)
{
    if (mProgram == nullptr || row < 0 || row >= mRowCount) {
        return;
    }
    std::snprintf(mTitle, sizeof(mTitle), "%d/%u", row + 1, static_cast<unsigned>(mRowCount));
    menuLayout.setTitle(mTitle);

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
            len += static_cast<size_t>(std::snprintf(context + len, sizeof(context) - len, "%sopen",
                                                     len ? " in " : r.repeat ? "in " : ""));
        } else {
            len += static_cast<size_t>(std::snprintf(context + len, sizeof(context) - len, "%sx%lu",
                                                     len ? " in " : r.repeat ? "in " : "",
                                                     static_cast<unsigned long>(o.repeatCount)));
        }
    }
    if (!r.repeat && s.notes[0] != '\0') {
        WorkoutUi::toText(s.notes, mInfo, sizeof(mInfo) / sizeof(mInfo[0]));
    } else {
        touchgfx::Unicode::strncpy(mInfo, context, sizeof(mInfo) / sizeof(mInfo[0]) - 1);
        mInfo[sizeof(mInfo) / sizeof(mInfo[0]) - 1] = 0;
    }
    WorkoutUi::fitWidth(mInfo, T_TMP_ITALIC_18, 170);
    menuLayout.setInfoMsgColor(SDK::GUI::Color::TEAL);
    menuLayout.setInfoText(mInfo[0] != 0 ? mInfo : nullptr);
}

void WorkoutDetailsView::rowText(int16_t row, touchgfx::Unicode::UnicodeChar* label,
                                 touchgfx::Unicode::UnicodeChar* tip, uint16_t cap) const
{
    label[0] = 0;
    tip[0]   = 0;
    if (mProgram == nullptr || row < 0 || row >= mRowCount) {
        return;
    }
    const Row& r = mRows[row];
    const SDK::Workout::Step& s = mProgram->steps[r.step];
    char text[40];

    if (r.repeat) {
        // "Repeat x5", and how many steps it repeats.
        const touchgfx::Unicode::UnicodeChar* word = touchgfx::TypedText(T_TEXT_REPEAT).getText();
        if (s.repeatCount == SDK::Workout::kRepeatForever) {
            touchgfx::Unicode::snprintf(label, cap, "%s", word);
            touchgfx::Unicode::snprintf(tip, cap, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
        } else {
            // The steps it repeats, not counting the repeats inside it.
            unsigned steps = 0;
            for (uint16_t k = s.repeatFrom; k < r.step; ++k) {
                steps += mProgram->steps[k].end != SDK::Workout::StepEnd::Repeat ? 1u : 0u;
            }
            touchgfx::Unicode::snprintf(label, cap, "%s x%u", word, static_cast<unsigned>(s.repeatCount));
            touchgfx::Unicode::snprintf(tip, cap, "%u %s", steps, touchgfx::TypedText(T_TEXT_STEPS_LC).getText());
        }
        return;
    }

    // The duration, or the kind of step when it has none.
    SDK::Workout::stepDuration(s, mImperial, text, sizeof(text));
    const bool hasDuration = text[0] != '\0';
    if (hasDuration) {
        touchgfx::Unicode::strncpy(label, text, static_cast<uint16_t>(cap - 1));
        label[cap - 1] = 0;
    } else {
        touchgfx::Unicode::snprintf(label, cap, "%s",
                                    touchgfx::TypedText(WorkoutUi::kindLabel(static_cast<uint8_t>(s.intensity))).getText());
    }

    // The target, else the kind of step, else Open.
    SDK::Workout::stepTarget(s, mImperial, text, sizeof(text));
    if (text[0] != '\0') {
        touchgfx::Unicode::strncpy(tip, text, static_cast<uint16_t>(cap - 1));
        tip[cap - 1] = 0;
    } else if (hasDuration) {
        touchgfx::Unicode::snprintf(tip, cap, "%s",
                                    touchgfx::TypedText(WorkoutUi::kindLabel(static_cast<uint8_t>(s.intensity))).getText());
    } else {
        touchgfx::Unicode::snprintf(tip, cap, "%s", touchgfx::TypedText(T_TEXT_OPEN).getText());
    }
}

void WorkoutDetailsView::updateItem(MainMenuItem& item, int16_t index)
{
    rowText(index, mLabel, mTip, 32);
    MenuItemConfig cfg;
    cfg.style    = MenuItemConfig::TIP;
    cfg.msgText  = mLabel;
    cfg.tipText  = mTip;
    cfg.tipColor = SDK::GUI::Color::TEAL;
    item.apply(cfg);
}

void WorkoutDetailsView::updateCenterItem(MainMenuCenterItem& item, int16_t index)
{
    rowText(index, mLabel, mTip, 32);
    WorkoutUi::fitWidth(mLabel, T_TMP_SEMIBOLD_30, 200);
    WorkoutUi::fitWidth(mTip, T_TMP_ITALIC_18, 200);
    MenuItemConfig cfg;
    cfg.style   = MenuItemConfig::TIP;
    cfg.msgText = mLabel;
    cfg.tipText = mTip;
    item.apply(cfg);
}

void WorkoutDetailsView::onAnimationMiddle(int16_t index)
{
    showRow(index);
}

void WorkoutDetailsView::handleKeyEvent(uint8_t key)
{
    if (key == SDK::GUI::Button::L2) {
        if (mFirstPage) {
            if (mRowCount > 0) {
                menuLayout.selectItem(0);
                showFirstPage(false);
            }
        } else if (menuLayout.getSelectedItem() + 1u < mRowCount) {
            menuLayout.selectNext();
        }
    }
    if (key == SDK::GUI::Button::L1 && !mFirstPage) {
        if (menuLayout.getSelectedItem() == 0) {
            showFirstPage(true);
        } else {
            menuLayout.selectPrev();
        }
    }
    if (key == SDK::GUI::Button::R2) {
        presenter->back();
    }
}
