/**
 ******************************************************************************
 * @file    Assets.hpp
 * @brief   Declarations of the generated fonts and images under assets/.
 *
 * Regenerate the definitions with assets/gen_assets.py; names follow the
 * source file names it derives them from.
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

// Fonts (Poppins, 2 bpp). The Light 60, Medium 40 and SemiBold 40 faces carry
// digits and punctuation only; SemiBold 60 adds A/P/M for the clock suffix and
// O/p/e/n for the interval timer's "Open"; the rest cover printable ASCII.
LV_FONT_DECLARE(poppins_italic_18);
LV_FONT_DECLARE(poppins_italic_20);
LV_FONT_DECLARE(poppins_light_60);
LV_FONT_DECLARE(poppins_medium_18);
LV_FONT_DECLARE(poppins_medium_25);
LV_FONT_DECLARE(poppins_medium_40);
LV_FONT_DECLARE(poppins_regular_14);
LV_FONT_DECLARE(poppins_regular_16);
LV_FONT_DECLARE(poppins_regular_18);
LV_FONT_DECLARE(poppins_semibold_20);
LV_FONT_DECLARE(poppins_semibold_25);
LV_FONT_DECLARE(poppins_semibold_30);
LV_FONT_DECLARE(poppins_semibold_35);
LV_FONT_DECLARE(poppins_semibold_40);
LV_FONT_DECLARE(poppins_semibold_60);

// Images. Multi-colour icons are RGB565A8; single-colour ones (ticks, crosses,
// pause and sensor icons) are A8 alpha masks that the screens tint with
// Theme::imageTinted(), so one bitmap serves every colour variant.
LV_IMAGE_DECLARE(img_circlecross_50x50);
LV_IMAGE_DECLARE(img_circletick_50x50);
LV_IMAGE_DECLARE(img_clock_16x19);
LV_IMAGE_DECLARE(img_crosswhite_17x17);
LV_IMAGE_DECLARE(img_heart_30x30);
LV_IMAGE_DECLARE(img_heart_46x39);
LV_IMAGE_DECLARE(img_intervals_24x26);
LV_IMAGE_DECLARE(img_intervals_40x43);
LV_IMAGE_DECLARE(img_pace_30x30);
LV_IMAGE_DECLARE(img_pause_14x14);
LV_IMAGE_DECLARE(img_runningman_30x30);
LV_IMAGE_DECLARE(img_runningman_46x46);
LV_IMAGE_DECLARE(img_sensorgpslight);
LV_IMAGE_DECLARE(img_sensorhrlight);
LV_IMAGE_DECLARE(img_tickgreen_22x17);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ASSETS_HPP
