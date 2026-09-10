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

// Fonts (Poppins, 2 bpp). The 40 and 60 px faces carry digits and punctuation
// only (60 adds A/P/M for the clock suffix); the rest cover printable ASCII.
LV_FONT_DECLARE(poppins_italic_18);
LV_FONT_DECLARE(poppins_medium_18);
LV_FONT_DECLARE(poppins_medium_25);
LV_FONT_DECLARE(poppins_regular_14);
LV_FONT_DECLARE(poppins_regular_16);
LV_FONT_DECLARE(poppins_regular_18);
LV_FONT_DECLARE(poppins_semibold_20);
LV_FONT_DECLARE(poppins_semibold_25);
LV_FONT_DECLARE(poppins_semibold_30);
LV_FONT_DECLARE(poppins_semibold_35);
LV_FONT_DECLARE(poppins_semibold_40);
LV_FONT_DECLARE(poppins_semibold_60);

// Images (RGB565A8).
LV_IMAGE_DECLARE(img_circlecross_50x50);
LV_IMAGE_DECLARE(img_circletick_50x50);
LV_IMAGE_DECLARE(img_clock_16x19);
LV_IMAGE_DECLARE(img_crossamber_17x17);
LV_IMAGE_DECLARE(img_crosswhite_17x17);
LV_IMAGE_DECLARE(img_heart_30x30);
LV_IMAGE_DECLARE(img_heart_46x39);
LV_IMAGE_DECLARE(img_heartratezone1);
LV_IMAGE_DECLARE(img_heartratezone2);
LV_IMAGE_DECLARE(img_heartratezone3);
LV_IMAGE_DECLARE(img_heartratezone4);
LV_IMAGE_DECLARE(img_heartratezone5);
LV_IMAGE_DECLARE(img_heartratezonegroup);
LV_IMAGE_DECLARE(img_intervals_24x26);
LV_IMAGE_DECLARE(img_intervals_40x43);
LV_IMAGE_DECLARE(img_pace_30x30);
LV_IMAGE_DECLARE(img_pause_14x14);
LV_IMAGE_DECLARE(img_runningman_30x30);
LV_IMAGE_DECLARE(img_runningman_46x46);
LV_IMAGE_DECLARE(img_sensorgpsdark);
LV_IMAGE_DECLARE(img_sensorgpslight);
LV_IMAGE_DECLARE(img_sensorhrdark);
LV_IMAGE_DECLARE(img_sensorhrlight);
LV_IMAGE_DECLARE(img_tickamber_22x17);
LV_IMAGE_DECLARE(img_tickgreen_22x17);
LV_IMAGE_DECLARE(img_tickred_22x17);

#endif // ASSETS_HPP
