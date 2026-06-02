/**
 ******************************************************************************
 * @file    lv_port_disp.h
 * @brief   LVGL v9 display port for the Una-Watch kernel GUI.
 *
 * Creates and configures the single 240x240 LVGL display. The flush callback
 * down-converts the RGB565 render buffer to the 8bpp ABGR2222 transport buffer
 * consumed by SDK::TouchGFXCommandProcessor::writeDisplayFrameBuffer (the frozen
 * transport seam — see Design.md §1.2 and the migration contract).
 ******************************************************************************
 */

#ifndef __UNA_LV_PORT_DISP_H
#define __UNA_LV_PORT_DISP_H

#include "lvgl.h"

/* Fixed display geometry (also defined in lv_conf.h; mirrored for callers that
   include this header without lv_conf.h visible). */
#ifndef UNA_LV_HOR_RES
#define UNA_LV_HOR_RES 240
#endif
#ifndef UNA_LV_VER_RES
#define UNA_LV_VER_RES 240
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create + configure the single LVGL display (240x240, RGB565 render,
 *        FULL render mode, flush_cb -> ABGR2222 pack -> kernel transport).
 * @return the created lv_display_t* (also set as the default display).
 */
lv_display_t* lv_port_disp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __UNA_LV_PORT_DISP_H */
