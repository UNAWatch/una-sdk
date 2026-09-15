/**
 ******************************************************************************
 * @file    Buttons.hpp
 * @brief   Bezel arcs showing which of the watch's four buttons do something.
 *
 * The arcs sit where the buttons are (L1 top left, L2 bottom left, R1 top
 * right, R2 bottom right) at the positions the UNA activity apps use, in the
 * colours those apps give each action: white to go back, amber to select or
 * start, red to discard, green to confirm.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_BUTTONS_HPP
#define SDK_GUI_LVGL_BUTTONS_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

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

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_BUTTONS_HPP
