/**
 ******************************************************************************
 * @file    HelloScreen.cpp
 * @brief   Bring-up screen (see HelloScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/HelloScreen.hpp"
#include "gui/theme/Theme.hpp"

#include <cstdio>

#define LOG_MODULE_PRX      "HelloScreen"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

HelloScreen::HelloScreen(Model& model)
    : Screen(model)
{
}

void HelloScreen::build()
{
    // A ring around the face, so a rendering fault is obvious at a glance.
    mRing = lv_arc_create(mRoot);
    lv_obj_set_size(mRing, 232, 232);
    lv_obj_center(mRing);
    lv_arc_set_rotation(mRing, 270);
    lv_arc_set_bg_angles(mRing, 0, 360);
    lv_arc_set_value(mRing, 0);
    lv_obj_remove_style(mRing, nullptr, LV_PART_KNOB);
    lv_obj_remove_flag(mRing, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(mRing, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(mRing, Theme::dark(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(mRing, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(mRing, Theme::amber(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(mRing, LV_OPA_TRANSP, LV_PART_MAIN);

    mTitle = Theme::label(mRoot, &lv_font_montserrat_28, "RunLVGL");
    lv_obj_align(mTitle, LV_ALIGN_TOP_MID, 0, 34);

    mClock = Theme::label(mRoot, &lv_font_montserrat_28, "--:--:--");
    lv_obj_align(mClock, LV_ALIGN_CENTER, 0, -18);

    mBattery = Theme::label(mRoot, &lv_font_montserrat_14, "Battery --%");
    lv_obj_set_style_text_color(mBattery, Theme::grey(), LV_PART_MAIN);
    lv_obj_align(mBattery, LV_ALIGN_CENTER, 0, 16);

    mGps = Theme::label(mRoot, &lv_font_montserrat_14, "GPS searching");
    lv_obj_set_style_text_color(mGps, Theme::grey(), LV_PART_MAIN);
    lv_obj_align(mGps, LV_ALIGN_CENTER, 0, 36);

    mKey = Theme::label(mRoot, &lv_font_montserrat_14, "R2 exits");
    lv_obj_align(mKey, LV_ALIGN_BOTTOM_MID, 0, -36);
}

void HelloScreen::onShow()
{
    refreshStatus();
}

void HelloScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;

    switch (code) {
        case Btn::R2:
            mModel.exitApp();
            return;

        case Btn::L1:
        case Btn::L2:
        case Btn::R1:
            ++mClicks;
            lv_arc_set_value(mRing, static_cast<int32_t>((mClicks * 10) % 101));
            break;

        default:
            break;   // press / release codes: shown, not acted on
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "key '%c'  clicks %lu", static_cast<char>(code),
             static_cast<unsigned long>(mClicks));
    lv_label_set_text(mKey, buf);
}

void HelloScreen::onIdleTimeout()
{
    LOG_INFO("Idle timeout, exiting\n");
    mModel.exitApp();
}

void HelloScreen::onTime(uint8_t hour, uint8_t minute, uint8_t sec)
{
    lv_label_set_text_fmt(mClock, "%02u:%02u:%02u", hour, minute, sec);
}

void HelloScreen::onBatteryLevel(uint8_t level)
{
    lv_label_set_text_fmt(mBattery, "Battery %u%%", level);
}

void HelloScreen::onGpsFix(bool acquired)
{
    lv_label_set_text(mGps, acquired ? "GPS fixed" : "GPS searching");
    lv_obj_set_style_text_color(mGps, acquired ? Theme::green() : Theme::grey(), LV_PART_MAIN);
}

void HelloScreen::refreshStatus()
{
    uint8_t h = 0, m = 0, s = 0;
    mModel.getTime(h, m, s);
    onTime(h, m, s);
    onBatteryLevel(mModel.getBatteryLevel());
    onGpsFix(mModel.hasGpsFix());
}
