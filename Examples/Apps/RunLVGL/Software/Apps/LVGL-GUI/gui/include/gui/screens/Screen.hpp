/**
 ******************************************************************************
 * @file    Screen.hpp
 * @brief   Base class of every RunLVGL screen.
 *
 * A Screen owns one LVGL screen object (lv_obj_create(nullptr)), receives the
 * kernel's button codes through LV_EVENT_KEY on that object, and is the
 * Model's listener while it is on display. This plays the role TouchGFX's
 * View + Presenter pair play in the Run app, collapsed into one class.
 ******************************************************************************
 */

#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <cstdint>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class Screen : public ModelListener
{
public:
    explicit Screen(Model& model);
    virtual ~Screen();

    Screen(const Screen&)            = delete;
    Screen& operator=(const Screen&) = delete;

    /// Create the LVGL screen object and its widgets (calls build()).
    void create();

    /// Delete the LVGL screen object and everything on it.
    void destroy();

    /// The LVGL screen object, or nullptr before create() / after destroy().
    lv_obj_t* root() const { return mRoot; }

    /// Called after this screen has become the active one.
    virtual void onShow() {}

    /// Called with an SDK::GUI::Button code (click, press or release).
    virtual void onKey(uint8_t code) { (void)code; }

protected:
    /// Populate mRoot with widgets. mRoot exists and is styled as the background.
    virtual void build() = 0;

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

private:
    static void keyEventCb(lv_event_t* e);
};

#endif // SCREEN_HPP
