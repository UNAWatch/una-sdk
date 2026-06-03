#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "SDK/Kernel/KernelProviderGUI.hpp"
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

#define LOG_MODULE_PRX      "Model"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

Model::Model()
    : modelListener(nullptr)
    , mKernel(SDK::KernelProviderGUI::GetInstance().getKernel())
    , mSrvSender(mKernel)
{
    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(this);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(this);
    memcpy(mHrThresholds, CustomMessage::kHrThresholdsDefault, sizeof(mHrThresholds));

#if defined(SIMULATOR)
    std::string fsPath = SDK::Simulator::KernelHolder::Get().getFsPath();
    LOG_INFO("Simulator.\n");
    LOG_INFO("FS path: [%s].\n", fsPath.c_str());
    LOG_INFO("Buttons: 1=L1 2=L2 3=R1 4=R2\n\n");
#endif
}

// Controls

FrontendApplication& Model::application()
{
    return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
}

App::MenuNav::Nav& Model::menu()
{
    return mMenu;
}

void Model::invalidate()
{
    mInvalidate = true;
}

void Model::tick()
{
#if defined(SIMULATOR)
    decIdleTimer();
#else
    if (mIsRunning) {
        decIdleTimer();
    }
#endif
    if (mInvalidate) {
        mInvalidate = false;
        application().invalidate();
    }
}

void Model::handleKeyEvent(uint8_t key)
{
    LOG_DEBUG("key = %c\n", static_cast<char>(key));
    if (isAnyKeyPressed(key)) {
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

    SDK::TouchGFXCommandProcessor::GetInstance().setAppLifeCycleCallback(nullptr);
    SDK::TouchGFXCommandProcessor::GetInstance().setCustomMessageHandler(nullptr);

    mKernel.sys.exit();
    // On simulator sys.exit() only sets a flag -- the current tick completes normally.
}


// Date/Time

void Model::getDate(uint8_t& month, uint8_t& day, uint8_t& weekday)
{
    month   = static_cast<uint8_t>(mTime.tm_mon);
    day     = static_cast<uint8_t>(mTime.tm_mday);
    weekday = static_cast<uint8_t>(mTime.tm_wday);
}

void Model::getTime(uint8_t& h, uint8_t& m, uint8_t& s)
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
    mSrvSender.settingsSave(mSettings);
}


// GPS

bool Model::hasGpsFix() const
{
    return mGpsFix;
}


// Track

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

// Navigation contract (callers must follow):
//
//   intervalsMode = false
//       TrackStartConfirmation -> gotoTrackScreen  (TrackFaceTotal)
//
//   intervalsMode = true
//       TrackStartConfirmationIntervals -> gotoTrackIntervalsCountdown
//       TrackIntervalsCountdown (on confirm / timer expire):
//           warmUp = true  -> gotoTrackScreen  (TrackFaceIntervals, WARM_UP phase)
//           warmUp = false -> gotoTrackIntervalsAlertScreen  (RUN phase)
//
// mTrackData.intervals is pre-populated from settings so that the
// destination screen has valid data before the first TRACK_DATA_UPDATE
// arrives from the Service (~1 s after start).
void Model::trackStart(bool intervalsMode)
{
    mTrackData.intervalsMode = intervalsMode;

    if (intervalsMode) {
        const Settings::Intervals& cfg = mSettings.intervals;
        Track::IntervalsData& iv       = mTrackData.intervals;

        iv = Track::IntervalsData{};
        // totalRepeats = repeatsNum + 1 (base cycle + N additional repeats)
        // Must mirror the calculation in Service::startTrack().
        iv.totalRepeats = cfg.repeatsNum + 1;

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

            // warmUp=false: GUI navigates directly to TrackIntervalsAlertScreen
            // before any INTERVALS_PHASE_ALERT arrives from Service, so pre-populate
            // mPendingAlertIntervals with the same data the alert screen will display.
            mPendingAlertIntervals = iv;
        }
    }

    mSrvSender.trackStart(intervalsMode);
}

void Model::intervalsNextPhase()
{
    mSrvSender.intervalsNextPhase();
}

bool Model::isTrackActive() const
{
    return mTrackState != Track::State::INACTIVE;
}

void Model::trackPause()
{
    mSrvSender.trackPause();
}

void Model::trackResume()
{
    mSrvSender.trackResume();
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
    mSrvSender.manualLap();
}

void Model::saveTrack()
{
    mSrvSender.trackStop(false);
}

void Model::discardTrack()
{
    mSrvSender.trackStop(true);
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
        if (--mIdleTimer == 0) {
            modelListener->onIdleTimeout();
        }
    }
}

bool Model::isAnyKeyPressed(uint8_t key) const
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

