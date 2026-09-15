/**
 ******************************************************************************
 * @file    SensorStatusRow.hpp
 * @brief   Centred row of sensor icons (GPS, external heart rate); an icon
 *          whose sensor is still searching blinks.
 *
 * The LVGL counterpart of SDK::GUI::SensorStatusRow. The app supplies its
 * icons as A8 alpha masks (see Draw::imageTinted): the row tints them white
 * when connected and alternates white and dark grey every 500 ms while
 * searching, so one bitmap per sensor serves both phases. Pass nullptr for a
 * sensor the app does not show.
 *
 * Teardown in either order: destroying the row leaves its objects to the
 * parent; deleting the parent first stops the blink (the row watches
 * LV_EVENT_DELETE) and turns later setGps()/setHr() calls into no-ops.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_SENSOR_STATUS_ROW_HPP
#define SDK_GUI_LVGL_SENSOR_STATUS_ROW_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/SensorLayer/DataParsers/SensorDataParserHeartRateEx.hpp"

namespace SDK::LVGL
{

class SensorStatusRow
{
public:
    enum class State : uint8_t { Absent, Searching, Connected };

    /// A GPS activity is acquiring until it has a fix, so it is never Absent here.
    static State gpsState(bool fix) { return fix ? State::Connected : State::Searching; }

    /// Map an SDK::Accessory::State value to the heart icon's state:
    /// UNAVAILABLE(0)/IDLE(1) -> Absent, SEARCHING(2)/CONNECTING(3)/LOST(5) ->
    /// Searching, CONNECTED(4) -> Connected.
    static State hrState(uint8_t accessoryState)
    {
        switch (accessoryState) {
            case 2: case 3: case 5: return State::Searching;
            case 4:                 return State::Connected;
            default:                return State::Absent;
        }
    }

    /// In-activity variant: steady only while the link is CONNECTED and the
    /// strap is the live heart-rate source (SDK::SensorDataParser::HeartRateEx::Source);
    /// anything else while external HR is engaged animates as searching.
    static State hrStateFromSource(uint8_t accessoryState, uint8_t hrSource)
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

    /**
     * @param x, y, w, h  The row's box; icons are bottom-aligned and centred
     *                    in it, as a pair or singly.
     * @param gpsIcon     A8 GPS glyph, or nullptr.
     * @param hrIcon      A8 heart glyph, or nullptr.
     */
    SensorStatusRow(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
                    const lv_image_dsc_t* gpsIcon, const lv_image_dsc_t* hrIcon);
    ~SensorStatusRow();

    SensorStatusRow(const SensorStatusRow&)            = delete;
    SensorStatusRow& operator=(const SensorStatusRow&) = delete;

    void setGps(State s);
    void setHr(State s);

private:
    static void tickCb(lv_timer_t* t);
    static void deleteCb(lv_event_t* e);
    void update();
    void layout();
    void applyIcons();

    lv_obj_t*   mRow   = nullptr;
    lv_obj_t*   mGps   = nullptr;
    lv_obj_t*   mHr    = nullptr;
    lv_timer_t* mTimer = nullptr;
    const lv_image_dsc_t* mGpsIcon = nullptr;
    const lv_image_dsc_t* mHrIcon  = nullptr;
    State       mGpsState = State::Absent;
    State       mHrState  = State::Absent;
    bool        mLight    = true;
    int32_t     mW = 0, mH = 0;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_SENSOR_STATUS_ROW_HPP
