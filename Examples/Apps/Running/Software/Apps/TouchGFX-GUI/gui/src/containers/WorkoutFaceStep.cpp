#include <gui/containers/WorkoutFaceStep.hpp>

#include <SDK/GUI/Color.hpp>
#include <SDK/Utils/Utils.hpp>

#include <cstdio>
#include <cstring>

namespace
{
constexpr int16_t kLeftX   = 74;
constexpr int16_t kRightX  = 166;
constexpr int16_t kCaptionY = 142;
constexpr int16_t kValueY   = 170;
} // namespace

WorkoutFaceStep::WorkoutFaceStep()
{
    setPosition(0, 0, 240, 240);

    mTitle.setXY(50, 0);
    mTitle.initialize();
    add(mTitle);

    mTargetCaption.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::WHITE);
    mTarget.init(T_TMP_SEMIBOLD_35_L, SDK::GUI::Color::WHITE);
    mTargetCaption.setAnchor(120, 62);
    mTarget.setAnchor(120, 92);
    add(mTargetCaption);
    add(mTarget);

    mDivider.setPosition(30, 119, 180, 3);
    mDivider.setColor(SDK::GUI::Color::TEAL);
    add(mDivider);
    mColumns.setPosition(119, 129, 3, 58);
    mColumns.setColor(SDK::GUI::Color::TEAL);
    add(mColumns);

    mAvgCaption.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::WHITE);
    mAvg.init(T_TMP_SEMIBOLD_30_L, SDK::GUI::Color::WHITE);
    mRepCaption.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::WHITE);
    mRep.init(T_TMP_SEMIBOLD_30_L, SDK::GUI::Color::WHITE);
    mAvgCaption.setText(T_TEXT_STEP_AVG);
    mRepCaption.setText(T_TEXT_REP);
    add(mAvgCaption);
    add(mAvg);
    add(mRepCaption);
    add(mRep);

    setRepeat(0, 0);
}

void WorkoutFaceStep::setStep(const Model::WorkoutStepInfo& step)
{
    mTitle.set(WorkoutUi::kindTitle(step.intensity));
    mTitle.setColor(WorkoutUi::kindColor(step.intensity));
    mTitle.invalidate();

    // "3:45-4:05 /km": the band large, its unit in the caption above.
    const char* target = step.target;
    const char* space  = std::strrchr(target, ' ');
    const bool  unit   = space != nullptr
        && (std::strcmp(space + 1, "/km") == 0 || std::strcmp(space + 1, "/mi") == 0
            || std::strcmp(space + 1, "bpm") == 0);

    touchgfx::Unicode::UnicodeChar* cap = mTargetCaption.buffer();
    touchgfx::Unicode::snprintf(cap, 24, "%s", touchgfx::TypedText(T_TEXT_TARGET).getText());
    if (unit) {
        const uint16_t len = touchgfx::Unicode::strlen(cap);
        if (len + 2 < 24) {
            cap[len] = ' ';
            touchgfx::Unicode::strncpy(cap + len + 1, space + 1, static_cast<uint16_t>(24 - len - 2));
            cap[23] = 0;
        }
    }
    mTargetCaption.place();

    if (target[0] == '\0') {
        mTarget.setText(T_TEXT_NO_TARGET);
    } else if (unit) {
        char band[32];
        std::snprintf(band, sizeof(band), "%.*s", static_cast<int>(space - target), target);
        mTarget.setFileText(band);
    } else {
        mTarget.setFileText(target);
    }
    mTarget.fit();
}

void WorkoutFaceStep::setRepeat(uint32_t rep, uint32_t reps)
{
    mHasRepeat = rep > 0;
    char text[24];
    if (reps > 0) {
        std::snprintf(text, sizeof(text), "%lu/%lu", static_cast<unsigned long>(rep),
                      static_cast<unsigned long>(reps));
    } else {
        std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(rep));
    }
    mRep.setText(text);

    // Outside a repeat the average has the row to itself.
    const int16_t avgX = mHasRepeat ? kLeftX : 120;
    mAvgCaption.setAnchor(avgX, kCaptionY);
    mAvg.setAnchor(avgX, kValueY);
    mRepCaption.setAnchor(kRightX, kCaptionY);
    mRep.setAnchor(kRightX, kValueY);
    mColumns.setVisible(mHasRepeat);
    mRepCaption.setVisible(mHasRepeat);
    mRep.setVisible(mHasRepeat);
    mColumns.invalidate();
    mRepCaption.invalidate();
    mRep.invalidate();
}

void WorkoutFaceStep::setAverages(float lapPace, float lapHr, bool heartRate)
{
    char text[10];
    if (heartRate) {
        if (lapHr < App::Display::kMinHR) {
            std::snprintf(text, sizeof(text), "---");
        } else {
            std::snprintf(text, sizeof(text), "%u", static_cast<unsigned>(lapHr + 0.5f));
        }
    } else if (lapPace < App::Display::kMinPace) {
        std::snprintf(text, sizeof(text), "---");
    } else {
        const auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(lapPace + 0.5f));
        if (hms.h > 0) {
            std::snprintf(text, sizeof(text), "%u:%02u", static_cast<unsigned>(hms.h), static_cast<unsigned>(hms.m));
        } else {
            std::snprintf(text, sizeof(text), "%u:%02u", static_cast<unsigned>(hms.m), static_cast<unsigned>(hms.s));
        }
    }
    mAvg.setText(text);
}
