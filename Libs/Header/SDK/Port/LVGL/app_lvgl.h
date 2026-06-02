/**
 ******************************************************************************
 * @file    app_lvgl.h  (SDK port glue)
 * @brief   PORT-level LVGL bring-up entry for the Una-Watch kernel GUI.
 *
 * This is the SDK-port glue header (lives in SDK/Libs/Header/SDK/Port/LVGL).
 * It exposes the PORT-level bring-up callable as MX_LVGL_Init():
 *   lv_init() + lv_port_disp_init() + lv_port_indev_init()
 *            + lv_port_lifecycle_init() + Theme::theme_init().
 *
 * NOTE: this is distinct from the KERNEL-side app_lvgl.h at
 * Software/Libs/Header/Components/GUI/app_lvgl.h, which replaces app_touchgfx.h
 * 1:1 in the kernel tree, owns the FreeRTOS GUI task, and forwards here
 * (Design.md §1.5).
 ******************************************************************************
 */

#ifndef __UNA_SDK_PORT_APP_LVGL_H
#define __UNA_SDK_PORT_APP_LVGL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PORT-level LVGL bring-up: lv_init + display/indev/lifecycle + theme.
 *        Does NOT create any OS task and does NOT call MX_CRC_Init (LVGL needs
 *        no CRC peripheral — Design.md §1.5).
 */
void MX_LVGL_Init(void);

/**
 * @brief No-op, for parity with MX_TouchGFX_Process.
 */
void MX_LVGL_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __UNA_SDK_PORT_APP_LVGL_H */
