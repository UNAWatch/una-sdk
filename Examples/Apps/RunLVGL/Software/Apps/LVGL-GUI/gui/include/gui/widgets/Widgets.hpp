/**
 ******************************************************************************
 * @file    Widgets.hpp
 * @brief   The Run app's shared widgets, rebuilt from LVGL primitives.
 *
 * Each class wraps the LVGL objects it creates on a parent and exposes the
 * same setters its TouchGFX container has. Geometry (positions, radii, arc
 * angles) is copied from the TouchGFX Designer output so the screens match.
 * Objects are owned by the LVGL parent; deleting the parent deletes them.
 * Classes that run an lv_timer stop it in their destructor.
 ******************************************************************************
 */

#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include <cstdint>
#include <ctime>
#include <memory>
#include <vector>

#include "lvgl.h"

#include "SDK/TrackMap/TrackMapScreen.hpp"
#include "Track.hpp"
#include "gui/theme/Theme.hpp"

namespace Widgets
{

/// Screen centre, where every bezel arc is centred.
inline constexpr int32_t kCx = 120;
inline constexpr int32_t kCy = 120;

// -----------------------------------------------------------------------------
/// Four arc hints on the bezel showing which physical buttons do something.
class Buttons
{
public:
    enum Color : uint8_t { NONE = 0, WHITE, AMBER, RED, GREEN };

    explicit Buttons(lv_obj_t* parent);

    void set(Color l1, Color l2, Color r1, Color r2);
    void setL1(Color c) { apply(0, c); }
    void setL2(Color c) { apply(1, c); }
    void setR1(Color c) { apply(2, c); }
    void setR2(Color c) { apply(3, c); }

private:
    void apply(int idx, Color c);
    lv_obj_t* mArc[4] = {};
};

// -----------------------------------------------------------------------------
/// Arc position indicator on the left bezel: a rail and a sliding handle.
class ScrollIndicator
{
public:
    struct Config {
        float handleLen;   ///< handle span, degrees
        float railMin;     ///< rail low bound (TouchGFX angle)
        float railMax;     ///< rail high bound (TouchGFX angle)
    };
    static constexpr Config kBig   = { 18.0f, 232.0f, 308.0f };
    static constexpr Config kSmall = { 10.0f, 252.0f, 288.0f };

    explicit ScrollIndicator(lv_obj_t* parent, const Config& cfg = kBig);
    ~ScrollIndicator();

    ScrollIndicator(const ScrollIndicator&)            = delete;
    ScrollIndicator& operator=(const ScrollIndicator&) = delete;

    void setConfig(const Config& cfg);
    void setCount(uint16_t count);      ///< resets to position 0; hidden when <= 1
    void setActive(uint16_t index);     ///< instant
    /// Slide the handle to an item over @p ms. A wrap between the ends jumps.
    void animateTo(uint16_t index, uint32_t ms);
    uint16_t active() const { return mPos; }

private:
    static void animExecCb(void* var, int32_t value);
    float startAngle(uint16_t index) const;
    void  setHandle(float startDeg);
    void  update();

    lv_obj_t* mRail   = nullptr;
    lv_obj_t* mHandle = nullptr;
    Config    mCfg;
    uint16_t  mCount  = 1;
    uint16_t  mPos    = 0;
};

// -----------------------------------------------------------------------------
/// Screen title at the top: italic text over a short grey rule.
class Title
{
public:
    Title(lv_obj_t* parent, const char* text);
    void setText(const char* text);
    void setColor(uint32_t color);
    void setVisible(bool visible);

private:
    lv_obj_t* mLine  = nullptr;
    lv_obj_t* mLabel = nullptr;
};

// -----------------------------------------------------------------------------
/// Four-segment battery gauge, 90 x 28.
class Battery
{
public:
    Battery(lv_obj_t* parent, int32_t x, int32_t y);
    void setLevel(uint8_t percent);

private:
    lv_obj_t* mSeg[4] = {};
    lv_obj_t* mNub    = nullptr;
};

// -----------------------------------------------------------------------------
/// Centred row of sensor icons (GPS, external HR); a searching icon blinks.
class SensorStatusRow
{
public:
    enum class State : uint8_t { Absent, Searching, Connected };

    static State gpsState(bool fix) { return fix ? State::Connected : State::Searching; }
    static State hrState(uint8_t accessoryState);
    static State hrStateFromSource(uint8_t accessoryState, uint8_t hrSource);

    SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);
    ~SensorStatusRow();

    void setGps(State s);
    void setHr(State s);

private:
    static void tickCb(lv_timer_t* t);
    void update();
    void layout();
    void applyIcons();

    lv_obj_t*   mRow   = nullptr;
    lv_obj_t*   mGps   = nullptr;
    lv_obj_t*   mHr    = nullptr;
    lv_timer_t* mTimer = nullptr;
    State       mGpsState = State::Absent;
    State       mHrState  = State::Absent;
    bool        mLight    = true;
    int32_t     mW = 0, mH = 0;
};

// -----------------------------------------------------------------------------
/// Five-zone heart-rate arc; lights the zone the current HR falls in.
class HeartRateZone
{
public:
    static constexpr uint8_t kZoneCount = 5;
    HeartRateZone(lv_obj_t* parent, int32_t x, int32_t y);
    void setHR(float bpm, const uint8_t* thresholds, uint8_t thresholdCount);

private:
    lv_obj_t* mZones[kZoneCount] = {};
};

