/**
 ******************************************************************************
 * @file    Kit.cpp
 * @brief   Track Run's kit components (see Kit.hpp).
 ******************************************************************************
 */

#include "gui/widgets/Kit.hpp"

#include <cstdio>
#include <cstring>

namespace Kit
{

namespace
{

int32_t textWidth(lv_obj_t* label)
{
    lv_point_t size;
    lv_text_get_size(&size, lv_label_get_text(label), lv_obj_get_style_text_font(label, LV_PART_MAIN), 0, 0,
                     LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

int32_t widthOf(lv_obj_t* obj)
{
    if (lv_obj_check_type(obj, &lv_label_class)) {
        return textWidth(obj);
    }
    if (lv_obj_check_type(obj, &lv_image_class)) {
        const void* src = lv_image_get_src(obj);
        if (src != nullptr && lv_image_src_get_type(src) == LV_IMAGE_SRC_VARIABLE) {
            return static_cast<const lv_image_dsc_t*>(src)->header.w;
        }
    }
    const int32_t w = lv_obj_get_style_width(obj, LV_PART_MAIN);
    return LV_COORD_IS_SPEC(w) ? lv_obj_get_width(obj) : w;
}

/// A ring arc with cut (not rounded) ends, as the design draws the bezel.
lv_obj_t* ringArc(lv_obj_t* parent, int32_t radius, int32_t width, int32_t from, int32_t to, uint32_t color)
{
    return Theme::arc(parent, kRingCx, kRingCy, radius, width, from, to, color, false);
}

} // namespace

lv_obj_t* text(lv_obj_t* parent, Theme::Font f, const char* s, uint32_t color)
{
    return Theme::label(parent, f, s, 0, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT, color);
}

lv_obj_t* divider(lv_obj_t* parent, int32_t y)
{
    return Theme::box(parent, kDividerX, y, kDividerW, kDividerH, kTeal);
}

// -----------------------------------------------------------------------------

void Row::add(lv_obj_t* obj, int32_t gapBefore)
{
    if (mCount < kMax) {
        mObj[mCount] = obj;
        mGap[mCount] = gapBefore;
        mX[mCount]   = -10000;
        ++mCount;
    }
}

void Row::layout(int32_t centreX)
{
    int32_t w[kMax] = {};
    int32_t total = 0;
    bool first = true;
    for (uint8_t i = 0; i < mCount; ++i) {
        if (lv_obj_has_flag(mObj[i], LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        w[i] = widthOf(mObj[i]);
        // A content-sized label is wrapped by LVGL against where it sits
        // now, before this moves it (else "Lane shortcut" goes on two lines):
        // it gets its one-line width, set only when that changes, and a
        // little more, as a glyph's ink may pass its advance (the "k"
        // of "New track" lost a column); the group is placed by the advance.
        if (lv_obj_check_type(mObj[i], &lv_label_class) && lv_obj_get_width(mObj[i]) != w[i] + kInkSlack) {
            lv_obj_set_width(mObj[i], w[i] + kInkSlack);
        }
        total += w[i] + (first ? 0 : mGap[i]);
        first = false;
    }
    mWidth = total;
    int32_t x = centreX - total / 2;
    first = true;
    for (uint8_t i = 0; i < mCount; ++i) {
        if (lv_obj_has_flag(mObj[i], LV_OBJ_FLAG_HIDDEN)) {
            continue;
        }
        x += first ? 0 : mGap[i];
        first = false;
        if (x != mX[i]) {
            mX[i] = x;
            lv_obj_set_x(mObj[i], x);
        }
        x += w[i];
    }
}

// -----------------------------------------------------------------------------

namespace
{
// The box: from the digits' tops (46 px above the baseline in the 60 px
// face) to below a comma's tail ("1,600"), the screen's width less the ring.
constexpr int32_t kFigureAbove = 46;
constexpr int32_t kFigureBelow = 10;
constexpr int32_t kFigureX     = 20;
constexpr int32_t kFigureW     = 200;

int32_t baselineFromTop(const lv_font_t* f)
{
    return lv_font_get_line_height(f) - f->base_line;
}
} // namespace

BigFigure::BigFigure(lv_obj_t* parent, int32_t baselineY, Theme::Font font, uint32_t color)
    : mBaseline(baselineY)
{
    mBox  = Theme::container(parent, kFigureX, baselineY - kFigureAbove, kFigureW, kFigureAbove + kFigureBelow);
    mHead = Theme::label(mBox, font, "", 0, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT, color);
    mTail = Theme::label(mBox, font, "", 0, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT, color);
    mRow.add(mHead);
    mRow.add(mTail);
    place();
}

void BigFigure::place()
{
    const int32_t y = kFigureAbove - baselineFromTop(lv_obj_get_style_text_font(mHead, LV_PART_MAIN));
    lv_obj_set_y(mHead, y);
    lv_obj_set_y(mTail, y);
}

void BigFigure::set(const char* text)
{
    if (std::strncmp(text, mText, sizeof(mText)) == 0) {
        return;
    }
    std::snprintf(mText, sizeof(mText), "%s", text);
    const size_t n    = std::strlen(mText);
    const size_t head = n > 2 ? n - 2 : 0;
    char h[sizeof(mText)];
    std::snprintf(h, sizeof(h), "%.*s", static_cast<int>(head), mText);
    if (std::strcmp(lv_label_get_text(mHead), h) != 0) {
        lv_label_set_text(mHead, h);
    }
    if (std::strcmp(lv_label_get_text(mTail), mText + head) != 0) {
        lv_label_set_text(mTail, mText + head);
    }
    mRow.layout(kFigureW / 2);
}

void BigFigure::setFont(Theme::Font font)
{
    lv_obj_set_style_text_font(mHead, Theme::font(font), LV_PART_MAIN);
    lv_obj_set_style_text_font(mTail, Theme::font(font), LV_PART_MAIN);
    place();
    mRow.layout(kFigureW / 2);
}

void BigFigure::setColor(uint32_t color)
{
    lv_obj_set_style_text_color(mHead, Theme::rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_color(mTail, Theme::rgb(color), LV_PART_MAIN);
}

// -----------------------------------------------------------------------------

namespace
{
/// Place @p obj just above @p ref among their parent's children, so a part
/// made later still draws under whatever the screen built after the ring.
lv_obj_t* above(lv_obj_t* obj, lv_obj_t* ref)
{
    lv_obj_move_to_index(obj, static_cast<int32_t>(lv_obj_get_index(ref)) + 1);
    return obj;
}

void hide(lv_obj_t* obj, bool hidden)
{
    if (obj != nullptr) {
        Theme::setHidden(obj, hidden);
    }
}
} // namespace

TrackRing::TrackRing(lv_obj_t* parent) : mParent(parent)
{
    // Back to front: the band (tartan, or the road's thin ring), progress,
    // the lane line, the start line, the calibration quarters, the dot. The
    // idle ring (tartan, lane line, start line) is made now; the other parts
    // when a state first needs them (a menu's ring is three LVGL
    // objects, not twelve, in a pool where the summary is tight).
    mTartan    = ringArc(parent, kRingRadius, kRingWidth, 0, 360, kTartan);
    // 2 px: at 1 px LVGL's antialiasing leaves too little of the line for
    // the panel's 2 bits a channel, and it breaks up into dots.
    mLane      = ringArc(parent, kRingRadius, kLaneWidth, 0, 360, kLaneOnTartan);
    mStartLine = Theme::box(parent, kRingCx - 1, kStartLineY, kStartLineW, kStartLineH, kText);
    applyMode(Mode::Idle);
}


void TrackRing::make(Mode mode)
{
    if (mode == Mode::Lap && mProgress == nullptr) {
        mProgress = above(ringArc(mParent, kRingRadius, kRingWidth, 0, 0, kTeal), mTartan);
        mLaneTeal = above(ringArc(mParent, kRingRadius, kLaneWidth, 0, 0, kLaneOnTeal), mProgress);
    }
    if (mode == Mode::Road && mRoad == nullptr) {
        mRoad     = above(ringArc(mParent, kRoadRadius, kRoadWidth, 0, 360, kMuted), mTartan);
        mRoadProg = above(ringArc(mParent, kRoadRadius, kRoadWidth, 0, 0, kTeal), mRoad);
    }
    if (mode == Mode::Calibrating && mCal[0] == nullptr) {
        lv_obj_t* ref = mStartLine;
        for (int i = 0; i < 4; ++i) {
            mCal[i] = above(ringArc(mParent, kRingRadius, kRingWidth, i * 90 + kCalGapDeg, (i + 1) * 90 - kCalGapDeg,
                                    kMuted), ref);
            ref = mCal[i];
        }
    }
    if (mode != Mode::Idle && mDot == nullptr) {
        mDot = Theme::box(mParent, 0, 0, kDotSize, kDotSize, kText, LV_RADIUS_CIRCLE);
        lv_obj_set_style_border_width(mDot, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(mDot, Theme::rgb(kBackground), LV_PART_MAIN);
        above(mDot, mCal[3] != nullptr ? mCal[3] : mStartLine);
        mDotX = mDotY = -1000;
    }
}

void TrackRing::setHidden(bool hidden)
{
    lv_obj_t* all[] = {mTartan, mRoad, mProgress, mRoadProg, mLaneTeal, mLane, mStartLine,
                       mCal[0], mCal[1], mCal[2], mCal[3], mDot};
    if (hidden) {
        for (lv_obj_t* o : all) {
            hide(o, true);
        }
        mHaveMode = false;
    } else {
        applyMode(mMode);
    }
}

void TrackRing::applyMode(Mode mode)
{
    make(mode);
    const bool lap  = mode == Mode::Lap;
    const bool road = mode == Mode::Road;
    const bool cal  = mode == Mode::Calibrating;
    const bool idle = mode == Mode::Idle;
    hide(mTartan, !(lap || idle));
    hide(mProgress, !lap);
    hide(mLaneTeal, !lap);
    hide(mLane, !(lap || idle));
    hide(mStartLine, !(lap || idle));
    hide(mRoad, !road);
    hide(mRoadProg, !road);
    for (lv_obj_t* q : mCal) {
        hide(q, !cal);
    }
    hide(mDot, idle);
    if (idle) {
        Theme::setArc(mLane, 0, 360);   // whole again after a lap (which shortens it)
    }
    mMode     = mode;
    mHaveMode = true;
    mDeg      = -1;        // redraw the progress for the new mode
    mCalLaps  = 0xFF;
}

void TrackRing::set(Mode mode, uint16_t permille, uint8_t calLaps)
{
    if (!mHaveMode || mode != mMode) {
        applyMode(mode);
    }
    if (mode == Mode::Idle) {
        return;
    }
    if (mode == Mode::Calibrating && calLaps != mCalLaps) {
        mCalLaps = calLaps;
        for (uint8_t i = 0; i < 4; ++i) {
            Theme::setArcColor(mCal[i], i < calLaps ? kAccent : kMuted);
        }
    }
    const int32_t deg = static_cast<int32_t>(permille > 1000u ? 1000u : permille) * 360 / 1000;
    if (deg != mDeg) {
        mDeg = deg;
        if (mode == Mode::Lap) {
            Theme::setArc(mProgress, 0, deg);
            Theme::setArc(mLaneTeal, 0, deg);
            Theme::setArc(mLane, deg, 360);
        } else if (mode == Mode::Road) {
            Theme::setArc(mRoadProg, 0, deg);
        }
        placeDot(deg);
    }
}

void TrackRing::placeDot(int32_t deg)
{
    // 0 at 12 o'clock, clockwise, on the band's centre line.
    const int16_t a  = static_cast<int16_t>(deg % 360);
    const int32_t dx = (kRingRadius * lv_trigo_sin(a) + (1 << (LV_TRIGO_SHIFT - 1))) >> LV_TRIGO_SHIFT;
    const int32_t dy = (kRingRadius * lv_trigo_cos(a) + (1 << (LV_TRIGO_SHIFT - 1))) >> LV_TRIGO_SHIFT;
    const int32_t x  = kRingCx + dx - kDotSize / 2;
    const int32_t y  = kRingCy - dy - kDotSize / 2;
    if (x != mDotX || y != mDotY) {
        mDotX = x;
        mDotY = y;
        lv_obj_set_pos(mDot, x, y);
    }
}

// -----------------------------------------------------------------------------

LaneBadge::LaneBadge(lv_obj_t* parent, int32_t x, int32_t y, bool large)
{
    const int32_t w = large ? kLargeW : kSmallW;
    const int32_t h = large ? kLargeH : kSmallH;
    // The lightest coral with a black number (tartan with white reads dark
    // on the panel), white lane lines left and right.
    mBox = Theme::box(parent, x, y, w, h, kLaneFill, 6);
    lv_obj_set_style_border_width(mBox, large ? 2 : 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(mBox, static_cast<lv_border_side_t>(LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT),
                                 LV_PART_MAIN);
    lv_obj_set_style_border_color(mBox, Theme::rgb(kText), LV_PART_MAIN);
    // The figure in SemiBold 20 small, SemiBold 25 large.
    mLabel = large ? Theme::label(mBox, kAction, "", -2, 5, w, LV_TEXT_ALIGN_CENTER, kOnAccent)
                   : Theme::label(mBox, kChoice, "", -1, 1, w, LV_TEXT_ALIGN_CENTER, kOnAccent);
}

void LaneBadge::set(uint8_t lane)
{
    if (lane != mLane) {
        mLane = lane;
        lv_label_set_text_fmt(mLabel, "%u", static_cast<unsigned>(lane));
    }
}

} // namespace Kit
