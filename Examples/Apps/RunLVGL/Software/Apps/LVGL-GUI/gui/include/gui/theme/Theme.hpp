/**
 ******************************************************************************
 * @file    Theme.hpp
 * @brief   Shared colours, fonts and styles of the RunLVGL GUI.
 *
 * LVGL is built without a stock theme (lv_conf.h), so every widget starts
 * unstyled and screens apply these instead. Colours are quantised to the
 * kernel's 2-bit-per-channel display when the frame is packed, so pick values
 * that survive that: multiples of 0x55 per channel are exact.
 ******************************************************************************
 */

#ifndef THEME_HPP
#define THEME_HPP

#include "lvgl.h"

namespace Theme
{

// Colours that map exactly onto the ABGR2222 display.
inline lv_color_t black()  { return lv_color_hex(0x000000); }
inline lv_color_t white()  { return lv_color_hex(0xFFFFFF); }
inline lv_color_t grey()   { return lv_color_hex(0xAAAAAA); }
inline lv_color_t dark()   { return lv_color_hex(0x555555); }
inline lv_color_t amber()  { return lv_color_hex(0xFFAA00); }
inline lv_color_t green()  { return lv_color_hex(0x55FF55); }
inline lv_color_t red()    { return lv_color_hex(0xFF5555); }

/// Build the shared styles. Call once after lv_init(), before any screen.
void init();

/// Black full-screen background with no padding, border or scrollbars.
void applyScreen(lv_obj_t* screen);

/// White text in the given font, transparent background.
lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, const char* text);

} // namespace Theme

#endif // THEME_HPP
