/**
 ******************************************************************************
 * @file    lv_port_indev.h
 * @brief   LVGL v9 keypad input-device port for the Una-Watch kernel GUI.
 *
 * One keypad indev (no touch / pointer). read_cb polls the SAME button source as
 * the TouchGFX port: SDK::TouchGFXCommandProcessor::getKeySample(). data->key
 * carries the RAW ASCII code ('1'..'4' = Gui::Config::Button L1/L2/R1/R2), NOT an
 * LV_KEY_* constant (Design.md §1.3). One global input group is exposed for the
 * ScreenManager to rebind on each screen load.
 ******************************************************************************
 */

#ifndef __UNA_LV_PORT_INDEV_H
#define __UNA_LV_PORT_INDEV_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the keypad indev (LV_INDEV_TYPE_KEYPAD) and its global input group.
 * @return the created lv_indev_t*.
 */
lv_indev_t* lv_port_indev_init(void);

/**
 * @brief Accessor for the single global input group (used by ScreenManager to
 *        rebind focused objects on screen swap).
 * @return the global lv_group_t* (NULL only if lv_port_indev_init() not yet called).
 */
lv_group_t* lv_port_indev_get_group(void);

#ifdef __cplusplus
}
#endif

#endif /* __UNA_LV_PORT_INDEV_H */
