/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts under assets/.
 *
 * The definitions are generated from assets/assets.json by
 * Utilities/Scripts/lvgl_assets/lvgl_assets.py. The two body faces also
 * carry the degree sign (U+00B0) for the compass line. All faces are 2 bits
 * per pixel: the display has two bits per colour channel, so finer glyph
 * anti-aliasing cannot be shown.
 ******************************************************************************
 */

#ifndef ASSETS_HPP
#define ASSETS_HPP

#include "lvgl.h"

// The generated definitions are C, so the names must not be mangled. GCC
// leaves global variables unmangled anyway; MSVC (the PC simulator) does not.
#ifdef __cplusplus
extern "C" {
#endif

LV_FONT_DECLARE(poppins_medium_10);   // header and stats
LV_FONT_DECLARE(poppins_regular_9);   // body, group views
LV_FONT_DECLARE(poppins_regular_18);  // body, single-sensor views

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
