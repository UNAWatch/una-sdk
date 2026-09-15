/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Images' one screen: a character drawn at its own size or scaled,
 *          with a jump animation.
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
    static constexpr int32_t kX         = 70;    // the TouchGFX design's position
    static constexpr int32_t kY         = 100;
    static constexpr int32_t kPlainSize = 100;   // the plain image's box
    static constexpr int32_t kScaledSize = 120;  // the scaled image's box
    static constexpr int32_t kJumpTicks = 60;    // 6 s at 10 frames per second

    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    void showMode();
    /// One jump step per kernel frame while the jump runs.
    void onFrame() override;

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;

    lv_obj_t*   mPlainBox  = nullptr;   ///< clips the plain image to 100 x 100
    lv_obj_t*   mPlain     = nullptr;   ///< the image at its own size
    lv_obj_t*   mScaled    = nullptr;   ///< the image stretched to 120 x 120
    bool        mJumping   = false;     ///< the jump is animating
    int         mJumpTick  = 0;
    bool        mScaledMode = true;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
