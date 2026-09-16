/**
 ******************************************************************************
 * @file    MainScreen.cpp
 * @brief   Sensors' one screen: battery, sensor readings and statistics.
 ******************************************************************************
 */

#include "gui/screens/MainScreen.hpp"

#include <cstdio>

#include "SDK/GUI/Button.hpp"
#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/Draw.hpp"

#include "gui/Assets.hpp"

#define LOG_MODULE_PRX      "MainScreen"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace Draw = SDK::LVGL::Draw;
namespace Color = SDK::GUI::Color;

MainScreen::MainScreen(Model& model)
    : mModel(model)
{
    mRoot = lv_obj_create(nullptr);
    Draw::applyScreen(mRoot);
    lv_obj_add_event_cb(mRoot, &MainScreen::keyEventCb, LV_EVENT_KEY, this);

    // Three text boxes at the TouchGFX design's positions. A label grows to
    // the lines it holds, so the design's heights are not needed.
    mHeader = Draw::label(mRoot, &poppins_medium_10, "", 67, 6, 107);
    mBody   = Draw::label(mRoot, &poppins_regular_9, "", 0, 47, 240);
    mStats  = Draw::label(mRoot, &poppins_medium_10, "", 60, 181, 114);

    mButtons = std::make_unique<SDK::LVGL::Buttons>(mRoot);
    mButtons->set(SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::NONE,
                  SDK::LVGL::Buttons::NONE, SDK::LVGL::Buttons::AMBER);

    bind(&mModel);
    mModel.bind(this);
}

MainScreen::~MainScreen()
{
    mModel.bind(nullptr);
    lv_obj_delete(mRoot);
}

void MainScreen::keyEventCb(lv_event_t* e)
{
    auto* self = static_cast<MainScreen*>(lv_event_get_user_data(e));
    self->onKey(static_cast<uint8_t>(lv_event_get_key(e)));
}

void MainScreen::onFrame()
{
    // Once per kernel frame, as the TouchGFX view redraws in handleTickEvent().
    // A timer at the frame period would skip frames: LVGL runs it only when a
    // full period has elapsed since it last ran, and the ticks arrive a few
    // milliseconds apart from one another.
    refreshDisplay();
    refreshStats();
    refreshBattery();
    mFrameCounter++;
}

// ModelListener
void MainScreen::updateHR(float hr, float tl)            { mHr = hr; mHrTl = tl; }
void MainScreen::updateGPS(float lat, float lon, float alt) { mGpsLat = lat; mGpsLon = lon; mGpsAlt = alt; }
void MainScreen::updateElevation(float elevation)         { mElevation = elevation; }
void MainScreen::updateAccelerometer(float x, float y, float z) { mAccX = x; mAccY = y; mAccZ = z; }
void MainScreen::updateStepCounter(uint32_t steps)        { mSteps = steps; }
void MainScreen::updateFloorCounter(uint32_t floors)      { mFloors = floors; }
void MainScreen::updateCompass(float heading)             { mHeading = heading; }
void MainScreen::updateRTC(uint32_t time)                 { mRtcTime = time; }
void MainScreen::updateBattery(float level)               { mBatteryLevel = level; }
void MainScreen::updatePressure(float pressure)           { mPressure = pressure; }

void MainScreen::updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec,
                             float txBytesPerSec, float rxBytesPerSec)
{
    mServiceCpu    = serviceCpu;
    mGuiCpu        = guiCpu;
    mTxMsgPerSec   = txMsgPerSec;
    mRxMsgPerSec   = rxMsgPerSec;
    mTxBytesPerSec = txBytesPerSec;
    mRxBytesPerSec = rxBytesPerSec;
}

void MainScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    switch (code) {
        case Btn::L1:
            mVerbosity = static_cast<VerbosityLevel>((mVerbosity + 1) % VERB_LEVEL_MAX);
            LOG_DEBUG("Verbosity changed to %d\n", static_cast<int>(mVerbosity));
            break;
        case Btn::L2:
            mVerbosity = static_cast<VerbosityLevel>((mVerbosity - 1 + VERB_LEVEL_MAX) % VERB_LEVEL_MAX);
            LOG_DEBUG("Verbosity changed to %d\n", static_cast<int>(mVerbosity));
            break;
        case Btn::R2:
            mModel.exitApp();
            break;
        default:
            break;
    }
}

namespace
{
/// Appends to a fixed buffer, never past its end. Once full, further appends
/// are dropped, so a long value cannot push later lines off into memory.
class LineBuffer
{
public:
    LineBuffer(char* buf, size_t size) : mBuf(buf), mSize(size) { mBuf[0] = '\0'; }

    template <typename... Args>
    void add(const char* fmt, Args... args)
    {
        if (mLen + 1 >= mSize) {
            return;
        }
        const int n = snprintf(mBuf + mLen, mSize - mLen, fmt, args...);
        if (n > 0) {
            mLen = (static_cast<size_t>(n) < mSize - mLen) ? mLen + n : mSize - 1;
        }
    }

private:
    char*  mBuf;
    size_t mSize;
    size_t mLen = 0;
};
} // namespace

void MainScreen::refreshDisplay()
{
    char       buffer[256];
    LineBuffer text(buffer, sizeof(buffer));

    if (mVerbosity <= FULL) {
        // Group display
        if (mVerbosity >= BASIC) {
            text.add("HR: %.0f BPM\n", mHr);
            text.add("Steps: %lu\n", static_cast<unsigned long>(mSteps));
        }
        if (mVerbosity >= DETAILED) {
            text.add("GPS: %.2f, %.2f, %.0f\n", mGpsLat, mGpsLon, mGpsAlt);
            text.add("Elev: %.1f m\n", mElevation);
            text.add("Acc: %.2f, %.2f, %.2f\n", mAccX, mAccY, mAccZ);
            text.add("Floors: %lu\n", static_cast<unsigned long>(mFloors));
        }
        if (mVerbosity >= FULL) {
            text.add("Compass: %.0f\xC2\xB0\n", mHeading);
        }
    } else {
        // Per-sensor detailed display
        switch (mVerbosity) {
            case HR:    text.add("HR: %.0f BPM\nTL: %.0f\n", mHr, mHrTl); break;
            case GPS:   text.add("GPS: %.6f, %.6f\nAlt: %.1f m\n", mGpsLat, mGpsLon, mGpsAlt); break;
            case ALT:   text.add("Elevation: %.1f m\n", mElevation); break;
            case ACC:   text.add("Accelerometer:\nX: %.2f G\nY: %.2f G\nZ: %.2f G\n", mAccX, mAccY, mAccZ); break;
            case STEP:  text.add("Steps: %lu\n", static_cast<unsigned long>(mSteps)); break;
            case FLOOR: text.add("Floors: %lu\n", static_cast<unsigned long>(mFloors)); break;
            case MAG:   text.add("Compass: %.0f\n", mHeading); break;
            default:    break;
        }
    }

    // A single sensor gets the large face; the group views the small one.
    lv_obj_set_style_text_font(mBody, mVerbosity > FULL ? &poppins_regular_18 : &poppins_regular_9, LV_PART_MAIN);
    lv_label_set_text(mBody, buffer);
}

void MainScreen::refreshStats()
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "CPU S: %.1f%% G: %.1f%%\nMsg Tx: %.0f Rx: %.0f\nBytes Tx: %.0f Rx: %.0f",
             mServiceCpu, mGuiCpu, mTxMsgPerSec, mRxMsgPerSec, mTxBytesPerSec, mRxBytesPerSec);
    lv_label_set_text(mStats, buffer);
}

void MainScreen::refreshBattery()
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Battery: %.1f%%\n# %lu\nTime: %lu", mBatteryLevel,
             static_cast<unsigned long>(mFrameCounter), static_cast<unsigned long>(mRtcTime));
    lv_label_set_text(mHeader, buffer);
}
