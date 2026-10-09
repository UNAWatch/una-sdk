/**
 ******************************************************************************
 * @file    WorkoutFaces.cpp
 * @brief   The workout faces of the Track screen and the step card.
 ******************************************************************************
 */

#include "gui/widgets/WorkoutFaces.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "SDK/GUI/Color.hpp"
#include "SDK/Utils/Utils.hpp"
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include "gui/Format.hpp"
#include "gui/WorkoutUi.hpp"

namespace Color = SDK::GUI::Color;
using F = Theme::Font;

// ---------------------------------------------------------------------------
// Gauge
// ---------------------------------------------------------------------------

namespace
{

// The arc spans 80 degrees across the top (0 = 12 o'clock, clockwise), in
// thirds with a small gap between them, clear of the large value below. Its
// line covers 100 to 109 px from the centre.
constexpr float   kArcFrom   = 320.0f;
constexpr float   kArcSpan   = 80.0f;
constexpr float   kArcGap    = 2.0f;
constexpr int32_t kArcRadius = 104;
constexpr int32_t kArcWidth  = 9;

// The pointer: a triangle inside the arc, its tip towards it.
constexpr float   kPointerTip  = 97.0f;  ///< Radius of the tip
constexpr float   kPointerBase = 85.0f;  ///< Radius of the base
constexpr float   kPointerHalf = 6.0f;   ///< Half the base's width
constexpr int32_t kPointerBox  = 26;     ///< Square the host takes, around the base-to-tip midpoint

constexpr int32_t kValueY      = 92;  ///< With the arc
constexpr int32_t kValueYNoArc = 84;

int32_t roundDeg(float deg)
{
    return static_cast<int32_t>(std::lround(deg));
}

/// Draw the pointer: the host's only content is one filled triangle.
void drawPointerCb(lv_event_t* e)
{
    auto* obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* ptr = static_cast<const WorkoutFaceGauge::Pointer*>(lv_event_get_user_data(e));

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.color = ptr->color;
    dsc.opa   = LV_OPA_COVER;
    for (int i = 0; i < 3; ++i) {
        dsc.p[i].x = coords.x1 + ptr->p[i].x;
        dsc.p[i].y = coords.y1 + ptr->p[i].y;
    }
    lv_draw_triangle(lv_event_get_layer(e), &dsc);
}

void formatClock(uint32_t ms, bool roundUp, char* buf, size_t cap)
{
    const uint32_t s = roundUp ? (ms + 999u) / 1000u : ms / 1000u;
    const auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(s));
    if (hms.h > 0) {
        std::snprintf(buf, cap, "%u:%02u:%02u", static_cast<unsigned>(hms.h),
                      static_cast<unsigned>(hms.m), static_cast<unsigned>(hms.s));
    } else {
        std::snprintf(buf, cap, "%u:%02u", static_cast<unsigned>(hms.m), static_cast<unsigned>(hms.s));
    }
}

} // namespace

WorkoutFaceGauge::WorkoutFaceGauge(lv_obj_t* parent)
{
    mRoot = Theme::container(parent, 0, 0, 240, 240);

    const float third = kArcSpan / 3.0f;
    mSlow = Theme::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kArcRadius, kArcWidth,
                       roundDeg(kArcFrom), roundDeg(kArcFrom + third - kArcGap / 2),
                       WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Below)), false);
    mBand = Theme::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kArcRadius, kArcWidth,
                       roundDeg(kArcFrom + third + kArcGap / 2), roundDeg(kArcFrom + 2 * third - kArcGap / 2),
                       WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::In)), false);
    mFast = Theme::arc(mRoot, SDK::LVGL::kCx, SDK::LVGL::kCy, kArcRadius, kArcWidth,
                       roundDeg(kArcFrom + 2 * third + kArcGap / 2), roundDeg(kArcFrom + kArcSpan),
                       WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Above)), false);

    mPointer = Theme::container(mRoot, 0, 0, kPointerBox, kPointerBox);
    lv_obj_add_event_cb(mPointer, drawPointerCb, LV_EVENT_DRAW_MAIN, &mPointerDsc);
    mPointerDsc.color = Theme::rgb(Color::WHITE);

    Theme::box(mRoot, 40, 127, 160, 3, Color::TEAL);

    mValue.init(mRoot, F::SemiBold60, Color::WHITE);
    mUnit.init(mRoot, F::Medium18, Color::WHITE);
    mSecond.init(mRoot, F::SemiBold40, Color::WHITE, 120, 162);
    mCaption.init(mRoot, F::Italic18, Color::WHITE, 120, 196);
}

