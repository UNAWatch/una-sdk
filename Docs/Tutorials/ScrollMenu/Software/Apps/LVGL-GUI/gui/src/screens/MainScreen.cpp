/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   ScrollMenu's one screen: a three-item wheel menu and a counter.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

namespace Draw = SDK::LVGL::Draw;
using SDK::LVGL::WheelMenu;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // The items. Simple style: centred text, large when selected. The first
    // item's text lives in mCounterText so it can show the count.
    using Style = WheelMenu::Item::Style;
    mItems[COUNTER]  = { Style::Simple, mCounterText };
    mItems[INCREASE] = { Style::Simple, "Increase" };
    mItems[DECREASE] = { Style::Simple, "Decrease" };

    // The wheel: SemiBold 30 for the selected item, Medium 18 around it, and
    // the italic face for Tip items' hint lines (unused here). The SDK widget
    // draws the teal lens, the two-line-of-sight strips and the scroll
    // indicator on the left bezel; the slide takes 400 ms by default.
    const WheelMenu::Fonts fonts = { &poppins_semibold_30, &poppins_medium_18, &poppins_italic_18 };
    mMenu = std::make_unique<WheelMenu>(mRoot, mItems, ITEM_COUNT, fonts);

    // R1 (act on the item) amber and R2 (exit) white: what the TouchGFX app
    // shows, where the Menu container's own hints draw over the view's.
    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::AMBER, SDK::LVGL::Buttons::WHITE);

    bind(&mModel);
    mModel.bind(this);
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    mMenu.reset();      // stops its animations before the objects go
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
            mMenu->prev();
            break;
        case Btn::L2:
            mMenu->next();
            break;
        case Btn::R1:
            // Act on the selected item.
            switch (mMenu->selected()) {
                case COUNTER:  mCounter = 0; break;
                case INCREASE: mCounter++;   break;
                case DECREASE: mCounter--;   break;
                default: break;
            }
            showCounter();
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            // Press and release codes arrive too; this screen acts on clicks.
            break;
    }
}

void MainScreen::showCounter()
{
    // The item table points at this buffer; refresh() re-renders the wheel's
    // slots from the table, so the change shows whether or not the counter
    // item is the selected one.
    snprintf(mCounterText, sizeof(mCounterText), "Counter: %d", mCounter);
    mMenu->refresh();
}
