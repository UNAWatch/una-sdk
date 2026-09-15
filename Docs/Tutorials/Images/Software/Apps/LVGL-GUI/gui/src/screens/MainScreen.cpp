/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Images' one screen: a character drawn plain or scaled, with a jump.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cmath>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // The plain image, drawn at the bitmap's own size (76 x 115) inside a
    // 100 x 100 box that clips it, as the TouchGFX Image widget does.
    mPlainBox = Draw::container(mRoot, kX, kY, kPlainSize, kPlainSize);
    mPlain    = Draw::image(mPlainBox, &img_guy_transparent, 0, 0);

    // The scaled image: LVGL scales in 1/256 steps, separately per axis, so
    // stretching a 76 x 115 bitmap over 120 x 120 is two factors. The result
    // is filtered (bilinear) by LVGL's software renderer.
    mScaled = Draw::image(mRoot, &img_guy_transparent, kX, kY);
    lv_image_set_scale_x(mScaled, kScaledSize * 256 / img_guy_transparent.header.w);
    lv_image_set_scale_y(mScaled, kScaledSize * 256 / img_guy_transparent.header.h);
    lv_image_set_pivot(mScaled, 0, 0);   // scale from the top-left corner, not the centre
    lv_image_set_antialias(mScaled, true);

    // L1 toggles the mode, R1 jumps, R2 exits.
    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::WHITE);

    showMode();
    bind(&mModel);
    mModel.bind(this);
}

MainScreen::~MainScreen()
{
    if (mJumpTimer) {
        lv_timer_delete(mJumpTimer);
    }
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
    switch (code) {
        case Btn::L1:
            mScaledMode = !mScaledMode;
            showMode();
            break;
        case Btn::R1:
            // Start (or restart) the jump; it only shows in plain mode, as in
            // the TouchGFX app.
            if (!mScaledMode) {
                mJumpTick = 0;
                if (!mJumpTimer) {
                    mJumpTimer = lv_timer_create(&MainScreen::jumpTickCb, 100, this);
                }
            }
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

void MainScreen::showMode()
{
    Draw::setHidden(mPlainBox, mScaledMode);
    Draw::setHidden(mScaled, !mScaledMode);
}

void MainScreen::jumpTickCb(lv_timer_t* t)
{
    // One step per 100 ms, the watch's frame period: the same sine-wave offset
    // the TouchGFX view computes in handleTickEvent().
    auto* self = static_cast<MainScreen*>(lv_timer_get_user_data(t));
    self->mJumpTick++;
    const float   phase  = self->mJumpTick * 0.1f;
    const int32_t offset = static_cast<int32_t>(sinf(phase) * 30.0f);
    lv_obj_set_y(self->mPlainBox, kY + offset);
    if (self->mJumpTick > kJumpTicks) {
        lv_timer_delete(self->mJumpTimer);
        self->mJumpTimer = nullptr;
    }
}
