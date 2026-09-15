/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts under assets/.
 *
 * The definitions are generated from assets/assets.json by
 * Utilities/Scripts/lvgl_assets/lvgl_assets.py.
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

LV_FONT_DECLARE(poppins_semibold_20);   // waypoint name
LV_FONT_DECLARE(poppins_semibold_25);   // bearing; distance when long
LV_FONT_DECLARE(poppins_semibold_30);   // distance, medium length
LV_FONT_DECLARE(poppins_semibold_40);   // distance, short
LV_FONT_DECLARE(poppins_regular_18);    // status line

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