void WorkoutFaceGauge::setArcVisible(bool visible)
{
    Theme::setHidden(mSlow, !visible);
    Theme::setHidden(mBand, !visible);
    Theme::setHidden(mFast, !visible);
    Theme::setHidden(mPointer, !visible);
}

void WorkoutFaceGauge::placePointer(float arc)
{
    if (arc < 0.0f) {
        arc = 0.0f;
    } else if (arc > 1.0f) {
        arc = 1.0f;
    }
    // 0 degrees is 12 o'clock, clockwise: outwards is (sin, -cos) and along
    // the arc (cos, sin).
    const float deg = kArcFrom + kArcSpan * arc;
    const float rad = deg * 3.14159265f / 180.0f;
    const float nx  = std::sin(rad);
    const float ny  = -std::cos(rad);
    const float mid = (kPointerTip + kPointerBase) / 2.0f;
    const int32_t bx = static_cast<int32_t>(120.0f + mid * nx) - kPointerBox / 2;
    const int32_t by = static_cast<int32_t>(120.0f + mid * ny) - kPointerBox / 2;

    // Corners in the host's own frame.
    const float ox = 120.0f - static_cast<float>(bx);
    const float oy = 120.0f - static_cast<float>(by);
    const auto pt = [](float x, float y) {
        return lv_point_precise_t{ static_cast<lv_value_precise_t>(std::lround(x)),
                                   static_cast<lv_value_precise_t>(std::lround(y)) };
    };
    mPointerDsc.p[0] = pt(ox + kPointerTip * nx, oy + kPointerTip * ny);
    mPointerDsc.p[1] = pt(ox + kPointerBase * nx - kPointerHalf * ny, oy + kPointerBase * ny + kPointerHalf * nx);
    mPointerDsc.p[2] = pt(ox + kPointerBase * nx + kPointerHalf * ny, oy + kPointerBase * ny - kPointerHalf * nx);
    lv_obj_set_pos(mPointer, bx, by);
    lv_obj_invalidate(mPointer);
}

void WorkoutFaceGauge::formatRemaining(const Model::WorkoutFace& face, bool imperial, char* buf, size_t cap,
                                       const char*& caption)
{
    caption = "left";
    switch (static_cast<SDK::Workout::StepEnd>(face.stepEnd)) {
    case SDK::Workout::StepEnd::Time:
        formatClock(face.remainingMs, true, buf, cap);
        break;
    case SDK::Workout::StepEnd::Distance: {
        const float m = static_cast<float>(face.remainingCm) / 100.0f;
        char number[12];
        if (imperial) {
            Fmt::fixed(number, sizeof(number), SDK::Utils::kmToMiles(m / 1000.0f), 2);
            std::snprintf(buf, cap, "%s mi", number);
        } else if (m < 1000.0f) {
            std::snprintf(buf, cap, "%u m", static_cast<unsigned>(m));
        } else {
            Fmt::fixed(number, sizeof(number), m / 1000.0f, 2);
            std::snprintf(buf, cap, "%s km", number);
        }
    } break;
    default:  // open: how long it has run
        formatClock(face.stepTimeMs, false, buf, cap);
        caption = "elapsed";
        break;
    }
}

