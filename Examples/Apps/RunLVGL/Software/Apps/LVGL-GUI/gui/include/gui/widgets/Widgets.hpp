/**
 ******************************************************************************
 * @file    Widgets.hpp
 * @brief   The Run app's widgets, rebuilt from LVGL primitives.
 *
 * The widgets every UNA activity app shares (button hints, title, scroll
 * indicator, sensor row, battery, timer ring, toggle, wheel menu) come from
 * the SDK (SDK/GUI/LVGL/) and appear here under the Widgets namespace, with
 * Run's font and icons filled in. The rest are Run's own.
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

#include "SDK/GUI/LVGL/Battery.hpp"
#include "SDK/GUI/LVGL/Buttons.hpp"
#include "SDK/GUI/LVGL/ScrollIndicator.hpp"
#include "SDK/GUI/LVGL/SensorStatusRow.hpp"
#include "SDK/GUI/LVGL/TimerRing.hpp"
#include "SDK/GUI/LVGL/Title.hpp"
#include "SDK/GUI/LVGL/Toggle.hpp"
#include "SDK/TrackMap/TrackMapScreen.hpp"
#include "Track.hpp"
#include "gui/theme/Theme.hpp"

namespace Widgets
{

/// Screen centre, where every bezel arc is centred.
using SDK::LVGL::kCx;
using SDK::LVGL::kCy;

// The SDK's activity-app widgets, as they are.
using Buttons         = SDK::LVGL::Buttons;
using ScrollIndicator = SDK::LVGL::ScrollIndicator;
using Battery         = SDK::LVGL::Battery;
using TimerRing       = SDK::LVGL::TimerRing;
using Toggle          = SDK::LVGL::Toggle;

// -----------------------------------------------------------------------------
/// The SDK title in Run's Italic 18.
class Title : public SDK::LVGL::Title
{
public:
    Title(lv_obj_t* parent, const char* text)
        : SDK::LVGL::Title(parent, Theme::font(Theme::Font::Italic18), text)
    {
    }
};

// -----------------------------------------------------------------------------
/// The SDK sensor row with Run's GPS and heart icons.
class SensorStatusRow : public SDK::LVGL::SensorStatusRow
{
public:
    SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);
};

// -----------------------------------------------------------------------------
/// Five-zone heart-rate arc; lights the zone the current HR falls in.
class HeartRateZone
{
public:
    static constexpr uint8_t kZoneCount = 5;
    HeartRateZone(lv_obj_t* parent, int32_t x, int32_t y);
    void setHR(float bpm, const uint8_t* thresholds, uint8_t thresholdCount);

    /// The arrow under the active zone, drawn as a triangle by an LV_EVENT_DRAW_MAIN
    /// handler on mArrow. Points are relative to the widget origin.
    struct Arrow {
        lv_point_precise_t p[3] = {};
        lv_color_t         color = {};
    };

private:
    void showZone(int zone);

    int32_t   mX = 0;
    int32_t   mY = 0;
    lv_obj_t* mMarker = nullptr;   ///< thick arc over the active segment
    lv_obj_t* mArrow  = nullptr;   ///< host object for the triangle
    Arrow     mArrowDsc;
    int       mActive = -1;
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
