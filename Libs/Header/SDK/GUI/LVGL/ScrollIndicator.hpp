/**
 ******************************************************************************
 * @file    ScrollIndicator.hpp
 * @brief   Arc position indicator on the left bezel: a rail and a sliding
 *          handle, as the UNA activity apps show beside a menu or a set of
 *          swipeable faces.
 *
 * Teardown in either order: destroying the indicator leaves its arcs to the
 * parent; deleting the parent first stops a running slide (the indicator
 * watches LV_EVENT_DELETE) and turns later calls into no-ops.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_SCROLL_INDICATOR_HPP
#define SDK_GUI_LVGL_SCROLL_INDICATOR_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

class ScrollIndicator
{
public:
    struct Config {
        float handleLen;   ///< handle span, degrees
        float railMin;     ///< rail low bound (TouchGFX angle, 0 = 12 o'clock)
        float railMax;     ///< rail high bound
    };
    /// The rail beside a menu wheel.
    static constexpr Config kBig   = { 18.0f, 232.0f, 308.0f };
    /// The shorter rail beside a set of faces.
    static constexpr Config kSmall = { 10.0f, 252.0f, 288.0f };

    explicit ScrollIndicator(lv_obj_t* parent, const Config& cfg = kBig);
    ~ScrollIndicator();

    ScrollIndicator(const ScrollIndicator&)            = delete;
    ScrollIndicator& operator=(const ScrollIndicator&) = delete;

    void setConfig(const Config& cfg);
    void setCount(uint16_t count);      ///< resets to position 0; hidden when <= 1
    void setActive(uint16_t index);     ///< instant
    /**
     * @brief Slide the handle to an item over @p ms.
     * @param direction  +1 when the list moved to the next item, -1 to the
     *        previous, 0 unknown. With it, a move between the two ends of the
     *        rail is a wrap: the handle slides off one end while a second
     *        handle slides in from the other.
     */
    void animateTo(uint16_t index, uint32_t ms, int direction = 0);
    uint16_t active() const { return mPos; }

private:
    static void animExecCb(void* var, int32_t value);
    static void animDoneCb(lv_anim_t* a);
    static void deleteCb(lv_event_t* e);
    float startAngle(uint16_t index) const;
    void  setHandle(float startDeg);
    void  setClampedArc(lv_obj_t* arc, float startDeg);
    void  update();

    lv_obj_t* mRail      = nullptr;
    lv_obj_t* mHandle    = nullptr;
    lv_obj_t* mHandleOvf = nullptr;   ///< the incoming handle during a wrap
    Config    mCfg;
    uint16_t  mCount     = 1;
    uint16_t  mPos       = 0;

    // Running slide, angles in degrees.
    float mAnimFrom = 0, mAnimTo = 0, mAnimOutEnd = 0, mAnimInStart = 0;
    bool  mAnimWrap = false;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_SCROLL_INDICATOR_HPP
