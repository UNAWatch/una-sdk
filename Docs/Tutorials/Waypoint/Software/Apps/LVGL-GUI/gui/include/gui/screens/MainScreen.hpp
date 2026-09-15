/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Waypoint's one screen: the target's name, the distance and bearing
 *          to it, and a status line. R1 saves the current position as the
 *          new target, R2 exits.
 ******************************************************************************
 */

#ifndef MAIN_SCREEN_HPP
#define MAIN_SCREEN_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/LVGL/Buttons.hpp"

#include "Commands.hpp"
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

    // ModelListener
    void onNavUpdate(const CustomMessage::NavState& nav) override;
    void onTargetSaved(CustomMessage::SaveOutcome outcome, float latitude, float longitude) override;

private:
    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    static void noticeEndCb(lv_timer_t* t);

    /// Draw one navigation state: name, distance, bearing and status.
    void showNav(const CustomMessage::NavState& nav);
    /// Write the distance row, sizing the font to what the string needs.
    void setDistance(const char* text);
    void setStatus(const char* text);

    Model&    mModel;
    lv_obj_t* mRoot     = nullptr;
    lv_obj_t* mName     = nullptr;
    lv_obj_t* mDistance = nullptr;
    lv_obj_t* mBearing  = nullptr;
    lv_obj_t* mStatus   = nullptr;

    /// Runs while a save confirmation is shown in place of the status line.
    lv_timer_t* mNotice = nullptr;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
