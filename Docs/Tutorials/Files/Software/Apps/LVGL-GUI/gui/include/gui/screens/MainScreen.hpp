/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Files' one screen: three settings in a column, one selected (red).
 *          L1/L2 change the selected value, R1 saves and moves the selection
 *          on, R2 saves and exits.
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
    void onSettingsUpdate(float decimalCounter, CustomMessage::ActivityType activityType,
                          CustomMessage::DisplayMode displayMode) override;

private:
    enum Setting : uint8_t { STG1 = 0, STG2, STG3, SETTING_COUNT };

    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);

    void changeSelected(int direction);
    void updateSettingsDisplay();

    Model&    mModel;
    lv_obj_t* mRoot = nullptr;
    lv_obj_t* mValue[SETTING_COUNT] = {};   ///< the three values, red when selected

    int32_t                     mDecimalCounter = 1;
    CustomMessage::ActivityType mActivityType   = CustomMessage::ActivityType::RUNNING;
    CustomMessage::DisplayMode  mDisplayMode    = CustomMessage::DisplayMode::SIMPLE;
    Setting                     mSelected       = STG1;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
