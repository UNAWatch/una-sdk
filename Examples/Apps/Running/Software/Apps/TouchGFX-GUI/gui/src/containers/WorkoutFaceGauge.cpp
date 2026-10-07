#include <gui/containers/WorkoutFaceGauge.hpp>

#include <SDK/GUI/Color.hpp>
#include <SDK/Utils/Utils.hpp>
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include <cmath>
#include <cstdio>

namespace
{

// The arc spans 80 degrees across the top (0 = 12 o'clock, clockwise), in
// thirds with a small gap between them, clear of the large value below.
constexpr float   kArcFrom   = 320.0f;
constexpr float   kArcSpan   = 80.0f;
constexpr float   kArcGap    = 2.0f;
constexpr int16_t kArcRadius = 109;
constexpr int16_t kDotRadius = 6;
constexpr int16_t kDotTrack  = 95;   ///< Radius the dot runs along, inside the arc

constexpr int16_t kValueY      = 92;  ///< With the arc
constexpr int16_t kValueYNoArc = 84;


void formatPace(float secPerUnit, char* buf, size_t cap)
{
    if (secPerUnit < App::Display::kMinPace) {
        std::snprintf(buf, cap, "---");
        return;
    }
    const auto hms = SDK::Utils::toHMS(static_cast<std::time_t>(secPerUnit + 0.5f));
    if (hms.h > 0) {
        std::snprintf(buf, cap, "%u:%02u", static_cast<unsigned>(hms.h), static_cast<unsigned>(hms.m));
    } else {
        std::snprintf(buf, cap, "%u:%02u", static_cast<unsigned>(hms.m), static_cast<unsigned>(hms.s));
    }
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

void setupArc(touchgfx::Circle& c, touchgfx::PainterABGR2222& p, float from, float to)
{
    c.setPosition(0, 0, 240, 240);
    c.setCenter(120, 120);
    c.setRadius(kArcRadius);
    c.setLineWidth(9);
    c.setArc(from, to);
    c.setCapPrecision(10);
    c.setPainter(p);
}

} // namespace

WorkoutFaceGauge::WorkoutFaceGauge()
{
    setPosition(0, 0, 240, 240);

    const float third = kArcSpan / 3.0f;
    setupArc(mSlow, mSlowPainter, kArcFrom, kArcFrom + third - kArcGap / 2);
    setupArc(mBand, mBandPainter, kArcFrom + third + kArcGap / 2, kArcFrom + 2 * third - kArcGap / 2);
    setupArc(mFast, mFastPainter, kArcFrom + 2 * third + kArcGap / 2, kArcFrom + kArcSpan);
    mSlowPainter.setColor(WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Below)));
    mBandPainter.setColor(WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::In)));
    mFastPainter.setColor(WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Above)));
    add(mSlow);
    add(mBand);
    add(mFast);

    mDot.setPosition(0, 0, 2 * kDotRadius + 2, 2 * kDotRadius + 2);
    mDot.setCenter(kDotRadius + 1, kDotRadius + 1);
    mDot.setRadius(kDotRadius);
    mDot.setLineWidth(0);
    mDot.setArc(0, 360);
    mDotPainter.setColor(SDK::GUI::Color::WHITE);
    mDot.setPainter(mDotPainter);
    add(mDot);

    mDivider.setPosition(40, 127, 160, 3);
    mDivider.setColor(SDK::GUI::Color::TEAL);
    add(mDivider);

    mValue.init(T_TMP_SEMIBOLD_60_L, SDK::GUI::Color::WHITE);
    mUnit.init(T_TMP_MEDIUM_18_L, SDK::GUI::Color::WHITE);
    mSecond.init(T_TMP_SEMIBOLD_40_L, SDK::GUI::Color::WHITE);
    mCaption.init(T_TMP_ITALIC_18_L, SDK::GUI::Color::WHITE);
    mSecond.setAnchor(120, 162);
    mCaption.setAnchor(120, 196);
    add(mValue);
    add(mUnit);
    add(mSecond);
    add(mCaption);
}

void WorkoutFaceGauge::setArcVisible(bool visible)
{
    mSlow.setVisible(visible);
    mBand.setVisible(visible);
    mFast.setVisible(visible);
    mDot.setVisible(visible);
    mSlow.invalidate();
    mBand.invalidate();
    mFast.invalidate();
    mDot.invalidate();
}

void WorkoutFaceGauge::placeDot(float arc)
{
    if (arc < 0.0f) {
        arc = 0.0f;
    } else if (arc > 1.0f) {
        arc = 1.0f;
    }
    // 0 degrees is 12 o'clock, clockwise.
    const float deg = kArcFrom + kArcSpan * arc;
    const float rad = deg * 3.14159265f / 180.0f;
    const int16_t x = static_cast<int16_t>(120.0f + kDotTrack * std::sin(rad));
    const int16_t y = static_cast<int16_t>(120.0f - kDotTrack * std::cos(rad));
    mDot.invalidate();
    mDot.setXY(static_cast<int16_t>(x - kDotRadius - 1), static_cast<int16_t>(y - kDotRadius - 1));
    mDot.invalidate();
}

