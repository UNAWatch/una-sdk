/**
 ******************************************************************************
 * @file    Widgets.cpp
 * @brief   The Run app's own widgets, rebuilt from LVGL primitives.
 ******************************************************************************
 */

#include "gui/widgets/Widgets.hpp"

#include <cmath>
#include "gui/Assets.hpp"
#include "gui/Format.hpp"

#include <cstdio>

using namespace SDK::GUI;

namespace Widgets
{

// --- SensorStatusRow ---------------------------------------------------------

SensorStatusRow::SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : SDK::LVGL::SensorStatusRow(parent, x, y, w, h, &img_sensorgpslight, &img_sensorhrlight)
{
}

// --- HeartRateZone ---------------------------------------------------------

namespace
{

// Geometry measured from the TouchGFX design's bitmaps (HeartRateZoneGroup and
// HeartRateZone1..5), in the widget's own coordinates: one circle centred at
// (105, 116) carries the five-segment bar, the thicker marker over the active
// segment, and the arrow pointing at it from inside.
constexpr int32_t kArcCx         = 105;
constexpr int32_t kArcCy         = 116;
constexpr int32_t kBarRadius     = 113;   // centre-line radius of the bar
constexpr int32_t kBarWidth      = 8;
constexpr int32_t kMarkerRadius  = 111;   // the marker is thicker and reaches further in
constexpr int32_t kMarkerWidth   = 12;
constexpr int32_t kSegmentDeg    = 24;    // each zone spans 24 degrees, 2 degrees apart
constexpr float   kArrowTipR     = 99.0f; // arrow apex, just inside the marker
constexpr float   kArrowBaseR    = 89.0f;
constexpr float   kArrowHalfW    = 5.5f;  // half the base width, along the tangent
constexpr int32_t kSegmentStart[HeartRateZone::kZoneCount] = { -64, -38, -12, 14, 40 };
constexpr uint32_t kZoneColor[HeartRateZone::kZoneCount] = {
    Color::GRAY, Color::CHARTREUSE, Color::YELLOW, Color::YELLOW_DARK, Color::RED
};

/// Draw the arrow: the object's only content is one filled triangle.
void drawArrowCb(lv_event_t* e)
{
    auto* obj   = static_cast<lv_obj_t*>(lv_event_get_target(e));
    auto* arrow = static_cast<const HeartRateZone::Arrow*>(lv_event_get_user_data(e));

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.color = arrow->color;
    dsc.opa   = LV_OPA_COVER;
    for (int i = 0; i < 3; ++i) {
        dsc.p[i].x = coords.x1 + arrow->p[i].x;
        dsc.p[i].y = coords.y1 + arrow->p[i].y;
    }
    lv_draw_triangle(lv_event_get_layer(e), &dsc);
}

} // namespace

HeartRateZone::HeartRateZone(lv_obj_t* parent, int32_t x, int32_t y)
    : mX(x)
    , mY(y)
{
    for (uint8_t i = 0; i < kZoneCount; ++i) {
        Theme::arc(parent, x + kArcCx, y + kArcCy, kBarRadius, kBarWidth,
                   kSegmentStart[i], kSegmentStart[i] + kSegmentDeg, kZoneColor[i], false);
    }

    // One marker and one arrow, re-aimed at whichever zone is active.
    mMarker = Theme::arc(parent, x + kArcCx, y + kArcCy, kMarkerRadius, kMarkerWidth,
                         kSegmentStart[0], kSegmentStart[0] + kSegmentDeg, kZoneColor[0], false);
    lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);

    // The arrow's host covers the whole bar area so any zone's triangle fits.
    mArrow = Theme::container(parent, x, y, 210, 69);
    lv_obj_add_event_cb(mArrow, drawArrowCb, LV_EVENT_DRAW_MAIN, &mArrowDsc);
    lv_obj_add_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
}

