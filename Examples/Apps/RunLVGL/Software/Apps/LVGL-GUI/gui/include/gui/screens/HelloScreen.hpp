/**
 ******************************************************************************
 * @file    HelloScreen.hpp
 * @brief   Bring-up screen: proves rendering, buttons, service messages and
 *          lifecycle on the kernel before the real Run screens are ported.
 *
 * Shows the clock, battery and GPS state fed by the service, echoes the last
 * button code, and exits on an R2 click or after the idle timeout.
 ******************************************************************************
 */

#ifndef HELLO_SCREEN_HPP
#define HELLO_SCREEN_HPP

#include "gui/screens/Screen.hpp"

class HelloScreen : public Screen
{
public:
    explicit HelloScreen(Model& model);

    // Screen
    void onShow() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onIdleTimeout() override;
    void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    void onBatteryLevel(uint8_t level) override;
    void onGpsFix(bool acquired) override;

protected:
    void build() override;

private:
    void refreshStatus();

    lv_obj_t* mTitle    = nullptr;
    lv_obj_t* mClock    = nullptr;
    lv_obj_t* mBattery  = nullptr;
    lv_obj_t* mGps      = nullptr;
    lv_obj_t* mKey      = nullptr;
    lv_obj_t* mRing     = nullptr;
    uint32_t  mClicks   = 0;
};

#endif // HELLO_SCREEN_HPP
