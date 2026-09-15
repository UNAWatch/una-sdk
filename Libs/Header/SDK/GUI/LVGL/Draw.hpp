/**
 ******************************************************************************
 * @file    Draw.hpp
 * @brief   Drawing helpers for an LVGL GUI process on the watch.
 *
 * The SDK's lv_conf.h builds LVGL without a stock theme, so objects start
 * unstyled and a screen composes its look from these helpers. Coordinates are
 * the same as a TouchGFX Designer's (origin top left, 240 x 240) and arc
 * angles are TouchGFX's (0 = 12 o'clock, clockwise), so an LVGL GUI can be
 * laid out from the same values as a TouchGFX one. Colours are the SDK's
 * 64-colour palette (SDK/GUI/Color.hpp), which the two-bits-per-channel
 * display renders exactly.
 *
 * Every helper creates its objects on the parent it is given, and the parent
 * owns them: deleting the parent deletes them. The widget classes beside this
 * file are C++ objects over such LVGL objects, and either may go first:
 * destroying the widget leaves its objects to the parent, and deleting the
 * parent first is noticed by the widgets that run a timer or an animation
 * (SensorStatusRow, WheelMenu, ScrollIndicator), which stop it and ignore
 * later calls. Destroying the widget before its parent is the natural order
 * and what the in-tree apps do.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_DRAW_HPP
#define SDK_GUI_LVGL_DRAW_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"

namespace SDK::LVGL
{

/// Screen centre, where every bezel arc is centred.
inline constexpr int32_t kCx = 120;
inline constexpr int32_t kCy = 120;

namespace Draw
{

/// SDK palette value (0xRRGGBB) to an LVGL colour.
inline lv_color_t rgb(uint32_t c) { return lv_color_hex(c); }

/// Convert a TouchGFX arc angle (0 = 12 o'clock, clockwise) to LVGL's
/// (0 = 3 o'clock, clockwise), folded into 0..359 for any input.
inline int32_t arcAngle(int32_t touchgfxDeg)
{
    const int32_t a = (touchgfxDeg + 270) % 360;
    return a < 0 ? a + 360 : a;
}

/// Build the shared styles. Call once after lv_init(), before any screen.
void init();

/// Black full-screen background with no padding, border or scrollbars.
void applyScreen(lv_obj_t* screen);

/// Show or hide an object.
void setHidden(lv_obj_t* obj, bool hidden);

/// Plain container: transparent, no padding/border, clips its children.
lv_obj_t* container(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

/// Single-line label in a text box, like a TouchGFX TextArea: the box is
/// positioned at (x, y) with width w, and the text is aligned within it.
lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, const char* text,
                int32_t x, int32_t y, int32_t w,
                lv_text_align_t align = LV_TEXT_ALIGN_CENTER,
                uint32_t color = SDK::GUI::Color::WHITE);

/// Horizontal 3 px divider with rounded ends.
lv_obj_t* hline(lv_obj_t* parent, int32_t x, int32_t y, int32_t w,
                uint32_t color = SDK::GUI::Color::TEAL);

/// Vertical 3 px divider with rounded ends.
lv_obj_t* vline(lv_obj_t* parent, int32_t x, int32_t y, int32_t h,
                uint32_t color = SDK::GUI::Color::TEAL);

/// Filled rectangle with rounded corners (LV_RADIUS_CIRCLE for a pill).
lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h,
              uint32_t color, int32_t radius = 0);

/// Static image at (x, y).
lv_obj_t* image(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y);

/// Alpha-only (A8) icon at (x, y) drawn in @p color. Single-colour icons are
/// stored without colour, one byte per pixel, and tinted here.
lv_obj_t* imageTinted(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y, uint32_t color);

/// Change the tint of an image made by imageTinted().
void tint(lv_obj_t* img, uint32_t color);

/// Filled circle of the given radius centred at (cx, cy).
lv_obj_t* dot(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, uint32_t color);

/// A static arc segment. Angles are TouchGFX-style (0 = 12 o'clock, clockwise)
/// and may lie outside 0..359; a span of 360 or more is the full ring. @p radius
/// is the arc's centre-line radius as in touchgfx::Circle. Ends are rounded
/// unless @p rounded is false, which cuts them radially.
lv_obj_t* arc(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, int32_t width,
              int32_t startDeg, int32_t endDeg, uint32_t color, bool rounded = true);

/// Re-aim an arc made by arc() (TouchGFX-style angles).
void setArc(lv_obj_t* arcObj, int32_t startDeg, int32_t endDeg);

void setArcColor(lv_obj_t* arcObj, uint32_t color);

} // namespace Draw
} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_DRAW_HPP