// -----------------------------------------------------------------------------
/// Grey dome at the bottom with a pause glyph and the paused session time.
class PauseIndicator
{
public:
    PauseIndicator(lv_obj_t* parent, int32_t y);
    void setTime(std::time_t seconds);

private:
    lv_obj_t* mLabel = nullptr;
};

// -----------------------------------------------------------------------------
/// Title + value panel that cycles through a set of slots on a timer.
class InfoCarousel
{
public:
    using UpdateCb = void (*)(void* user, int16_t index);

    InfoCarousel(lv_obj_t* parent, int32_t x, int32_t y);
    ~InfoCarousel();

    void setPeriodMs(uint32_t ms);
    void setCallback(UpdateCb cb, void* user);
    void setCount(uint16_t count);   ///< resets to slot 0 and fires the callback
    void refresh();                  ///< re-fires the callback for the current slot

    void setTitle(const char* text);
    void setValue(const char* text); ///< nullptr hides the value

private:
    static void tickCb(lv_timer_t* t);
    void fire();

    lv_obj_t*   mTitle = nullptr;
    lv_obj_t*   mValue = nullptr;
    lv_timer_t* mTimer = nullptr;
    UpdateCb    mCb    = nullptr;
    void*       mUser  = nullptr;
    uint16_t    mCount = 0;
    uint16_t    mIndex = 0;
    uint32_t    mPeriodMs = 3000;
};

// -----------------------------------------------------------------------------
/// Progress ring around the face: grey track, coloured fill from the start.
class TimerRing
{
public:
    explicit TimerRing(lv_obj_t* parent);
    void setColor(uint32_t color);
    /// FILL: the arc grows from the track's start; @p permille (0..1000) is how much.
    void setProgress(uint32_t permille);
    /// DRAIN: the arc shrinks towards the track's end; @p permille is how much is left.
    void setRemaining(uint32_t permille);

private:
    lv_obj_t* mTrack    = nullptr;
    lv_obj_t* mProgress = nullptr;
};

// -----------------------------------------------------------------------------
/// Two-state switch, 60 x 30: a pill rail (black off, amber on) and a light knob.
class Toggle
{
public:
    Toggle(lv_obj_t* parent, int32_t x, int32_t y);
    void setState(bool on);
    bool state() const { return mOn; }
    void setVisible(bool visible);

private:
    lv_obj_t* mRail   = nullptr;
    lv_obj_t* mHandle = nullptr;
    int32_t   mX = 0, mY = 0;
    bool      mOn = false;
};

// -----------------------------------------------------------------------------
/// Interval phase readout, 190 x 91: big MM:SS or distance, a description line
/// and a divider, all in the phase's accent colour.
class IntervalsTimer
{
public:
    IntervalsTimer(lv_obj_t* parent, int32_t x, int32_t y);

    /// MM:SS (clamped to 99:59) with "Open" / "Remaining" / "Elapsed" beneath.
    void setPhaseTime(std::time_t sec, Track::IntervalsMetric metric);
    /// Distance left with "km remaining" / "mi remaining" beneath.
    void setPhaseDistance(float distInUnits, bool imperial);
    /// MM:SS only (alert screens).
    void setRemainingTime(std::time_t sec);
    /// "Open" as the main readout (alert screens).
    void setOpen();

    void setColor(uint32_t color);
    void setLineVisible(bool visible);
    void setDescriptionVisible(bool visible);

private:
    void setTimerClamped(std::time_t sec);
    void setDescription(const char* text);   ///< nullptr hides it

    lv_obj_t* mTimer       = nullptr;
    lv_obj_t* mDescription = nullptr;
    lv_obj_t* mLine        = nullptr;
};

// -----------------------------------------------------------------------------
/// Two-stage "two-tone" value picker (whole.fraction or minutes:seconds): the
/// component being edited is teal SemiBold 60, the other grey Light 60, with the
/// next two values of the active component listed beneath it.
class TwoTonePicker
{
public:
    explicit TwoTonePicker(lv_obj_t* parent);

    void setTitle(const char* title);
    /// One centred teal subtitle (distance pickers).
    void renderSubtitleSingle(const char* label);
    /// Two subtitles, one over each column; the active one teal (time picker).
    void renderSubtitleDual(const char* left, const char* right, bool leftActive);
    /// The composite value plus the two upcoming values of the active component.
    void renderValue(bool leftActive, const char* left, const char* right, const char* sep,
                     const char* up1, const char* up2);

private:
    std::unique_ptr<Title>   mTitle;
    std::unique_ptr<Buttons> mButtons;
    lv_obj_t* mSubLeft  = nullptr;
    lv_obj_t* mSubRight = nullptr;
    lv_obj_t* mValLeft  = nullptr;
    lv_obj_t* mValSep   = nullptr;
    lv_obj_t* mValRight = nullptr;
    lv_obj_t* mNext1    = nullptr;
    lv_obj_t* mNext2    = nullptr;
};

// -----------------------------------------------------------------------------
/// The recorded route as a polyline with start (green) and end (red) markers.
class Map
{
public:
    Map(lv_obj_t* parent, int32_t x, int32_t y);
    void setMap(const SDK::TrackMapScreen& map);

private:
    lv_obj_t* mRoot  = nullptr;
    lv_obj_t* mLine  = nullptr;
    lv_obj_t* mStart = nullptr;
    lv_obj_t* mEnd   = nullptr;
    std::vector<lv_point_precise_t> mPoints;
};

} // namespace Widgets

#endif // WIDGETS_HPP
