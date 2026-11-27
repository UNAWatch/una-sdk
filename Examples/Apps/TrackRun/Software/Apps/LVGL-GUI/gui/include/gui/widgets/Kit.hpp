/**
 ******************************************************************************
 * @file    Kit.hpp
 * @brief   Track Run's kit components: the track ring, the lane badge and
 *          the divider. LVGL drawing only (arcs, rounded rectangles), no
 *          bitmaps. The menus use RunLVGL's title.
 ******************************************************************************
 */

#ifndef TRACKRUN_KIT_WIDGETS_HPP
#define TRACKRUN_KIT_WIDGETS_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/theme/Kit.hpp"

namespace Kit
{

/// A label sized to its text (for rows laid out by Row).
lv_obj_t* text(lv_obj_t* parent, Theme::Font f, const char* s, uint32_t color);

/// The kit's divider: Teal, 150 x 2, centred.
lv_obj_t* divider(lv_obj_t* parent, int32_t y);

/// Places objects left to right with gaps, the group centred on x = 120. Each
/// keeps its own y (set when it was made). Call after any text changes; it
/// moves only what moved.
class Row
{
public:
    static constexpr uint8_t kMax = 6;
    static constexpr int32_t kInkSlack = 2;   ///< A label's box past its advance
    void add(lv_obj_t* obj, int32_t gapBefore = 0);
    void layout(int32_t centreX = 120);
    /// The group's width at the last layout().
    int32_t width() const { return mWidth; }

private:
    int32_t   mWidth     = 0;
    lv_obj_t* mObj[kMax] = {};
    int32_t   mGap[kMax] = {};
    int32_t   mX[kMax]   = {};
    uint8_t   mCount     = 0;
};

// -----------------------------------------------------------------------------
/// A large figure that changes every second (the lap clock, metres to go),
/// centred on the screen. Drawing cost: LVGL gives a label a margin of a
/// quarter of its line height all round for overhanging glyphs, and redraws
/// all of it on a change (about 172 x 109 px for "0:33" in the 60 px face).
/// Here the figure sits in a box that clips to its glyphs' rows, and its
/// last two characters are a label of their own, so a second's change
/// redraws only those two digits.
class BigFigure
{
public:
    BigFigure(lv_obj_t* parent, int32_t baselineY, Theme::Font font, uint32_t color);
    void set(const char* text);
    /// Another face, on the same baseline (the clock's 40 px face from an hour).
    void setFont(Theme::Font font);
    void setColor(uint32_t color);

private:
    void place();

    lv_obj_t* mBox  = nullptr;
    lv_obj_t* mHead = nullptr;
    lv_obj_t* mTail = nullptr;
    Row       mRow;
    int32_t   mBaseline = 0;
    char      mText[16] = {};
};

// -----------------------------------------------------------------------------
/// The bezel as a running track. Its progress shows the
/// share of the lap (or road split) given to set(); the dot sits at its end.
/// Only what changes is redrawn: the arc's new wedge and the dot's two boxes.
class TrackRing
{
public:
    enum class Mode : uint8_t { Lap, Calibrating, Road, Idle };

    explicit TrackRing(lv_obj_t* parent);

    /// permille: 0 to 1000 of the lap or road split (held by the caller so it
    /// never runs back, Ui::RingProgress); calLaps 0 to 4 in Calibrating.
    void set(Mode mode, uint16_t permille, uint8_t calLaps = 0);

    void setHidden(bool hidden);

private:
    void applyMode(Mode mode);
    void make(Mode mode);
    void placeDot(int32_t deg);

    lv_obj_t* mParent    = nullptr;

    lv_obj_t* mTartan    = nullptr;
    lv_obj_t* mRoad      = nullptr;
    lv_obj_t* mProgress  = nullptr;
    lv_obj_t* mRoadProg  = nullptr;
    lv_obj_t* mLaneTeal  = nullptr;   ///< The lane line over the progress
    lv_obj_t* mLane      = nullptr;   ///< and over the rest of the band
    lv_obj_t* mStartLine = nullptr;
    lv_obj_t* mCal[4]    = {};
    lv_obj_t* mDot       = nullptr;

    Mode    mMode    = Mode::Idle;
    bool    mHaveMode = false;
    int32_t mDeg     = -1;            ///< The progress shown, whole degrees
    int32_t mDotX    = -1000;
    int32_t mDotY    = -1000;
    uint8_t mCalLaps = 0xFF;
};

// -----------------------------------------------------------------------------
/// The lane number on a track: black on the lightest coral, white lines
/// left and right. Small: 20 x 24 for the run screen; large: 36 x 42 beside
/// a lane picker's selected lane.
class LaneBadge
{
public:
    static constexpr int32_t kSmallW = 20;
    static constexpr int32_t kSmallH = 24;
    static constexpr int32_t kLargeW = 36;
    static constexpr int32_t kLargeH = 42;

    LaneBadge(lv_obj_t* parent, int32_t x, int32_t y, bool large = false);
    void set(uint8_t lane);
    lv_obj_t* obj() const { return mBox; }

private:
    lv_obj_t* mBox   = nullptr;
    lv_obj_t* mLabel = nullptr;
    uint8_t   mLane  = 0;
};

} // namespace Kit

#endif // TRACKRUN_KIT_WIDGETS_HPP