void WorkoutFaceGauge::set(const Model::WorkoutFace& face, bool imperial, float livePace)
{
    const auto target = static_cast<SDK::Workout::Target>(face.target);
    const bool gauge  = target != SDK::Workout::Target::Open;
    const auto zone   = static_cast<SDK::Workout::Zone>(face.zone);

    char value[16];
    char second[16];
    const char* caption = "left";

    if (gauge) {
        const uint32_t color = WorkoutUi::zoneColor(face.zone);
        if (target == SDK::Workout::Target::Speed) {
            // The service floors the speed: 0 means no pace.
            const float perUnit = imperial ? 1609.344f : 1000.0f;
            Fmt::pace(value, sizeof(value), face.value > 0.0f ? perUnit / face.value : 0.0f);
            mUnit.setText(imperial ? "/mi" : "/km");
        } else {
            Fmt::heartRate(value, sizeof(value), face.value);
            mUnit.setText("bpm");
        }
        mValue.setColor(color);
        mUnit.setColor(color);
        formatRemaining(face, imperial, second, sizeof(second), caption);
        if (zone == SDK::Workout::Zone::NoSignal && target == SDK::Workout::Target::Speed) {
            caption = "no GPS";
        }
        placePointer(face.arc);
        // Without a signal the whole gauge greys out.
        const bool lost = zone == SDK::Workout::Zone::NoSignal;
        mPointerDsc.color = Theme::rgb(lost ? Color::GRAY : Color::WHITE);
        Theme::setArcColor(mSlow, lost ? Color::GRAY_DARK
                                       : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Below)));
        Theme::setArcColor(mBand, lost ? Color::GRAY
                                       : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::In)));
        Theme::setArcColor(mFast, lost ? Color::GRAY_DARK
                                       : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Above)));
    } else {
        // Without a target, what is left of the step comes first: "132 m"
        // splits into the number and its unit, as the large font has only
        // digits and separators.
        char remaining[16];
        char unit[8];
        formatRemaining(face, imperial, remaining, sizeof(remaining), caption);
        WorkoutUi::splitDuration(remaining, value, sizeof(value), unit, sizeof(unit));
        mValue.setColor(Color::WHITE);
        mUnit.setColor(Color::WHITE);
        mUnit.setText(unit);
        Fmt::pace(second, sizeof(second), livePace);
        caption = "pace";
    }
    setArcVisible(gauge);

    // Value and unit side by side, centred together.
    const int32_t vy = gauge ? kValueY : kValueYNoArc;
    mValue.setFont(F::SemiBold60);
    mValue.setText(value);
    mValue.setAnchor(120, vy);
    if (mValue.width() + mUnit.width() + 5 > mValue.availableWidth()) {
        // An hour or more left, or a long distance: too wide at 60 px.
        mValue.setFont(F::SemiBold40);
    }
    const int32_t vw  = mValue.width();
    const int32_t uw  = mUnit.width();
    const int32_t gap = uw > 0 ? 5 : 0;
    const int32_t x0  = 120 - (vw + gap + uw) / 2;
    mValue.setAnchor(x0 + vw / 2, vy);
    mUnit.setAnchor(x0 + vw + gap + uw / 2, vy + 13);

    // The lead-in: the step ends within about 5 s.
    const bool leadIn = face.leadIn && face.stepEnd != static_cast<uint8_t>(SDK::Workout::StepEnd::Open);
    if (!gauge && leadIn) {
        mValue.setColor(Color::YELLOW_DARK);
        mUnit.setColor(Color::YELLOW_DARK);
    }
    mSecond.setColor(gauge && leadIn ? Color::YELLOW_DARK : Color::WHITE);
    mSecond.setText(second);

    // "pace /km": without a target the caption names the unit of the pace above it.
    char text[24];
    std::snprintf(text, sizeof(text), "%s%s", caption, gauge ? "" : imperial ? " /mi" : " /km");
    mCaption.setColor(Color::WHITE);
    mCaption.setText(text);
}

