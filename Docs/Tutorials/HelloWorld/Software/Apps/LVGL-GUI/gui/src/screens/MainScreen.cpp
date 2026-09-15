/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   HelloWorld's one screen: the greeting and the R2 (back) hint.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    // A screen object with the SDK's black, unscrollable background style.
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Same geometry as the TouchGFX design: a 203 px wide text box at (19, 98).
    Draw::label(mRoot, &poppins_regular_18, "Hello World", 19, 98, 203,
                LV_TEXT_ALIGN_CENTER, SDK::GUI::Color::WHITE);

    // Only R2 (back) does something, so only its hint is shown.
    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::WHITE);

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
    switch (code) {
        case Btn::L1:
        case Btn::L2:
        case Btn::R1:
            // Nothing yet: the Buttons tutorial gives these a job.
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            // Press and release codes arrive too; HelloWorld acts on clicks.
            break;
    }
}
