#include <gui/containers/WorkoutStepCard.hpp>

#include <SDK/GUI/Color.hpp>

#include <cstdio>

namespace
{
constexpr int16_t kValueY  = 82;
constexpr int16_t kTargetY = 128;
constexpr int16_t kSubY    = 156;
constexpr int16_t kNotes1Y = 182;
constexpr int16_t kNotes2Y = 204;
} // namespace

WorkoutStepCard::WorkoutStepCard()
{
    setPosition(0, 0, 240, 240);

    mTitle.setXY(50, 0);
    mTitle.initialize();
    add(mTitle);

    mValue.init(T_TMP_SEMIBOLD_60_L, SDK::GUI::Color::WHITE);
    mUnit.init(T_TMP_MEDIUM_25_L, SDK::GUI::Color::WHITE);
    mTarget.init(T_TMP_SEMIBOLD_30_L, SDK::GUI::Color::WHITE);
    mSub.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::GRAY);
    mNotes1.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mNotes2.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mTarget.setAnchor(120, kTargetY);
    mSub.setAnchor(120, kSubY);
    mNotes1.setAnchor(120, kNotes1Y);
    mNotes2.setAnchor(120, kNotes2Y);
    add(mValue);
    add(mUnit);
    add(mTarget);
    add(mSub);
    add(mNotes1);
    add(mNotes2);
}

void WorkoutStepCard::showCurrent(const Model::WorkoutStepInfo& step)
{
    show(step, false);
}

void WorkoutStepCard::showNext(const Model::WorkoutStepInfo& step)
{
    show(step, true);
}

void WorkoutStepCard::show(const Model::WorkoutStepInfo& step, bool next)
{
    if (next) {
        mTitle.set(T_TEXT_NEXT_UC);
        mTitle.setColor(SDK::GUI::Color::WHITE);
    } else {
        mTitle.set(WorkoutUi::kindTitle(step.intensity));
        mTitle.setColor(WorkoutUi::kindColor(step.intensity));
    }
    mTitle.invalidate();

    if (step.none) {
        // Nothing follows: the run carries on as a free run.
        mValue.setText("");
        mUnit.setText("");
        mTarget.setAnchor(120, 100);
        mTarget.setText(T_TEXT_LAST_STEP);
        mSub.setAnchor(120, 132);
        mSub.setText(T_TEXT_THEN_FREE_RUN);
        mNotes1.setText("");
        mNotes2.setText("");
        return;
    }
    mTarget.setAnchor(120, kTargetY);
    mSub.setAnchor(120, kSubY);

    // The duration large, its unit beside it in a smaller size: the large
    // font has only digits, separators and the word Open.
    const touchgfx::colortype color = WorkoutUi::kindColor(step.intensity);
    char number[16];
    char unit[8];
    WorkoutUi::splitDuration(step.duration, number, sizeof(number), unit, sizeof(unit));
    if (number[0] == '\0') {
        mValue.setText(T_TEXT_OPEN);
    } else {
        mValue.setText(number);
    }
    mUnit.setText(unit);
    mValue.setColor(color);
    mUnit.setColor(color);
    const int16_t vw  = mValue.getWidth();
    const int16_t uw  = mUnit.getWidth();
    const int16_t gap = uw > 0 ? 6 : 0;
    const int16_t x0  = static_cast<int16_t>(120 - (vw + gap + uw) / 2);
    mValue.setAnchor(static_cast<int16_t>(x0 + vw / 2), kValueY);
    mUnit.setAnchor(static_cast<int16_t>(x0 + vw + gap + uw / 2), static_cast<int16_t>(kValueY + 13));

    // The target at 30 px, or 25 px when that is too wide for the display.
    mTarget.setTypography(T_TMP_SEMIBOLD_30_L);
    if (step.target[0] != '\0') {
        mTarget.setFileText(step.target);
    } else {
        mTarget.setText(T_TEXT_NO_TARGET);
    }
    if (mTarget.getWidth() > mTarget.availableWidth()) {
        mTarget.setTypography(T_TMP_SEMIBOLD_25_L);
    }
    mTarget.fit();

    if (!next && step.rep > 0) {
        // Unicode::snprintf's %s takes UnicodeChar text, as TypedText gives it.
        const touchgfx::Unicode::UnicodeChar* word = touchgfx::TypedText(T_TEXT_REP).getText();
        if (step.reps > 0) {
            touchgfx::Unicode::snprintf(mSub.buffer(), 24, "%s %u/%u", word,
                                        static_cast<unsigned>(step.rep), static_cast<unsigned>(step.reps));
        } else {
            touchgfx::Unicode::snprintf(mSub.buffer(), 24, "%s %u", word, static_cast<unsigned>(step.rep));
        }
        mSub.place();
    } else {
        mSub.setText(WorkoutUi::kindLabel(step.intensity));
    }

    // The notes over two lines, each as wide as the display allows there.
    touchgfx::Unicode::UnicodeChar text[SDK::Workout::kStepNotesBytes];
    WorkoutUi::toText(step.notes, text, SDK::Workout::kStepNotesBytes);
    WorkoutUi::wrapTwo(text, mNotes1.buffer(), mNotes2.buffer(), SDK::Workout::kStepNotesBytes,
                       T_TMP_MEDIUM_18_L, WorkoutUi::lineWidth(kNotes1Y - 11, kNotes1Y + 11),
                       WorkoutUi::lineWidth(kNotes2Y - 11, kNotes2Y + 11));
    mNotes1.place();
    mNotes2.place();
}