// ---------------------------------------------------------------------------
// Current step
// ---------------------------------------------------------------------------

namespace
{
constexpr int32_t kLeftX    = 74;
constexpr int32_t kRightX   = 166;
constexpr int32_t kCaptionY = 142;
constexpr int32_t kStepValueY = 170;
} // namespace

WorkoutFaceStep::WorkoutFaceStep(lv_obj_t* parent)
{
    mRoot  = Theme::container(parent, 0, 0, 240, 240);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "RUN");

    mTargetCaption.init(mRoot, F::Italic18, Color::WHITE, 120, 62);
    mTarget.init(mRoot, F::SemiBold35, Color::WHITE, 120, 92);

    Theme::box(mRoot, 30, 119, 180, 3, Color::TEAL);
    mColumns = Theme::box(mRoot, 119, 129, 3, 58, Color::TEAL);

    mAvgCaption.init(mRoot, F::Italic18, Color::WHITE);
    mAvg.init(mRoot, F::SemiBold30, Color::WHITE);
    mRepCaption.init(mRoot, F::Italic18, Color::WHITE);
    mRep.init(mRoot, F::SemiBold30, Color::WHITE);
    mAvgCaption.setText("Step avg");
    mRepCaption.setText("Rep");

    setRepeat(0, 0);
}

void WorkoutFaceStep::setStep(const Model::WorkoutStepInfo& step)
{
    mTitle->setText(WorkoutUi::kindTitle(step.intensity));
    mTitle->setColor(WorkoutUi::kindColor(step.intensity));

    // "3:45-4:05 /km": the band large, its unit in the caption above.
    const char* target = step.target;
    const char* space  = std::strrchr(target, ' ');
    const bool  unit   = space != nullptr
        && (std::strcmp(space + 1, "/km") == 0 || std::strcmp(space + 1, "/mi") == 0
            || std::strcmp(space + 1, "bpm") == 0);

    char caption[24];
    std::snprintf(caption, sizeof(caption), "Target%s%s", unit ? " " : "", unit ? space + 1 : "");
    mTargetCaption.setText(caption);

    if (target[0] == '\0') {
        mTarget.setText("No target");
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
    const bool hasRepeat = rep > 0;
    char text[24];
    if (reps > 0) {
        std::snprintf(text, sizeof(text), "%lu/%lu", static_cast<unsigned long>(rep),
                      static_cast<unsigned long>(reps));
    } else {
        std::snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(rep));
    }
    mRep.setText(text);

    // Outside a repeat the average has the row to itself.
    const int32_t avgX = hasRepeat ? kLeftX : 120;
    mAvgCaption.setAnchor(avgX, kCaptionY);
    mAvg.setAnchor(avgX, kStepValueY);
    mRepCaption.setAnchor(kRightX, kCaptionY);
    mRep.setAnchor(kRightX, kStepValueY);
    Theme::setHidden(mColumns, !hasRepeat);
    mRepCaption.setVisible(hasRepeat);
    mRep.setVisible(hasRepeat);
}

void WorkoutFaceStep::setAverages(float lapPace, float lapHr, bool heartRate)
{
    char text[12];
    if (heartRate) {
        Fmt::heartRate(text, sizeof(text), lapHr);
    } else {
        Fmt::pace(text, sizeof(text), lapPace);
    }
    mAvg.setText(text);
}

// ---------------------------------------------------------------------------
// Step card
// ---------------------------------------------------------------------------

namespace
{
constexpr int32_t kCardValueY = 82;
constexpr int32_t kTargetY    = 128;
constexpr int32_t kSubY       = 156;
constexpr int32_t kNotes1Y    = 182;
constexpr int32_t kNotes2Y    = 204;
} // namespace

