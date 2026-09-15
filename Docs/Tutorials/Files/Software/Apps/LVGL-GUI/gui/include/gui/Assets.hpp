/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts under assets/.
 *
 * The definitions are generated from assets/assets.json by
 * Utilities/Scripts/lvgl_assets/lvgl_assets.py. The SemiBold 35 face only
 * carries the dash (and a space): it draws the row markers and nothing else.
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

LV_FONT_DECLARE(poppins_semibold_35);   // the "-" row markers
LV_FONT_DECLARE(poppins_medium_25);     // the setting values

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
