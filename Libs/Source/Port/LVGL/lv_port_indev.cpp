/**
 ******************************************************************************
 * @file    lv_port_indev.cpp
 * @brief   LVGL v9 keypad input-device port for the Una-Watch kernel GUI.
 *
 * One keypad indev (no touch / pointer). read_cb polls the SAME source as the
 * TouchGFX port: SDK::TouchGFXCommandProcessor::getKeySample(). data->key carries
 * the RAW ASCII code ('1'..'4' = Gui::Config::Button L1/L2/R1/R2), never an
 * LV_KEY_* constant. The SW2<->SW3 swap is already applied inside the transport
 * (TouchGFXCommandProcessor::handleEvent). Presses are edge-like: getKeySample
 * clears the cached code at the start of each waitForFrameTick() (Design.md §1.3).
 ******************************************************************************
 */

#include "SDK/Port/LVGL/lv_port_indev.h"

#define LOG_MODULE_PRX   "lv_port_indev"
#define LOG_MODULE_LEVEL LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

/* The single global input group. The ScreenManager rebinds focused objects on
   each screen swap (Design.md §2.4). */
static lv_group_t* s_input_group = nullptr;

/**
 * @brief Keypad read callback. Reports the raw ASCII button code from the kernel.
 */
static void una_lv_keypad_read_cb(lv_indev_t* /*indev*/, lv_indev_data_t* data)
{
    uint8_t key = 0;
    bool pressed = SDK::TouchGFXCommandProcessor::GetInstance().getKeySample(key);
    if (pressed) {
        data->key   = key;                      /* raw ASCII '1'..'4' (Gui::Config::Button) */
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

extern "C" lv_indev_t* lv_port_indev_init(void)
{
    s_input_group = lv_group_create();

    lv_indev_t* indev = lv_indev_create();
    if (!indev) {
        LOG_ERROR("lv_indev_create failed\n");
        return nullptr;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, una_lv_keypad_read_cb);
    lv_indev_set_group(indev, s_input_group);

    return indev;
}

extern "C" lv_group_t* lv_port_indev_get_group(void)
{
    return s_input_group;
}