WorkoutStepCard::WorkoutStepCard(lv_obj_t* parent)
{
    mRoot  = Theme::container(parent, 0, 0, 240, 240);
    mTitle = std::make_unique<Widgets::Title>(mRoot, "");

    mValue.init(mRoot, F::SemiBold60, Color::WHITE);
    mUnit.init(mRoot, F::Medium25, Color::WHITE);
    mTarget.init(mRoot, F::SemiBold30, Color::WHITE, 120, kTargetY);
    mSub.init(mRoot, F::Italic18, Color::GRAY, 120, kSubY);
    mNotes1.init(mRoot, F::Medium18, Color::WHITE, 120, kNotes1Y);
    mNotes2.init(mRoot, F::Medium18, Color::WHITE, 120, kNotes2Y);
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
        mTitle->setText("NEXT");
        mTitle->setColor(Color::WHITE);
    } else {
        mTitle->setText(WorkoutUi::kindTitle(step.intensity));
        mTitle->setColor(WorkoutUi::kindColor(step.intensity));
    }

    mTarget.setFont(F::SemiBold30);
    if (step.none) {
        // Nothing follows: the run carries on as a free run.
        mValue.setText("");
        mUnit.setText("");
        mTarget.setAnchor(120, 100);
        mTarget.setText("Last step");
        mSub.setAnchor(120, 132);
        mSub.setText("Then a free run");
        mNotes1.setText("");
        mNotes2.setText("");
        return;
    }
    mTarget.setAnchor(120, kTargetY);
    mSub.setAnchor(120, kSubY);

    // The duration large, its unit beside it in a smaller size: the large
    // font has only digits, separators and the word Open.
    const uint32_t color = WorkoutUi::kindColor(step.intensity);
    char number[16];
    char unit[8];
    WorkoutUi::splitDuration(step.duration, number, sizeof(number), unit, sizeof(unit));
    mValue.setText(number[0] == '\0' ? "Open" : number);
    mUnit.setText(unit);
    mValue.setColor(color);
    mUnit.setColor(color);
    const int32_t vw  = mValue.width();
    const int32_t uw  = mUnit.width();
    const int32_t gap = uw > 0 ? 6 : 0;
    const int32_t x0  = 120 - (vw + gap + uw) / 2;
    mValue.setAnchor(x0 + vw / 2, kCardValueY);
    mUnit.setAnchor(x0 + vw + gap + uw / 2, kCardValueY + 13);

    // The target at 30 px, or 25 px when that is too wide for the display.
    if (step.target[0] != '\0') {
        mTarget.setFileText(step.target);
    } else {
        mTarget.setText("No target");
    }
    if (mTarget.width() > mTarget.availableWidth()) {
        mTarget.setFont(F::SemiBold25);
    }
    mTarget.fit();

    char sub[32];
    if (!next && step.rep > 0) {
        if (step.reps > 0) {
            std::snprintf(sub, sizeof(sub), "Rep %u/%u", static_cast<unsigned>(step.rep),
                          static_cast<unsigned>(step.reps));
        } else {
            std::snprintf(sub, sizeof(sub), "Rep %u", static_cast<unsigned>(step.rep));
        }
        mSub.setText(sub);
    } else {
        mSub.setText(WorkoutUi::kindLabel(step.intensity));
    }

    // The notes over two lines, each as wide as the display allows there.
    char text[SDK::Workout::kStepNotesBytes];
    char line1[SDK::Workout::kStepNotesBytes];
    char line2[SDK::Workout::kStepNotesBytes];
    WorkoutUi::toText(step.notes, text, sizeof(text));
    WorkoutUi::wrapTwo(text, line1, line2, sizeof(line1), mNotes1.font(),
                       WorkoutUi::lineWidth(kNotes1Y - 11, kNotes1Y + 11),
                       WorkoutUi::lineWidth(kNotes2Y - 11, kNotes2Y + 11));
    mNotes1.setText(line1);
    mNotes2.setText(line2);
}
