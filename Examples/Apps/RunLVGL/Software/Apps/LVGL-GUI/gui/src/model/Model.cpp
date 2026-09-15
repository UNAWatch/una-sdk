/**
 ******************************************************************************
 * @file    Model.cpp
 * @brief   GUI-side state of the Run app and its link to the service process.
 ******************************************************************************
 */

#include "gui/model/Model.hpp"
#include "gui/model/ModelListener.hpp"

#include <cstring>

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Messages/MessageGuard.hpp"
#include "SDK/Port/LVGL/LvglPort.hpp"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
{
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(this);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(this);

    memcpy(mHrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(mHrThresholds));
}

// Controls
void Model::handleKeyEvent(uint8_t key)
{
    LOG_DEBUG("key = %c\n", static_cast<char>(key));
    if (isClick(key)) {
        resetIdleTimer();
    }
}

void Model::resetIdleTimer()
{
    mIdleTimer = App::Config::kScreenTimeoutSteps;
}

void Model::exitApp()
{
    LOG_INFO("Manually exiting the application\n");
    SDK::LVGL::Port::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::LVGL::Port::GetInstance().setCustomMessageHandler(nullptr);
    mKernel.sys.exit();
}

// Date/Time
void Model::getDate(uint8_t& month, uint8_t& day, uint8_t& weekday) const
{
    month   = static_cast<uint8_t>(mTime.tm_mon);
    day     = static_cast<uint8_t>(mTime.tm_mday);
    weekday = static_cast<uint8_t>(mTime.tm_wday);
}

void Model::getTime(uint8_t& h, uint8_t& m, uint8_t& s) const
{
    h = static_cast<uint8_t>(mTime.tm_hour);
    m = static_cast<uint8_t>(mTime.tm_min);
    s = static_cast<uint8_t>(mTime.tm_sec);
}

// Power
uint8_t Model::getBatteryLevel() const
{
    return mBatteryLevel;
}

// Settings
bool Model::isUnitsImperial() const
{
    return mUnitsImperial;
}

bool Model::is12HourFormat() const
{
    return mTimeFormat12h;
}

const uint8_t* Model::getHrThresholds() const
{
    return mHrThresholds;
}

uint8_t Model::getHrThresholdsCount() const
{
    return mHrThresholdsCount;
}

const Settings& Model::getSettings() const
{
    return mSettings;
}

void Model::saveSettings(const Settings& sett)
{
    mSettings = sett;
    SDK::send_msg<CustomMessage::SettingsSave>(mKernel, mSettings);
}

// GPS
bool Model::hasGpsFix() const
{
    return mGpsFix;
}

uint8_t Model::getAccessoryState() const
{
    return mAccessoryState;
}

// Track
void Model::setHoldConfirmMode(HoldConfirmMode mode)
{
    mHoldConfirmMode = mode;
}

Model::HoldConfirmMode Model::getHoldConfirmMode() const
{
    return mHoldConfirmMode;
}

void Model::setPendingIntervalsMode(bool mode)
{
    mPendingIntervalsMode = mode;
}

bool Model::isPendingIntervalsMode() const
{
    return mPendingIntervalsMode;
}

const Track::IntervalsData& Model::getPendingAlertIntervals() const
{
    return mPendingAlertIntervals;
}

// mTrackData.intervals is pre-populated from settings so that the destination
// screen has valid data before the first TRACK_DATA_UPDATE arrives from the
// Service (~1 s after start). Mirrors Service::startTrack().
void Model::trackStart(bool intervalsMode)
{
    mTrackData.intervalsMode = intervalsMode;

    if (intervalsMode) {
        const Settings::Intervals& cfg = mSettings.intervals;
        Track::IntervalsData& iv       = mTrackData.intervals;

        iv = Track::IntervalsData{};
        // totalRepeats is the literal repeat count chosen by the user; 0 == 'Open'.
        iv.totalRepeats = cfg.repeatsNum;

        if (cfg.warmUp) {
            iv.phase  = Track::IntervalsPhase::WARM_UP;
            iv.metric = Track::IntervalsMetric::TIME_OPEN;
        } else {
            iv.phase  = Track::IntervalsPhase::RUN;
            iv.repeat = 1;
            switch (cfg.runMetric) {
                case Settings::Intervals::TIME:
                    iv.metric        = Track::IntervalsMetric::TIME_REMAINING;
                    iv.phaseTimerSec = static_cast<time_t>(cfg.runTime);
                    break;
                case Settings::Intervals::DISTANCE:
                    iv.metric        = Track::IntervalsMetric::DISTANCE;
                    iv.distRemaining = cfg.runDistance;
                    break;
                default:
                    iv.metric = Track::IntervalsMetric::TIME_OPEN;
                    break;
            }
            // warmUp=false: the GUI goes straight to the alert screen before any
            // INTERVALS_PHASE_ALERT arrives, so give it the same data now.
            mPendingAlertIntervals = iv;
        }
    }

    SDK::send_msg<CustomMessage::TrackStart>(mKernel, intervalsMode);
}

void Model::intervalsNextPhase()
{
    SDK::send_msg<CustomMessage::IntervalsNextPhase>(mKernel);
}

bool Model::isTrackActive() const
{
    return mTrackState != Track::State::INACTIVE;
}

void Model::trackPause()
{
    SDK::send_msg<CustomMessage::TrackPause>(mKernel);
}

void Model::trackResume()
{
    SDK::send_msg<CustomMessage::TrackResume>(mKernel);
}

bool Model::isTrackPaused() const
{
    return mTrackState == Track::State::PAUSED;
}

const Track::Data& Model::getTrackData() const
{
    return mTrackData;
}

void Model::saveLap()
{
    SDK::send_msg<CustomMessage::ManualLap>(mKernel);
}

