/**
 ******************************************************************************
 * @file    lv_port_lifecycle.h
 * @brief   LVGL v9 lifecycle + tick + render loop for the Una-Watch kernel GUI.
 *
 * Owns the IGuiLifeCycleCallback implementation and the GUI render loop. Analog
 * of TouchGFX's OSWrappers::waitForVSync + hal.taskEntry(). The frame clock is
 * the kernel's EVENT_GUI_TICK consumed via TouchGFXCommandProcessor::waitForFrameTick();
 * lv_tick_inc() is fed once per frame from onFrame() (Design.md §1.4).
 ******************************************************************************
 */

#ifndef __UNA_LV_PORT_LIFECYCLE_H
#define __UNA_LV_PORT_LIFECYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the GuiLifeCycle callback with the kernel command processor.
 *        Call once after lv_init + display/indev init.
 */
void lv_port_lifecycle_init(void);

/**
 * @brief The GUI render loop. Never returns (exits via kernel sys.exit on STOP).
 *
 * Loop: waitForFrameTick() gate -> callCustomMessageHandler() -> Model::tick()
 *       -> lv_timer_handler().
 */
void lvgl_taskEntry(void);

#ifdef __cplusplus
}
#endif

#endif /* __UNA_LV_PORT_LIFECYCLE_H */
