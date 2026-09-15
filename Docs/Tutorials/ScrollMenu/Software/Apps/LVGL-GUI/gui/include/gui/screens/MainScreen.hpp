/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   ScrollMenu's one screen: a three-item wheel menu (Counter,
 *          Increase, Decrease) and the counter it drives.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"
#include "SDK/GUI/LVGL/WheelMenu.hpp"

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
    enum Item : uint16_t { COUNTER = 0, INCREASE, DECREASE, ITEM_COUNT };

    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    void showCounter();

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

    // The menu keeps a pointer to this table; the counter item's text is a
    // buffer the screen rewrites, then asks the menu to re-render.
    SDK::LVGL::WheelMenu::Item mItems[ITEM_COUNT] {};
    char                       mCounterText[32] = "Counter";
    int                        mCounter = 0;

    std::unique_ptr<SDK::LVGL::WheelMenu> mMenu;
    std::unique_ptr<SDK::LVGL::Buttons>   mButtons;
};

#endif // MAIN_SCREEN_HPP
