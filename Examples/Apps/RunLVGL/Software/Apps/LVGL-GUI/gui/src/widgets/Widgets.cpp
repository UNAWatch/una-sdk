/**
 ******************************************************************************
 * @file    Widgets.cpp
 * @brief   The Run app's shared widgets, rebuilt from LVGL primitives.
 ******************************************************************************
 */

#include "gui/widgets/Widgets.hpp"
#include "gui/Assets.hpp"

#include <cstdio>

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"

using namespace SDK::GUI;

namespace Widgets
{

// --- Buttons -----------------------------------------------------------------

namespace
{
constexpr int32_t kButtonRadius = 113;
constexpr int32_t kButtonWidth  = 6;
// TouchGFX arc bounds per button: L1, L2, R1, R2.
constexpr int32_t kButtonArcs[4][2] = { { 291, 308 }, { 232, 249 }, { 52, 69 }, { 111, 128 } };

uint32_t buttonColor(Buttons::Color c)
{
    switch (c) {
        case Buttons::WHITE: return Color::WHITE;
        case Buttons::AMBER: return Color::YELLOW_DARK;   // C0 80 00
        case Buttons::RED:   return Color::RED;
        case Buttons::GREEN: return Color::CHARTREUSE;    // 40 C0 00
        default:             return Color::BLACK;
    }
}
} // namespace

Buttons::Buttons(lv_obj_t* parent)
{
    for (int i = 0; i < 4; ++i) {
        mArc[i] = Theme::arc(parent, kCx, kCy, kButtonRadius, kButtonWidth,
                             kButtonArcs[i][0], kButtonArcs[i][1], Color::WHITE);
        lv_obj_add_flag(mArc[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void Buttons::set(Color l1, Color l2, Color r1, Color r2)
{
    apply(0, l1);
    apply(1, l2);
    apply(2, r1);
    apply(3, r2);
}

void Buttons::apply(int idx, Color c)
{
    if (c == NONE) {
        lv_obj_add_flag(mArc[idx], LV_OBJ_FLAG_HIDDEN);
        return;
    }
    Theme::setArcColor(mArc[idx], buttonColor(c));
    lv_obj_remove_flag(mArc[idx], LV_OBJ_FLAG_HIDDEN);
}

// --- ScrollIndicator ---------------------------------------------------------

namespace
{
constexpr int32_t kRailRadius = 112;
constexpr int32_t kRailWidth  = 9;   // TouchGFX draws 8.5
} // namespace

ScrollIndicator::ScrollIndicator(lv_obj_t* parent, const Config& cfg)
    : mCfg(cfg)
{
    mRail   = Theme::arc(parent, kCx, kCy, kRailRadius, kRailWidth,
                         static_cast<int32_t>(cfg.railMin), static_cast<int32_t>(cfg.railMax),
                         Color::GRAY_DARK);
    mHandle = Theme::arc(parent, kCx, kCy, kRailRadius, kRailWidth, 0, 1, Color::WHITE);
    update();
}

void ScrollIndicator::setConfig(const Config& cfg)
{
    mCfg = cfg;
    Theme::setArc(mRail, static_cast<int32_t>(cfg.railMin), static_cast<int32_t>(cfg.railMax));
    update();
}

void ScrollIndicator::setCount(uint16_t count)
{
    mCount = (count == 0) ? 1 : count;
    mPos   = 0;
    update();
}

ScrollIndicator::~ScrollIndicator()
{
    lv_anim_delete(this, nullptr);
}

void ScrollIndicator::setActive(uint16_t index)
{
    lv_anim_delete(this, nullptr);
    mPos = (mCount <= 1) ? 0 : (index >= mCount ? mCount - 1 : index);
    update();
}

void ScrollIndicator::animateTo(uint16_t index, uint32_t ms)
{
    if (mCount <= 1 || index >= mCount) {
        setActive(index);
        return;
    }
    const uint16_t from = mPos;
    const int      delta = static_cast<int>(index) - static_cast<int>(from);
    // The TouchGFX indicator slides a second handle in from the far end on a
    // wrap; here a wrap simply jumps.
    if (delta > 1 || delta < -1 || ms == 0) {
        setActive(index);
        return;
    }
    lv_anim_delete(this, nullptr);
    mPos = index;

    // Animate the handle's start angle in tenths of a degree.
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, static_cast<int32_t>(startAngle(from) * 10.0f),
                       static_cast<int32_t>(startAngle(index) * 10.0f));
    lv_anim_set_duration(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, &ScrollIndicator::animExecCb);
    lv_anim_start(&a);
}

void ScrollIndicator::animExecCb(void* var, int32_t value)
{
    static_cast<ScrollIndicator*>(var)->setHandle(static_cast<float>(value) / 10.0f);
}

float ScrollIndicator::startAngle(uint16_t index) const
{
    // Same mapping as the TouchGFX ScrollIndicator: item 0 sits at the top of
    // the rail (railMax end) and later items step towards railMin.
    if (mCount <= 1) {
        return (mCfg.railMax + mCfg.railMin - mCfg.handleLen) / 2.0f;
    }
    const float step = (mCfg.railMax - mCfg.railMin - mCfg.handleLen) / static_cast<float>(mCount - 1);
    return (mCfg.railMax - mCfg.handleLen) - step * static_cast<float>(index);
}

void ScrollIndicator::setHandle(float startDeg)
{
    Theme::setArc(mHandle, static_cast<int32_t>(startDeg + 0.5f),
                  static_cast<int32_t>(startDeg + mCfg.handleLen + 0.5f));
}

void ScrollIndicator::update()
{
    if (mCount <= 1) {
        lv_obj_add_flag(mHandle, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    setHandle(startAngle(mPos));
    lv_obj_remove_flag(mHandle, LV_OBJ_FLAG_HIDDEN);
}

// --- Title -------------------------------------------------------------------

Title::Title(lv_obj_t* parent, const char* text)
{
    mLine  = Theme::hline(parent, 50, 38, 140, Color::GRAY_DARK);
    mLabel = Theme::label(parent, Theme::Font::Italic18, text, 60, 11, 120);
}

void Title::setText(const char* text)
{
    lv_label_set_text(mLabel, text);
}

void Title::setColor(uint32_t color)
{
    lv_obj_set_style_bg_color(mLine, Theme::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_color(mLabel, Theme::rgb(color), LV_PART_MAIN);
}

void Title::setVisible(bool visible)
{
    if (visible) {
        lv_obj_remove_flag(mLine, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(mLabel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(mLine, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

// --- Battery -----------------------------------------------------------------

namespace
{
lv_obj_t* roundedBox(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius)
{
    lv_obj_t* b = Theme::container(parent, x, y, w, h);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, Theme::rgb(Color::GRAY_DARK), LV_PART_MAIN);
    lv_obj_set_style_radius(b, radius, LV_PART_MAIN);
    return b;
}
} // namespace

Battery::Battery(lv_obj_t* parent, int32_t x, int32_t y)
{
    // Segment spans taken from the TouchGFX design (box plus round-cap edges).
    mSeg[0] = roundedBox(parent, x + 0,  y, 20, 28, 5);
    mSeg[1] = roundedBox(parent, x + 22, y, 20, 28, 3);
    mSeg[2] = roundedBox(parent, x + 44, y, 20, 28, 3);
    mSeg[3] = roundedBox(parent, x + 66, y, 20, 28, 5);
    mNub    = roundedBox(parent, x + 82, y + 6, 8, 16, 4);
    setLevel(0);
}

void Battery::setLevel(uint8_t level)
{
    const uint32_t c1 = (level == 0) ? Color::GRAY_DARK : (level < 25) ? Color::RED : Color::TEAL;
    const uint32_t c2 = level >= 25 ? Color::TEAL : Color::GRAY_DARK;
    const uint32_t c3 = level >= 50 ? Color::TEAL : Color::GRAY_DARK;
    const uint32_t c4 = level >= 75 ? Color::TEAL : Color::GRAY_DARK;
    lv_obj_set_style_bg_color(mSeg[0], Theme::rgb(c1), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[1], Theme::rgb(c2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[2], Theme::rgb(c3), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mSeg[3], Theme::rgb(c4), LV_PART_MAIN);
    lv_obj_set_style_bg_color(mNub,    Theme::rgb(c4), LV_PART_MAIN);
}

// --- SensorStatusRow ---------------------------------------------------------

SensorStatusRow::State SensorStatusRow::hrState(uint8_t accessoryState)
{
    // SDK::Accessory::State: UNAVAILABLE(0)/IDLE(1) -> Absent,
    // SEARCHING(2)/CONNECTING(3)/LOST(5) -> Searching, CONNECTED(4) -> Connected.
    switch (accessoryState) {
        case 2: case 3: case 5: return State::Searching;
        case 4:                 return State::Connected;
        default:                return State::Absent;
    }
}

SensorStatusRow::State SensorStatusRow::hrStateFromSource(uint8_t accessoryState, uint8_t hrSource)
{
    if (hrState(accessoryState) == State::Absent) {
        return State::Absent;
    }
    constexpr uint8_t kAccessoryConnected = 4;
    const bool live = accessoryState == kAccessoryConnected &&
                      hrSource == static_cast<uint8_t>(
                          SDK::SensorDataParser::HeartRateEx::Source::EXTERNAL);
    return live ? State::Connected : State::Searching;
}

SensorStatusRow::SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
    : mW(w), mH(h)
{
    mRow = Theme::container(parent, x, y, w, h);
    mGps = Theme::image(mRow, &img_sensorgpslight, 0, 0);
    mHr  = Theme::image(mRow, &img_sensorhrlight, 0, 0);
    update();
}

SensorStatusRow::~SensorStatusRow()
{
    if (mTimer) {
        lv_timer_delete(mTimer);
    }
}

void SensorStatusRow::setGps(State s)
{
    if (s != mGpsState) {
        mGpsState = s;
        update();
    }
}

void SensorStatusRow::setHr(State s)
{
    if (s != mHrState) {
        mHrState = s;
        update();
    }
}

void SensorStatusRow::tickCb(lv_timer_t* t)
{
    auto* self = static_cast<SensorStatusRow*>(lv_timer_get_user_data(t));
    self->mLight = !self->mLight;
    self->applyIcons();
}

void SensorStatusRow::update()
{
    layout();
    const bool animating = mGpsState == State::Searching || mHrState == State::Searching;
    if (animating && !mTimer) {
        mLight = true;
        mTimer = lv_timer_create(&SensorStatusRow::tickCb, 500, this);
    } else if (!animating && mTimer) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
    applyIcons();
}

void SensorStatusRow::layout()
{
    const bool gpsOn = mGpsState != State::Absent;
    const bool hrOn  = mHrState != State::Absent;
    const int32_t gw = img_sensorgpslight.header.w, gh = img_sensorgpslight.header.h;
    const int32_t hw = img_sensorhrlight.header.w,  hh = img_sensorhrlight.header.h;
    const int32_t gap = 24;

    // Icons are bottom-aligned; centred as a pair, or singly.
    if (gpsOn && hrOn) {
        const int32_t x = (mW - (gw + gap + hw)) / 2;
        lv_obj_set_pos(mGps, x, mH - gh);
        lv_obj_set_pos(mHr, x + gw + gap, mH - hh);
    } else if (gpsOn) {
        lv_obj_set_pos(mGps, (mW - gw) / 2, mH - gh);
    } else if (hrOn) {
        lv_obj_set_pos(mHr, (mW - hw) / 2, mH - hh);
    }
}

void SensorStatusRow::applyIcons()
{
    auto applyOne = [this](lv_obj_t* img, State s, const lv_image_dsc_t* dark, const lv_image_dsc_t* light) {
        if (s == State::Absent) {
            lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
            return;
        }
        lv_obj_remove_flag(img, LV_OBJ_FLAG_HIDDEN);
        lv_image_set_src(img, (s == State::Connected || mLight) ? light : dark);
    };
    applyOne(mGps, mGpsState, &img_sensorgpsdark, &img_sensorgpslight);
    applyOne(mHr,  mHrState,  &img_sensorhrdark,  &img_sensorhrlight);
}

// --- HeartRateZone -----------------------------------------------------------

HeartRateZone::HeartRateZone(lv_obj_t* parent, int32_t x, int32_t y)
{
    Theme::image(parent, &img_heartratezonegroup, x, y);
    const lv_image_dsc_t* zoneImgs[kZoneCount] = {
        &img_heartratezone1, &img_heartratezone2, &img_heartratezone3,
        &img_heartratezone4, &img_heartratezone5
    };
    const int32_t zoneOffsets[kZoneCount][2] = { { 0, 28 }, { 33, 3 }, { 80, 0 }, { 131, 3 }, { 173, 28 } };
    for (uint8_t i = 0; i < kZoneCount; ++i) {
        mZones[i] = Theme::image(parent, zoneImgs[i], x + zoneOffsets[i][0], y + zoneOffsets[i][1]);
        lv_obj_add_flag(mZones[i], LV_OBJ_FLAG_HIDDEN);
    }
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
    int active = kZoneCount - 1;
    for (int i = thresholdCount - 1; i >= 0; --i) {
        if (bpm > thresholds[i]) {
            break;
        }
        --active;
    }
    for (int i = 0; i < kZoneCount; ++i) {
        if (i == active) {
            lv_obj_remove_flag(mZones[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(mZones[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// --- PauseIndicator ----------------------------------------------------------

PauseIndicator::PauseIndicator(lv_obj_t* parent, int32_t y)
{
    // The TouchGFX design clips a radius-116 circle centred 82 px above the
    // panel to a 164 x 34 box, giving a shallow dome. Reproduce it the same
    // way: a clipping container with an oversized circle inside.
    lv_obj_t* clip = Theme::container(parent, 38, y, 164, 34);
    lv_obj_t* dome = Theme::dot(clip, 82, -82, 116, Color::GRAY_DARK);
    (void)dome;
    Theme::image(parent, &img_pause_14x14, 76, y + 10);
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

// --- InfoCarousel ------------------------------------------------------------

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

// --- TimerRing ---------------------------------------------------------------

namespace
{
constexpr int32_t kRingRadius = 112;
constexpr int32_t kRingWidth  = 9;
constexpr int32_t kRingStart  = 138;   // TouchGFX angles: 138 -> 402 (264 degrees)
constexpr int32_t kRingSpan   = 264;
} // namespace

TimerRing::TimerRing(lv_obj_t* parent)
{
    mTrack    = Theme::arc(parent, kCx, kCy, kRingRadius, kRingWidth,
                           kRingStart, kRingStart + kRingSpan, Color::GRAY_DARK);
    mProgress = Theme::arc(parent, kCx, kCy, kRingRadius, kRingWidth,
                           kRingStart, kRingStart + 1, Color::YELLOW_DARK);
    setProgress(0);
}

void TimerRing::setColor(uint32_t color)
{
    Theme::setArcColor(mProgress, color);
}

void TimerRing::setProgress(uint32_t permille)
{
    if (permille > 1000) {
        permille = 1000;
    }
    const int32_t span = static_cast<int32_t>((kRingSpan * permille + 500) / 1000);
    if (span <= 0) {
        lv_obj_add_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    Theme::setArc(mProgress, kRingStart, kRingStart + span);
    lv_obj_remove_flag(mProgress, LV_OBJ_FLAG_HIDDEN);
}

// --- Map ---------------------------------------------------------------------

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

} // namespace Widgets