void WorkoutFaceGauge::formatRemaining(const Model::WorkoutFace& face, bool imperial, char* buf, size_t cap,
                                       touchgfx::TypedTextId& caption)
{
    caption = T_TEXT_LEFT;
    switch (static_cast<SDK::Workout::StepEnd>(face.stepEnd)) {
    case SDK::Workout::StepEnd::Time:
        formatClock(face.remainingMs, true, buf, cap);
        break;
    case SDK::Workout::StepEnd::Distance: {
        const float m = static_cast<float>(face.remainingCm) / 100.0f;
        if (imperial) {
            std::snprintf(buf, cap, "%.2f mi", static_cast<double>(SDK::Utils::kmToMiles(m / 1000.0f)));
        } else if (m < 1000.0f) {
            std::snprintf(buf, cap, "%u m", static_cast<unsigned>(m));
        } else {
            std::snprintf(buf, cap, "%.2f km", static_cast<double>(m / 1000.0f));
        }
    } break;
    default:  // open: how long it has run
        formatClock(face.stepTimeMs, false, buf, cap);
        caption = T_TEXT_ELAPSED_LC;
        break;
    }
}

void WorkoutFaceGauge::set(const Model::WorkoutFace& face, bool imperial, float livePace)
{
    const auto target = static_cast<SDK::Workout::Target>(face.target);
    const bool gauge  = target != SDK::Workout::Target::Open;
    const auto zone   = static_cast<SDK::Workout::Zone>(face.zone);

    char value[12];
    char second[12];
    touchgfx::TypedTextId caption = T_TEXT_LEFT;

    if (gauge) {
        const touchgfx::colortype color = WorkoutUi::zoneColor(face.zone);
        if (target == SDK::Workout::Target::Speed) {
            const float perUnit = imperial ? 1609.344f : 1000.0f;
            formatPace(face.value > 0.05f ? perUnit / face.value : 0.0f, value, sizeof(value));
            mUnit.setText(imperial ? "/mi" : "/km");
        } else if (face.value >= App::Display::kMinHR) {
            std::snprintf(value, sizeof(value), "%u", static_cast<unsigned>(face.value + 0.5f));
            mUnit.setText("bpm");
        } else {
            std::snprintf(value, sizeof(value), "---");
            mUnit.setText("bpm");
        }
        mValue.setColor(color);
        mUnit.setColor(color);
        formatRemaining(face, imperial, second, sizeof(second), caption);
        if (zone == SDK::Workout::Zone::NoSignal && target == SDK::Workout::Target::Speed) {
            caption = T_TEXT_NO_GPS;
        }
        placeDot(face.arc);
        // Without a signal the whole gauge greys out.
        const bool lost = zone == SDK::Workout::Zone::NoSignal;
        mDotPainter.setColor(lost ? SDK::GUI::Color::GRAY : SDK::GUI::Color::WHITE);
        mSlowPainter.setColor(lost ? touchgfx::colortype(SDK::GUI::Color::GRAY_DARK)
                                   : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Below)));
        mBandPainter.setColor(lost ? touchgfx::colortype(SDK::GUI::Color::GRAY)
                                   : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::In)));
        mFastPainter.setColor(lost ? touchgfx::colortype(SDK::GUI::Color::GRAY_DARK)
                                   : WorkoutUi::zoneColor(static_cast<uint8_t>(SDK::Workout::Zone::Above)));
    } else {
        // Without a target, what is left of the step comes first: "132 m"
        // splits into the number and its unit, as the large font has only
        // digits and separators.
        char remaining[12];
        char unit[6];
        formatRemaining(face, imperial, remaining, sizeof(remaining), caption);
        WorkoutUi::splitDuration(remaining, value, sizeof(value), unit, sizeof(unit));
        mValue.setColor(SDK::GUI::Color::WHITE);
        mUnit.setColor(SDK::GUI::Color::WHITE);
        mUnit.setText(unit);
        formatPace(livePace, second, sizeof(second));
        caption = T_TEXT_PACE_LC;
    }
    setArcVisible(gauge);

    // Value and unit side by side, centred together.
    const int16_t vy = gauge ? kValueY : kValueYNoArc;
    mValue.setAnchor(120, vy);
    mValue.setTypography(T_TMP_SEMIBOLD_60_L);
    mValue.setText(value);
    if (mValue.getWidth() + mUnit.getWidth() + 5 > mValue.availableWidth()) {
        // An hour or more left, or a long distance: too wide at 60 px.
        mValue.setTypography(T_TMP_SEMIBOLD_40_L);
    }
    const int16_t vw = mValue.getWidth();
    const int16_t uw = mUnit.getWidth();
    const int16_t gap = uw > 0 ? 5 : 0;
    const int16_t x0 = static_cast<int16_t>(120 - (vw + gap + uw) / 2);
    mValue.setAnchor(static_cast<int16_t>(x0 + vw / 2), vy);
    mUnit.setAnchor(static_cast<int16_t>(x0 + vw + gap + uw / 2), static_cast<int16_t>(vy + 13));

    // The lead-in: the step ends within about 5 s.
    const bool leadIn = face.leadIn && face.stepEnd != static_cast<uint8_t>(SDK::Workout::StepEnd::Open);
    if (!gauge && leadIn) {
        mValue.setColor(SDK::GUI::Color::YELLOW_DARK);
        mUnit.setColor(SDK::GUI::Color::YELLOW_DARK);
        mValue.invalidate();
        mUnit.invalidate();
    }
    mSecond.setColor(gauge && leadIn ? SDK::GUI::Color::YELLOW_DARK : SDK::GUI::Color::WHITE);
    mSecond.setText(second);

    mCaption.setColor(SDK::GUI::Color::WHITE);
    mCaption.setText(caption);
    if (!gauge) {
        // "pace /km": the caption names the unit of the pace above it.
        touchgfx::Unicode::UnicodeChar* buf = mCaption.buffer();
        const uint16_t len = touchgfx::Unicode::strlen(buf);
        touchgfx::Unicode::snprintf(buf + len, static_cast<uint16_t>(16 - len), imperial ? " /mi" : " /km");
        mCaption.place();
    }
}
