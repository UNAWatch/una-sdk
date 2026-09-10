/**
 ******************************************************************************
 * @file    Screen.cpp
 * @brief   Base class of every RunLVGL screen.
 ******************************************************************************
 */

#include "gui/screens/Screen.hpp"
#include "gui/theme/Theme.hpp"

Screen::Screen(Model& model)
    : mModel(model)
{
}

Screen::~Screen()
{
    destroy();
}

void Screen::create()
{
    if (mRoot) {
        return;
    }
    mRoot = lv_obj_create(nullptr);
    Theme::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &Screen::keyEventCb, LV_EVENT_KEY, this);
    build();
}

void Screen::destroy()
{
    if (!mRoot) {
        return;
    }
    lv_obj_delete(mRoot);
    mRoot = nullptr;
}

void Screen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<Screen*>(lv_event_get_user_data(e));
    const uint8_t code = static_cast<uint8_t>(lv_event_get_key(e));
    self->mModel.handleKeyEvent(code);
    self->onKey(code);
}
