/**
 ******************************************************************************
 * @file    LvglAssert.h
 * @brief   Assert hook LVGL calls through LV_ASSERT_HANDLER (see lv_conf.h).
 *
 * Plain C so LVGL's C sources can include it. Implemented in LvglPort.cpp.
 ******************************************************************************
 */

#ifndef SDK_PORT_LVGL_LVGL_ASSERT_H
#define SDK_PORT_LVGL_LVGL_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Log the failing location through the kernel logger and end the process.
 * @param file Source file of the failed LVGL assert.
 * @param line Line number of the failed LVGL assert.
 */
void una_lvgl_assert_failed(const char* file, int line);

#ifdef __cplusplus
}
#endif

#endif /* SDK_PORT_LVGL_LVGL_ASSERT_H */
