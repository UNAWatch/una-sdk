/**
 ******************************************************************************
 * @file    MainScreen.hpp
 * @brief   Sensors' one screen: battery at the top, sensor readings in the
 *          middle at a chosen level of detail, link statistics at the bottom.
 *
 * The screen stores the latest value of every sensor as the model reports
 * it, and rewrites its three labels ten times a second from a timer, the
 * way the TouchGFX view does from handleTickEvent().
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

    // ModelListener: store the values; the timer draws them.
    void updateHR(float hr, float tl) override;
    void updateGPS(float lat, float lon, float alt) override;
    void updateElevation(float elevation) override;
    void updateAccelerometer(float x, float y, float z) override;
    void updateStepCounter(uint32_t steps) override;
    void updateFloorCounter(uint32_t floors) override;
    void updateCompass(float heading) override;
    void updateRTC(uint32_t time) override;
    void updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec,
                     float txBytesPerSec, float rxBytesPerSec) override;
    void updateBattery(float level) override;
    void updatePressure(float pressure) override;

private:
    /// What the middle label shows: three group views, then one sensor at a time.
    enum VerbosityLevel { BASIC, DETAILED, FULL, HR, GPS, ALT, ACC, STEP, FLOOR, MAG, VERB_LEVEL_MAX };

    /// Called with an SDK::GUI::Button code (click, press or release).
    void onKey(uint8_t code);
    static void keyEventCb(lv_event_t* e);
    static void refreshCb(lv_timer_t* t);

    void refreshDisplay();
    void refreshStats();
    void refreshBattery();

    Model&      mModel;
    lv_obj_t*   mRoot    = nullptr;
    lv_obj_t*   mHeader  = nullptr;   ///< battery, frame count, time
    lv_obj_t*   mBody    = nullptr;   ///< sensor readings
    lv_obj_t*   mStats   = nullptr;   ///< CPU and message rates
    lv_timer_t* mRefresh = nullptr;

    VerbosityLevel mVerbosity = FULL;

    // Latest sensor values
    float    mHr = 0, mHrTl = 0;
    float    mGpsLat = 0, mGpsLon = 0, mGpsAlt = 0;
    float    mElevation = 0;
    float    mPressure = 0;
    float    mAccX = 0, mAccY = 0, mAccZ = 0;
    uint32_t mSteps = 0, mFloors = 0;
    float    mHeading = 0;
    uint32_t mRtcTime = 0;
    float    mServiceCpu = 0, mGuiCpu = 0, mTxMsgPerSec = 0, mRxMsgPerSec = 0, mTxBytesPerSec = 0, mRxBytesPerSec = 0;
    float    mBatteryLevel = 0;
    uint32_t mFrameCounter = 0;

    std::unique_ptr<SDK::LVGL::Buttons> mButtons;
};

#endif // MAIN_SCREEN_HPP