void HeartRateZone::showZone(int zone)
{
    if (zone == mActive) {
        return;
    }
    mActive = zone;

    if (zone < 0) {
        lv_obj_add_flag(mMarker, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    Theme::setArc(mMarker, kSegmentStart[zone], kSegmentStart[zone] + kSegmentDeg);
    Theme::setArcColor(mMarker, kZoneColor[zone]);
    lv_obj_remove_flag(mMarker, LV_OBJ_FLAG_HIDDEN);

    // Triangle on the segment's mid angle: apex towards the bar, base inside.
    // TouchGFX angles: 0 = 12 o'clock, clockwise. Radial unit vector is
    // (sin a, -cos a); the tangent is (cos a, sin a).
    const float a  = (kSegmentStart[zone] + kSegmentDeg / 2.0f) * 3.14159265f / 180.0f;
    const float rx = sinf(a), ry = -cosf(a);
    const float tx = cosf(a), ty = sinf(a);
    const float cx = static_cast<float>(kArcCx), cy = static_cast<float>(kArcCy) + 0.4f;
    // lv_value_precise_t is integer unless LV_USE_FLOAT is on; round to the pixel grid.
    auto pt = [](float px, float py) {
        return lv_point_precise_t { static_cast<lv_value_precise_t>(lroundf(px)),
                                    static_cast<lv_value_precise_t>(lroundf(py)) };
    };
    mArrowDsc.p[0] = pt(cx + kArrowTipR * rx, cy + kArrowTipR * ry);
    mArrowDsc.p[1] = pt(cx + kArrowBaseR * rx + kArrowHalfW * tx, cy + kArrowBaseR * ry + kArrowHalfW * ty);
    mArrowDsc.p[2] = pt(cx + kArrowBaseR * rx - kArrowHalfW * tx, cy + kArrowBaseR * ry - kArrowHalfW * ty);
    mArrowDsc.color = Theme::rgb(kZoneColor[zone]);
    lv_obj_remove_flag(mArrow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(mArrow);
}

void HeartRateZone::setHR(float bpm, const uint8_t* thresholds, uint8_t thresholdCount)
{
    if (!thresholds || thresholdCount == 0) {
        return;
    }
    if (thresholdCount > kZoneCount) {
        thresholdCount = kZoneCount;
    }
    // Highest threshold the HR exceeds selects the zone; below all: none.
    // Threshold i is the entry to zone i, so the walk starts at the zone of
    // the highest supplied threshold.
    int active = thresholdCount - 1;
    for (int i = thresholdCount - 1; i >= 0; --i) {
        if (bpm > thresholds[i]) {
            break;
        }
        --active;
    }
    showZone(active);
}

// --- PauseIndicator --------------------------------------------------------

PauseIndicator::PauseIndicator(lv_obj_t* parent, int32_t y)
{
    // The TouchGFX design clips a radius-116 circle centred 82 px above the
    // panel to a 164 x 34 box, giving a shallow dome. Reproduce it the same
    // way: a clipping container with an oversized circle inside.
    lv_obj_t* clip = Theme::container(parent, 38, y, 164, 34);
    lv_obj_t* dome = Theme::dot(clip, 82, -82, 116, Color::GRAY_DARK);
    (void)dome;
    Theme::imageTinted(parent, &img_pause_14x14, 76, y + 10, Color::WHITE);
    mLabel = Theme::label(parent, Theme::Font::Italic18, "", 88, y + 5, 85);
}

void PauseIndicator::setTime(std::time_t seconds)
{
    const unsigned h = static_cast<unsigned>(seconds / 3600);
    const unsigned m = static_cast<unsigned>((seconds % 3600) / 60);
    const unsigned s = static_cast<unsigned>(seconds % 60);
    if (h > 0) {
        lv_label_set_text_fmt(mLabel, "%u:%02u:%02u", h, m, s);
    } else {
        lv_label_set_text_fmt(mLabel, "%02u:%02u", m, s);
    }
}

// --- InfoCarousel ----------------------------------------------------------

InfoCarousel::InfoCarousel(lv_obj_t* parent, int32_t x, int32_t y)
{
    mTitle = Theme::label(parent, Theme::Font::Italic18, "", x, y + 10, 160);
    mValue = Theme::label(parent, Theme::Font::SemiBold30, "", x, y + 29, 160);
    Theme::hline(parent, x, y + 65, 160, Color::GRAY_DARK);
}

InfoCarousel::~InfoCarousel()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void InfoCarousel::setPeriodMs(uint32_t ms)
{
    mPeriodMs = ms ? ms : 1;
    if (mTimer) {
        lv_timer_set_period(mTimer, mPeriodMs);
    }
}

void InfoCarousel::setCallback(UpdateCb cb, void* user)
{
    mCb   = cb;
    mUser = user;
}

void InfoCarousel::setCount(uint16_t count)
{
    mCount = count;
    mIndex = 0;
    if (mCount > 1 && !mTimer) {
        mTimer = lv_timer_create(&InfoCarousel::tickCb, mPeriodMs, this);
    } else if (mCount <= 1 && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
    if (mTimer) {
        lv_timer_reset(mTimer);
    }
    fire();
}

void InfoCarousel::refresh()
{
    fire();
}

void InfoCarousel::setTitle(const char* text)
{
    lv_label_set_text(mTitle, text);
}

void InfoCarousel::setValue(const char* text)
{
    if (text) {
        lv_label_set_text(mValue, text);
        lv_obj_remove_flag(mValue, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mValue, LV_OBJ_FLAG_HIDDEN);
    }
}

void InfoCarousel::tickCb(lv_timer_t* t)
{
    auto* self = static_cast<InfoCarousel*>(lv_timer_get_user_data(t));
    if (self->mCount > 0) {
        self->mIndex = static_cast<uint16_t>((self->mIndex + 1) % self->mCount);
    }
    self->fire();
}

void InfoCarousel::fire()
{
    if (mCb) {
        mCb(mUser, static_cast<int16_t>(mIndex));
    }
}

// --- Map -------------------------------------------------------------------

Map::Map(lv_obj_t* parent, int32_t x, int32_t y)
{
    mRoot = Theme::container(parent, x, y, 150, 150);
    mLine = lv_line_create(mRoot);
    lv_obj_set_pos(mLine, 0, 0);
    lv_obj_set_size(mLine, 150, 150);
    lv_obj_set_style_line_width(mLine, 3, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(mLine, true, LV_PART_MAIN);
    lv_obj_set_style_line_color(mLine, Theme::rgb(Color::YELLOW_DARK), LV_PART_MAIN);
    mStart = Theme::dot(mRoot, 75, 75, 4, Color::CHARTREUSE);
    mEnd   = Theme::dot(mRoot, 75, 75, 4, Color::RED);
}

void Map::setMap(const SDK::TrackMapScreen& map)
{
    if (map.points.empty()) {
        return;
    }
    // Centre the route's bounding box in the 142 px area inside a 4 px inset,
    // as the TouchGFX Map does. +1 keeps the 3 px stroke centred on the point.
    const int32_t contentW = map.maxx - map.minx;
    const int32_t contentH = map.maxy - map.miny;
    const int32_t baseX = 4 + (142 - contentW) / 2 - map.minx;
    const int32_t baseY = 4 + (142 - contentH) / 2 - map.miny;

    mPoints.resize(map.points.size());
    for (size_t i = 0; i < map.points.size(); ++i) {
        mPoints[i].x = static_cast<lv_value_precise_t>(baseX + map.points[i].x + 1);
        mPoints[i].y = static_cast<lv_value_precise_t>(baseY + map.points[i].y + 1);
    }
    lv_line_set_points(mLine, mPoints.data(), static_cast<uint32_t>(mPoints.size()));

    const auto& first = map.points.front();
    const auto& last  = map.points.back();
    lv_obj_set_pos(mStart, baseX + first.x - 4, baseY + first.y - 4);
    lv_obj_set_pos(mEnd,   baseX + last.x - 4,  baseY + last.y - 4);
}

// --- IntervalsTimer --------------------------------------------------------

namespace
{
constexpr std::time_t kMaxIntervalSec = 5999;   // 99:59
} // namespace

IntervalsTimer::IntervalsTimer(lv_obj_t* parent, int32_t x, int32_t y)
{
    lv_obj_t* box = Theme::container(parent, x, y, 190, 91);
    // The 60 px readout box starts 5 px above the container in the TouchGFX
    // design; LVGL clips children, so keep the box inside and offset the text.
    mTimer       = Theme::label(box, Theme::Font::SemiBold60, "00:00", 0, -5, 190);
    lv_obj_add_flag(box, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    mDescription = Theme::label(box, Theme::Font::Regular18, "", 0, 60, 190);
    mLine        = Theme::hline(box, 0, 88, 190, Color::WHITE);
}

void IntervalsTimer::setPhaseTime(std::time_t sec, Track::IntervalsMetric metric)
{
    setTimerClamped(sec);
    switch (metric) {
        case Track::IntervalsMetric::TIME_OPEN:      setDescription("Open");      break;
        case Track::IntervalsMetric::TIME_REMAINING: setDescription("Remaining"); break;
        case Track::IntervalsMetric::TIME_ELAPSED:   setDescription("Elapsed");   break;
        default: break;
    }
}

void IntervalsTimer::setPhaseDistance(float distInUnits, bool imperial)
{
    if (distInUnits < 0.0f) {
        distInUnits = 0.0f;
    }
    // TouchGFX prints "%05.02f" below 100 and "%05.01f" above: five characters,
    // zero padded on the left.
    char buf[12];
    Fmt::fixedPadded(buf, sizeof(buf), distInUnits, distInUnits < 100.0f ? 2 : 1, 5);
    lv_label_set_text(mTimer, buf);
    char desc[24];
    snprintf(desc, sizeof(desc), "%s remaining", Fmt::units(imperial));
    setDescription(desc);
}

void IntervalsTimer::setRemainingTime(std::time_t sec)
{
    setTimerClamped(sec);
    setDescription(nullptr);
}

void IntervalsTimer::setOpen()
{
    lv_label_set_text(mTimer, "Open");
    setDescription(nullptr);
}

void IntervalsTimer::setColor(uint32_t color)
{
    lv_obj_set_style_text_color(mTimer, Theme::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_color(mDescription, Theme::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mLine, Theme::rgb(color), LV_PART_MAIN);
}

void IntervalsTimer::setLineVisible(bool visible)
{
    if (visible) {
        lv_obj_remove_flag(mLine, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mLine, LV_OBJ_FLAG_HIDDEN);
    }
}

void IntervalsTimer::setDescriptionVisible(bool visible)
{
    if (visible) {
        lv_obj_remove_flag(mDescription, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mDescription, LV_OBJ_FLAG_HIDDEN);
    }
}

void IntervalsTimer::setTimerClamped(std::time_t sec)
{
    if (sec < 0) {
        sec = 0;
    }
    if (sec > kMaxIntervalSec) {
        sec = kMaxIntervalSec;
    }
    lv_label_set_text_fmt(mTimer, "%02u:%02u", static_cast<unsigned>(sec / 60), static_cast<unsigned>(sec % 60));
}

void IntervalsTimer::setDescription(const char* text)
{
    if (text) {
        lv_label_set_text(mDescription, text);
        lv_obj_remove_flag(mDescription, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mDescription, LV_OBJ_FLAG_HIDDEN);
    }
}

// --- TwoTonePicker ---------------------------------------------------------

namespace
{
constexpr uint32_t kPickerActive   = Color::TEAL;
constexpr uint32_t kPickerInactive = Color::WHITE;
} // namespace

TwoTonePicker::TwoTonePicker(lv_obj_t* parent)
{
    using F = Theme::Font;
    mButtons = std::make_unique<Buttons>(parent);
    // Fixed bezel mapping: left = scroll, R1 = confirm/advance, R2 = skip/back.
    mButtons->set(Buttons::WHITE, Buttons::WHITE, Buttons::AMBER, Buttons::WHITE);

    mSubLeft  = Theme::label(parent, F::Italic20, "", 5, 58, 110, LV_TEXT_ALIGN_CENTER, kPickerActive);
    mSubRight = Theme::label(parent, F::Italic20, "", 127, 58, 108, LV_TEXT_ALIGN_CENTER, kPickerInactive);
    mValLeft  = Theme::label(parent, F::SemiBold60, "", 5, 92, 110, LV_TEXT_ALIGN_RIGHT, kPickerActive);
    mValSep   = Theme::label(parent, F::Light60, "", 110, 92, 22, LV_TEXT_ALIGN_CENTER, kPickerInactive);
    mValRight = Theme::label(parent, F::Light60, "", 127, 92, 108, LV_TEXT_ALIGN_LEFT, kPickerInactive);
    mNext1    = Theme::label(parent, F::Medium40, "", 5, 151, 110, LV_TEXT_ALIGN_RIGHT, kPickerInactive);
    mNext2    = Theme::label(parent, F::Medium25, "", 5, 193, 110, LV_TEXT_ALIGN_RIGHT, kPickerInactive);
    mTitle    = std::make_unique<Title>(parent, "");
}

void TwoTonePicker::setTitle(const char* title)
{
    mTitle->setText(title);
}

void TwoTonePicker::renderSubtitleSingle(const char* label)
{
    lv_obj_set_pos(mSubLeft, 20, 58);
    lv_obj_set_width(mSubLeft, 200);
    lv_label_set_text(mSubLeft, label);
    lv_obj_set_style_text_color(mSubLeft, Theme::rgb(kPickerActive), LV_PART_MAIN);
    lv_label_set_text(mSubRight, "");
}

void TwoTonePicker::renderSubtitleDual(const char* left, const char* right, bool leftActive)
{
    lv_obj_set_pos(mSubLeft, 5, 58);
    lv_obj_set_width(mSubLeft, 110);
    lv_label_set_text(mSubLeft, left);
    lv_obj_set_style_text_color(mSubLeft, Theme::rgb(leftActive ? kPickerActive : kPickerInactive), LV_PART_MAIN);
    lv_label_set_text(mSubRight, right);
    lv_obj_set_style_text_color(mSubRight, Theme::rgb(leftActive ? kPickerInactive : kPickerActive), LV_PART_MAIN);
}

void TwoTonePicker::renderValue(bool leftActive, const char* left, const char* right, const char* sep,
                                const char* up1, const char* up2)
{
    lv_label_set_text(mValSep, sep);

    const auto leftFont  = Theme::font(leftActive ? Theme::Font::SemiBold60 : Theme::Font::Light60);
    const auto rightFont = Theme::font(leftActive ? Theme::Font::Light60 : Theme::Font::SemiBold60);

    lv_label_set_text(mValLeft, left);
    lv_obj_set_style_text_font(mValLeft, leftFont, LV_PART_MAIN);
    lv_obj_set_style_text_color(mValLeft, Theme::rgb(leftActive ? kPickerActive : kPickerInactive), LV_PART_MAIN);

    lv_label_set_text(mValRight, right);
    lv_obj_set_style_text_font(mValRight, rightFont, LV_PART_MAIN);
    lv_obj_set_style_text_color(mValRight, Theme::rgb(leftActive ? kPickerInactive : kPickerActive), LV_PART_MAIN);

    // Upcoming values sit under the active component, pulled towards the centre.
    if (leftActive) {
        lv_obj_set_pos(mNext1, 5, 151);
        lv_obj_set_width(mNext1, 110);
        lv_obj_set_pos(mNext2, 5, 193);
        lv_obj_set_width(mNext2, 110);
        lv_obj_set_style_text_align(mNext1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_set_style_text_align(mNext2, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    } else {
        lv_obj_set_pos(mNext1, 127, 151);
        lv_obj_set_width(mNext1, 108);
        lv_obj_set_pos(mNext2, 127, 193);
        lv_obj_set_width(mNext2, 108);
        lv_obj_set_style_text_align(mNext1, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_style_text_align(mNext2, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    }
    lv_label_set_text(mNext1, up1 ? up1 : "");
    lv_label_set_text(mNext2, up2 ? up2 : "");
}

} // namespace Widgets
