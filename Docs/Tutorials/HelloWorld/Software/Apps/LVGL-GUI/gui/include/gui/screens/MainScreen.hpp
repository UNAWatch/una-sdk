/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   HelloWorld's one screen: the greeting and the R2 (back) hint.
 *
 * A screen owns one LVGL screen object (lv_obj_create(nullptr)) and receives
 * the kernel's button codes through LV_EVENT_KEY on it. It is the model's
 * listener while on display. This is the TouchGFX MainView and MainPresenter
 * in one class.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

class MainScreen : public ModelListener
{
public:
    explicit MainScreen(Model& model);
    ~MainScreen();

    MainScreen(const MainScreen&)            = delete;
    MainScreen& operator=(const MainScreen&) = delete;

    /// The LVGL screen object, to pass to lv_screen_load().
    lv_obj_t* root() const { return mRoot; }

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
