/**
 ******************************************************************************
 * @file    Theme.hpp
 * @brief   Palette, fonts and drawing helpers shared by every RunLVGL screen.
 *
 * LVGL is built without a stock theme (lv_conf.h), so widgets start unstyled
 * and the screens compose their look from these helpers. Coordinates given to
 * the helpers are the TouchGFX Run app's, so the two apps lay out identically.
 *
 * Colours come from the SDK's 64-colour palette (SDK/GUI/Color.hpp): the
 * display keeps two bits per channel, so those values render exactly.
 ******************************************************************************
 */

#ifndef THEME_HPP
#define THEME_HPP

#include <cstdint>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"

namespace Theme
{

/// The Poppins faces the Run app uses, by weight and pixel size.
enum class Font : uint8_t {
    Italic18,
    Italic20,
    Light60,
    Medium18,
    Medium25,
    Medium40,
    Regular14,
    Regular16,
    Regular18,
    SemiBold20,
    SemiBold25,
    SemiBold30,
    SemiBold35,
    SemiBold40,
    SemiBold60,
};

const lv_font_t* font(Font f);

/// SDK palette value (0xRRGGBB) to an LVGL colour.
inline lv_color_t rgb(uint32_t c) { return lv_color_hex(c); }

/// Convert a TouchGFX arc angle (0 = 12 o'clock, clockwise) to LVGL's
/// (0 = 3 o'clock, clockwise).
inline int32_t arcAngle(int32_t touchgfxDeg) { return (touchgfxDeg + 270) % 360; }

/// Build the shared styles. Call once after lv_init(), before any screen.
void init();

/// Black full-screen background with no padding, border or scrollbars.
void applyScreen(lv_obj_t* screen);

/// Plain container: transparent, no padding/border, clips its children.
lv_obj_t* container(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h);

/// Single-line label in a text box, like a TouchGFX TextArea: the box is
/// positioned at (x, y) with width w, and the text is aligned within it.
lv_obj_t* label(lv_obj_t* parent, Font f, const char* text,
                int32_t x, int32_t y, int32_t w,
                lv_text_align_t align = LV_TEXT_ALIGN_CENTER,
                uint32_t color = SDK::GUI::Color::WHITE);

/// Horizontal 3 px divider with rounded ends.
lv_obj_t* hline(lv_obj_t* parent, int32_t x, int32_t y, int32_t w,
                uint32_t color = SDK::GUI::Color::TEAL);

/// Vertical 3 px divider with rounded ends.
lv_obj_t* vline(lv_obj_t* parent, int32_t x, int32_t y, int32_t h,
                uint32_t color = SDK::GUI::Color::TEAL);

/// Static image at (x, y).
lv_obj_t* image(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y);

/// Filled circle of the given radius centred at (cx, cy).
lv_obj_t* dot(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, uint32_t color);

/// A static arc segment. Angles are TouchGFX-style (0 = 12 o'clock, clockwise);
/// @p radius is the arc's centre-line radius as in touchgfx::Circle.
lv_obj_t* arc(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, int32_t width,
              int32_t startDeg, int32_t endDeg, uint32_t color);

/// Re-aim an arc made by arc() (TouchGFX-style angles).
void setArc(lv_obj_t* arcObj, int32_t startDeg, int32_t endDeg);

void setArcColor(lv_obj_t* arcObj, uint32_t color);

} // namespace Theme

#endif // THEME_HPP