void Model::onResume()
{
    mIsRunning = true;
    resetIdleTimer();
    invalidate();
}

void Model::onSuspend()
{
    mIsRunning = false;
}

void Model::onStop()
{
    LOG_INFO("Force exit from the application\n");
}


// ICustomMessageHandler

bool Model::customMessageHandler(SDK::MessageBase* message)
{
    switch (message->getType()) {
        case CustomMessage::SETTINGS_UPDATE: {
            LOG_DEBUG("SETTINGS_UPDATE\n");
            auto* msg          = static_cast<CustomMessage::SettingsUpd*>(message);
            mSettings          = msg->settings;
            mUnitsImperial     = msg->unitsImperial;
            memcpy(mHrThresholds, msg->hrThresholds, sizeof(mHrThresholds));
            mHrThresholdsCount = msg->hrThresholdsCount;
            modelListener->onSettings(mSettings);
        } break;

        case CustomMessage::LOCAL_TIME: {
            auto* msg = static_cast<CustomMessage::Time*>(message);
            std::tm newTime = msg->localTime;
            LOG_DEBUG("LOCAL_TIME %04u-%02u-%02u %02u:%02u:%02u\n",
                newTime.tm_year + 1900, newTime.tm_mon + 1, newTime.tm_mday,
                newTime.tm_hour, newTime.tm_min, newTime.tm_sec);

            const bool dateChanged = newTime.tm_year  != mTime.tm_year  ||
                                     newTime.tm_mon   != mTime.tm_mon   ||
                                     newTime.tm_mday  != mTime.tm_mday;
            const bool timeChanged = newTime.tm_hour  != mTime.tm_hour  ||
                                     newTime.tm_min   != mTime.tm_min   ||
                                     newTime.tm_sec   != mTime.tm_sec;
            mTime = newTime;

            if (dateChanged) {
                modelListener->onDate(mTime.tm_year + 1900, mTime.tm_mon + 1,
                                      mTime.tm_mday, mTime.tm_wday);
            }
            if (timeChanged) {
                modelListener->onTime(mTime.tm_hour, mTime.tm_min, mTime.tm_sec);
            }
        } break;

        case CustomMessage::BATTERY: {
            auto* msg = static_cast<CustomMessage::Battery*>(message);
            LOG_DEBUG("BATTERY Level %u\n", msg->level);
            if (mBatteryLevel != msg->level) {
                mBatteryLevel = msg->level;
                modelListener->onBatteryLevel(mBatteryLevel);
            }
        } break;

        case CustomMessage::GPS_FIX: {
            auto* msg = static_cast<CustomMessage::GpsFix*>(message);
            LOG_DEBUG("GPS_FIX %u\n", msg->state);
            if (mGpsFix != msg->state) {
                mGpsFix = msg->state;
                modelListener->onGpsFix(mGpsFix);
            }
        } break;

        case CustomMessage::TRACK_STATE_UPDATE: {
            LOG_DEBUG("TRACK_STATE_UPDATE\n");
            auto* msg = static_cast<CustomMessage::TrackStateUpd*>(message);
            if (mTrackState != msg->state) {
                mTrackState = msg->state;
                modelListener->onTrackState(mTrackState);
            }
        } break;

        case CustomMessage::TRACK_DATA_UPDATE: {
            LOG_DEBUG("TRACK_DATA_UPDATE\n");
            auto* msg  = static_cast<CustomMessage::TrackDataUpd*>(message);
            mTrackData = msg->data;
            modelListener->onTrackData(mTrackData);
        } break;

        case CustomMessage::LAP_END: {
            LOG_DEBUG("LAP_END\n");
            auto* msg = static_cast<CustomMessage::LapEnded*>(message);
            modelListener->onLapChanged(msg->lapNum);
        } break;

        case CustomMessage::INTERVALS_PHASE_ALERT: {
            LOG_DEBUG("INTERVALS_PHASE_ALERT\n");
            auto* msg          = static_cast<CustomMessage::IntervalsPhaseAlert*>(message);
            mPendingAlertIntervals = msg->intervals;
            modelListener->onIntervalsPhaseAlert();
        } break;

        case CustomMessage::INTERVALS_WORKOUT_COMPLETED: {
            LOG_DEBUG("INTERVALS_WORKOUT_COMPLETED\n");
            modelListener->onIntervalsWorkoutCompleted();
        } break;

        case CustomMessage::SUMMARY: {
            LOG_DEBUG("SUMMARY\n");
            auto* msg = static_cast<CustomMessage::Summary*>(message);
            if (msg->summary) {
                mActivitySummary = msg->summary;
                modelListener->onActivitySummary(*mActivitySummary);
            }
        } break;

        default:
            break;
    }

    return true;
}
