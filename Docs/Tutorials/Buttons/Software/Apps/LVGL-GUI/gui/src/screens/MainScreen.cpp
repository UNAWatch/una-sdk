/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Buttons' one screen: each button paints the background a colour.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

namespace Draw = SDK::LVGL::Draw;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);   // black to start with
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Every button does something, so every hint is shown.
    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::AMBER,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::AMBER);

    bind(&mModel);
    mModel.bind(this);
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;

    // Press and release codes arrive as well as clicks; only clicks count here,
    // so a press followed by an R2 click still reads as a double R2.
    if (!Btn::isButtonCode(code) || code == Btn::L1_PRESS || code == Btn::L2_PRESS ||
        code == Btn::R1_PRESS || code == Btn::R2_PRESS || code == Btn::L1_RELEASE ||
        code == Btn::L2_RELEASE || code == Btn::R1_RELEASE || code == Btn::R2_RELEASE) {
        return;
    }

    // Setting a style property marks the object dirty; LVGL redraws it on the
    // next frame. There is no invalidate() to call.
    switch (code) {
        case Btn::L1: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x000000), LV_PART_MAIN); break;
        case Btn::L2: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0xFF0000), LV_PART_MAIN); break;
        case Btn::R1: lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x0000FF), LV_PART_MAIN); break;
        case Btn::R2:
            lv_obj_set_style_bg_color(mRoot, Draw::rgb(0x00FF00), LV_PART_MAIN);
            if (mLastKey == code) {
                mModel.exitApp();
            }
            break;
        default:
            break;
    }
    mLastKey = code;
}