void Model::saveTrack()
{
    SDK::send_msg<CustomMessage::TrackStop>(mKernel, false);
}

void Model::discardTrack()
{
    SDK::send_msg<CustomMessage::TrackStop>(mKernel, true);
}

bool Model::isTrackSummaryAvailable() const
{
    return mActivitySummary != nullptr && mActivitySummary->time != 0;
}

const ActivitySummary& Model::getTrackSummary() const
{
    return *mActivitySummary;
}

// Private
void Model::decIdleTimer()
{
    if (mIdleTimer > 0) {
        if (--mIdleTimer == 0 && modelListener) {
            modelListener->onIdleTimeout();
        }
    }
}

bool Model::isClick(uint8_t key)
{
    return key == SDK::GUI::Button::L1 ||
           key == SDK::GUI::Button::L2 ||
           key == SDK::GUI::Button::R1 ||
           key == SDK::GUI::Button::R2;
}

// IGuiLifeCycleCallback
void Model::onStart()
{
    LOG_INFO("Started\n");
}

void Model::onFrame()
{
    if (mIsRunning) {
        decIdleTimer();
    }
}

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
}

void Model::onSuspend()
{
    mIsRunning = false;
    if (modelListener) {
        modelListener->onSuspend();
    }
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}

// ICustomMessageHandler
bool Model::customMessageHandler(SDK::MessageBase* message)
{
    // State is always updated; with no screen bound the notifications go to a
    // listener that ignores them.
    static ModelListener sNullListener;
    ModelListener* modelListener = this->modelListener ? this->modelListener : &sNullListener;

    switch (message->getType()) {
        case CustomMessage::SETTINGS_UPDATE: {
            LOG_DEBUG("SETTINGS_UPDATE\n");
            auto* msg          = static_cast<CustomMessage::SettingsUpd*>(message);
            mSettings          = msg->settings;
            mUnitsImperial     = msg->unitsImperial;
            mTimeFormat12h     = msg->timeFormat12h;
            memcpy(mHrThresholds, msg->hrThresholds, sizeof(mHrThresholds));
            mHrThresholdsCount = msg->hrThresholdsCount;
            modelListener->onSettings(mSettings);
        } break;

        case CustomMessage::LOCAL_TIME: {
            auto* msg = static_cast<CustomMessage::Time*>(message);
            std::tm newTime = msg->localTime;

            const bool dateChanged = newTime.tm_year  != mTime.tm_year  ||
                                     newTime.tm_mon   != mTime.tm_mon   ||
                                     newTime.tm_mday  != mTime.tm_mday;
            const bool timeChanged = newTime.tm_hour  != mTime.tm_hour  ||
                                     newTime.tm_min   != mTime.tm_min   ||
                                     newTime.tm_sec   != mTime.tm_sec;
            mTime = newTime;

            if (dateChanged) {
                modelListener->onDate(static_cast<uint16_t>(mTime.tm_year + 1900),
                                      static_cast<uint8_t>(mTime.tm_mon + 1),
                                      static_cast<uint8_t>(mTime.tm_mday),
                                      static_cast<uint8_t>(mTime.tm_wday));
            }
            if (timeChanged) {
                modelListener->onTime(static_cast<uint8_t>(mTime.tm_hour),
                                      static_cast<uint8_t>(mTime.tm_min),
                                      static_cast<uint8_t>(mTime.tm_sec));
            }
        } break;

        case CustomMessage::BATTERY: {
            auto* msg = static_cast<CustomMessage::Battery*>(message);
            if (mBatteryLevel != msg->level) {
                mBatteryLevel = msg->level;
                modelListener->onBatteryLevel(mBatteryLevel);
            }
        } break;

        case CustomMessage::GPS_FIX: {
            auto* msg = static_cast<CustomMessage::GpsFix*>(message);
            if (mGpsFix != msg->state) {
                mGpsFix = msg->state;
                modelListener->onGpsFix(mGpsFix);
            }
        } break;

        case CustomMessage::TRACK_STATE_UPDATE: {
            auto* msg = static_cast<CustomMessage::TrackStateUpd*>(message);
            if (mTrackState != msg->state) {
                mTrackState = msg->state;
                modelListener->onTrackState(mTrackState);
            }
        } break;

        case CustomMessage::TRACK_DATA_UPDATE: {
            auto* msg  = static_cast<CustomMessage::TrackDataUpd*>(message);
            mTrackData = msg->data;
            modelListener->onTrackData(mTrackData);
        } break;

        case CustomMessage::LAP_END: {
            auto* msg = static_cast<CustomMessage::LapEnded*>(message);
            modelListener->onLapChanged(static_cast<uint8_t>(msg->lapNum));
        } break;

        case CustomMessage::INTERVALS_PHASE_ALERT: {
            auto* msg              = static_cast<CustomMessage::IntervalsPhaseAlert*>(message);
            mPendingAlertIntervals = msg->intervals;
            modelListener->onIntervalsPhaseAlert();
        } break;

        case CustomMessage::INTERVALS_WORKOUT_COMPLETED: {
            modelListener->onIntervalsWorkoutCompleted();
        } break;

        case CustomMessage::SUMMARY: {
            auto* msg = static_cast<CustomMessage::Summary*>(message);
            if (msg->summary) {
                mActivitySummary = msg->summary;
                modelListener->onActivitySummary(*mActivitySummary);
            }
        } break;

        case CustomMessage::ACCESSORY_STATUS: {
            auto* msg = static_cast<CustomMessage::AccessoryStatusUpd*>(message);
            if (mAccessoryState != msg->state) {
                mAccessoryState = msg->state;
                modelListener->onAccessoryStatus(msg->state, msg->name);
            }
        } break;

        default:
            break;
    }
    return true;
}
