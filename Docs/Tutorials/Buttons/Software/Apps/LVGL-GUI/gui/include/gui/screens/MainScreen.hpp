/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Buttons' one screen: each button paints the background a colour,
 *          and R2 twice in a row exits.
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
    uint8_t   mLastKey = 0;   ///< previous click, for the double-R2 exit

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
