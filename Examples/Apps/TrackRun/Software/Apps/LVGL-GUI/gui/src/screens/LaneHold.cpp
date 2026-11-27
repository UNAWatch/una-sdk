/**
 ******************************************************************************
 * @file    LaneHold.cpp
 * @brief   The lane picker on a hold of Lap (see LaneHold.hpp).
 ******************************************************************************
 */

#include "gui/screens/LaneHold.hpp"
#include "gui/model/Model.hpp"
#include "gui/screens/ScreenManager.hpp"

#include "SDK/GUI/Button.hpp"

void LaneHold::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    if (code == Btn::R2_PRESS) {
        stop();
        if (mModel.laneShortcut() && mModel.isTrackActive()) {
            mTimer = lv_timer_create(&LaneHold::fireCb, kHoldMs, this);
            lv_timer_set_repeat_count(mTimer, 1);
        }
    } else if (code == Btn::R2_RELEASE) {
        stop();   // let go before kHoldMs: no picker (under 500 ms the click is a lap)
    }
}

void LaneHold::stop()
{
    if (mTimer != nullptr) {
        lv_timer_delete(mTimer);
        mTimer = nullptr;
    }
}

void LaneHold::fireCb(lv_timer_t* t)
{
    auto* self   = static_cast<LaneHold*>(lv_timer_get_user_data(t));
    self->mTimer = nullptr;   // one-shot: LVGL deletes the timer after this call
    self->mModel.setLaneFrom(Model::LaneFrom::Run);
    ScreenManager::instance().goTo(ScreenId::Lane);
}
