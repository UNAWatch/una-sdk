/**
 ******************************************************************************
 * @file    app_lvgl.cpp  (SDK port glue)
 * @brief   PORT-level LVGL bring-up for the Una-Watch kernel GUI.
 *
 * Provides MX_LVGL_Init(): lv_init + display/indev/lifecycle init + theme init.
 * Does NOT create an OS task (that is the KERNEL-side app_lvgl.cpp's job) and does
 * NOT call MX_CRC_Init (LVGL needs no CRC peripheral) — Design.md §1.5.
 ******************************************************************************
 */

#include "SDK/Port/LVGL/app_lvgl.h"

#define LOG_MODULE_PRX   "app_lvgl_port"
#define LOG_MODULE_LEVEL LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "lvgl.h"

#include "SDK/Port/LVGL/lv_port_disp.h"
#include "SDK/Port/LVGL/lv_port_indev.h"
#include "SDK/Port/LVGL/lv_port_lifecycle.h"

/* Theme module owned by the theme foundation task (Software/App/LVGL-GUI/theme). */
#include "Theme.hpp"

extern "C" void MX_LVGL_Init(void)
{
    lv_init();

    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_lifecycle_init();

    Theme::theme_init();

    // NOTE: NO MX_CRC_Init() here — LVGL needs no CRC peripheral (Design.md §1.5,
    // TODO-CRC: verify no non-GUI consumer relies on GUI init for CRC bring-up).
    // NO frame timer — the kernel's EVENT_GUI_TICK is the frame clock (§1.5 item 3).
}

extern "C" void MX_LVGL_Process(void)
{
    // No-op: parity with MX_TouchGFX_Process. The render loop is lvgl_taskEntry().
}
