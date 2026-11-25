#include <gui/model/Model.hpp>
#include <cstring>
#include "SDK/Messages/MessageGuard.hpp"
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

void Model::saveRunAlert(uint32_t value)
{
    Settings sett = mSettings;
    RunAlerts::setField(mRunAlertField, value, mUnitsImperial, sett.runAlerts);
    saveSettings(sett);
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

void Model::setLapFromPauseMenu(bool fromPauseMenu)
{
    mLapFromPauseMenu = fromPauseMenu;
}

bool Model::isLapFromPauseMenu() const
{
    return mLapFromPauseMenu;
}

bool Model::isPausedLapTaken() const
{
    return mPausedLapTaken;
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

RunAlerts::StartMode Model::startMode(bool intervalsRequested) const
{
    return RunAlerts::startMode(intervalsRequested, intervalsRequested ? kNoWorkout : mArmedWorkout,
                                mSettings.runAlerts);
}

// Navigation contract (callers must follow):
//
//   intervalsMode = false, Run/Walk off
//       TrackStartConfirmation -> gotoTrackScreen  (TrackFaceTotal)
//
//   intervalsMode = false, Run/Walk on
//       TrackStartConfirmation -> gotoTrackIntervalsCountdown
//       TrackIntervalsCountdown (on confirm / timer expire):
//           gotoTrackIntervalsAlertScreen  (RUN phase)
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
    // An armed workout is followed unless Intervals were chosen; either way
    // it is used up by this run.
    const int16_t workout = intervalsMode ? kNoWorkout : mArmedWorkout;
    // A plain Start runs Run/Walk when it is on; the service makes the same
    // choice from the same settings, so the message stays as it was.
    const bool runWalk = startMode(intervalsMode) == RunAlerts::StartMode::RunWalk;
    mArmedWorkout = kNoWorkout;

    mTrackData.intervalsMode = intervalsMode || runWalk;
    mTrackData.workoutMode   = workout != kNoWorkout;
    if (mTrackData.workoutMode) {
        mMenu.track.set(App::MenuNav::TrackView::ID_WORKOUT_GAUGE);
    } else if (mTrackData.intervalsMode) {
        mMenu.track.set(App::MenuNav::TrackView::ID_INTERVALS);
    }
    mStepCard    = false;
    mWaitingCount = 0;
    mWorkoutFace = WorkoutFace{};
    mWorkoutStep = WorkoutStepInfo{};
    mWorkoutNext = WorkoutStepInfo{};

    if (runWalk) {
        // As Service::startTrack(): the timed RUN phase first, open repeats,
        // no warm-up. The countdown goes straight to the RUN pop-up, before
        // any INTERVALS_PHASE_ALERT, so it gets the same data now.
        Track::IntervalsData& iv = mTrackData.intervals;
        iv               = Track::IntervalsData{};
        iv.phase         = Track::IntervalsPhase::RUN;
        iv.metric        = Track::IntervalsMetric::TIME_REMAINING;
        iv.repeat        = 1;
        iv.phaseTimerSec = static_cast<time_t>(mSettings.runAlerts.runWalkRunSec);
        iv.runWalk       = true;
        mPendingAlertIntervals = iv;
    } else if (intervalsMode) {
        const Settings::Intervals& cfg = mSettings.intervals;
        Track::IntervalsData& iv       = mTrackData.intervals;

        iv = Track::IntervalsData{};
        // totalRepeats is the literal repeat count chosen by the user; 0 == 'Open'.
        // Must mirror the calculation in Service::startTrack().
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

            // warmUp=false: GUI navigates directly to TrackIntervalsAlertScreen
            // before any INTERVALS_PHASE_ALERT arrives from Service, so pre-populate
            // mPendingAlertIntervals with the same data the alert screen will display.
            mPendingAlertIntervals = iv;
        }
    }

    SDK::send_msg<CustomMessage::TrackStart>(mKernel, intervalsMode, workout);
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
    // The track faces open the lap pop-up now, before LAP_END comes back.
    keepLap();
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


// Structured workouts

uint32_t Model::getTodayYmd() const
{
    if (mTime.tm_year + 1900 < 2020) {
        return 0;
    }
    return static_cast<uint32_t>(mTime.tm_year + 1900) * 10000u
         + static_cast<uint32_t>(mTime.tm_mon + 1) * 100u
         + static_cast<uint32_t>(mTime.tm_mday);
}

const SDK::Workout::Library* Model::getWorkoutLibrary() const
{
    return mWorkoutLibrary;
}

bool Model::takeTodayPrompt()
{
    if (mTodayPrompted || mWorkoutLibrary == nullptr || isTrackActive()
        || mTodayWorkout == CustomMessage::WorkoutList::kNoToday
        || mTodayWorkout >= mWorkoutLibrary->size()) {
        return false;
    }
    mTodayPrompted = true;
    return true;
}

uint16_t Model::getTodayWorkout() const
{
    return mTodayWorkout;
}

void Model::setChosenWorkout(uint16_t index)
{
    mChosenWorkout = index;
}

uint16_t Model::getChosenWorkout() const
{
    return mChosenWorkout;
}

void Model::armWorkout(int16_t index)
{
    mArmedWorkout = index;
}

int16_t Model::getArmedWorkout() const
{
    return mArmedWorkout;
}

void Model::requestWorkoutDetails(uint16_t index)
{
    mWorkoutDetails = nullptr;
    SDK::send_msg<CustomMessage::WorkoutDetailsRequest>(mKernel, index);
}

const SDK::Workout::Program* Model::getWorkoutDetails() const
{
    return mWorkoutDetails;
}

void Model::endWorkout()
{
    // As trackStart() sets it: the Track screen should not wait for the
    // service's next update to drop the workout faces.
    mTrackData.workoutMode = false;
    SDK::send_msg<CustomMessage::WorkoutEnd>(mKernel);
}

const Model::WorkoutFace& Model::getWorkoutFace() const
{
    return mWorkoutFace;
}

const Model::WorkoutStepInfo& Model::getWorkoutStep(bool next) const
{
    return next ? mWorkoutNext : mWorkoutStep;
}

bool Model::takeStepCard()
{
    const bool card = mStepCard;
    mStepCard = false;
    return card;
}

void Model::takePopup(Popup popup)
{
    for (uint8_t i = 0; i < mWaitingCount; ++i) {
        if (mWaiting[i] == popup) {
            for (uint8_t j = i + 1; j < mWaitingCount; ++j) {
                mWaiting[j - 1] = mWaiting[j];
            }
            --mWaitingCount;
            return;
        }
    }
}

void Model::waitPopup(Popup popup)
{
    // Once each: one already waiting shows the latest data when it shows.
    for (uint8_t i = 0; i < mWaitingCount; ++i) {
        if (mWaiting[i] == popup) {
            return;
        }
    }
    if (mWaitingCount < kPopups) {
        mWaiting[mWaitingCount++] = popup;
    }
}

void Model::keepLap()
{
    mLap.num      = mTrackData.lapNum;
    mLap.distance = mTrackData.lapDistance;
    mLap.time     = mTrackData.lapTime;
    mLap.pace     = mTrackData.lapPace;
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
            mTimeFormat12h     = msg->timeFormat12h;
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
                if (mTrackState != Track::State::PAUSED) {
                    mPausedLapTaken = false;
                }
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
            if (mTrackState == Track::State::PAUSED) {
                mPausedLapTaken = true;
            }
            // The track data still holds the lap just ended.
            keepLap();
            waitPopup(Popup::Lap);
            modelListener->onLapChanged(msg->lapNum);
        } break;

        case CustomMessage::INTERVALS_PHASE_ALERT: {
            LOG_DEBUG("INTERVALS_PHASE_ALERT\n");
            auto* msg          = static_cast<CustomMessage::IntervalsPhaseAlert*>(message);
            mPendingAlertIntervals = msg->intervals;
            waitPopup(Popup::Phase);
            modelListener->onIntervalsPhaseAlert();
        } break;

        case CustomMessage::RUN_ALERT: {
            LOG_DEBUG("RUN_ALERT\n");
            auto* msg       = static_cast<CustomMessage::RunAlert*>(message);
            mRunAlert.kind  = static_cast<Track::RunAlertKind>(msg->kind);
            mRunAlert.value = msg->value;
            waitPopup(Popup::RunAlert);
            modelListener->onRunAlert();
        } break;

        case CustomMessage::INTERVALS_WORKOUT_COMPLETED: {
            LOG_DEBUG("INTERVALS_WORKOUT_COMPLETED\n");
            waitPopup(Popup::Completed);
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

        case CustomMessage::WORKOUT_LIST: {
            LOG_DEBUG("WORKOUT_LIST\n");
            auto* msg       = static_cast<CustomMessage::WorkoutList*>(message);
            mWorkoutLibrary = msg->library;
            mTodayWorkout   = msg->today;
            modelListener->onWorkoutList();
        } break;

        case CustomMessage::WORKOUT_DETAILS: {
            LOG_DEBUG("WORKOUT_DETAILS\n");
            auto* msg = static_cast<CustomMessage::WorkoutDetails*>(message);
            // An answer to an earlier request, for a workout since left.
            if (msg->index != mChosenWorkout) {
                break;
            }
            mWorkoutDetails = msg->program;
            modelListener->onWorkoutDetails();
        } break;

        case CustomMessage::WORKOUT_DATA: {
            auto* msg = static_cast<CustomMessage::WorkoutData*>(message);
            mWorkoutFace.remainingMs = msg->remainingMs;
            mWorkoutFace.remainingCm = msg->remainingCm;
            mWorkoutFace.stepTimeMs  = msg->stepTimeMs;
            mWorkoutFace.rep         = msg->rep;
            mWorkoutFace.reps        = msg->reps;
            mWorkoutFace.value       = msg->value;
            mWorkoutFace.arc         = msg->arc;
            mWorkoutFace.stepEnd     = msg->stepEnd;
            mWorkoutFace.target      = msg->target;
            mWorkoutFace.zone        = msg->zone;
            mWorkoutFace.intensity   = msg->intensity;
            mWorkoutFace.leadIn      = msg->leadIn;
            modelListener->onWorkoutData(mWorkoutFace);
        } break;

        case CustomMessage::WORKOUT_STEP: {
            LOG_DEBUG("WORKOUT_STEP\n");
            auto* msg = static_cast<CustomMessage::WorkoutStep*>(message);
            WorkoutStepInfo& step = msg->next ? mWorkoutNext : mWorkoutStep;
            std::memcpy(step.notes, msg->notes, sizeof(step.notes));
            step.notes[sizeof(step.notes) - 1] = '\0';
            std::memcpy(step.duration, msg->duration, sizeof(step.duration));
            step.duration[sizeof(step.duration) - 1] = '\0';
            std::memcpy(step.target, msg->target, sizeof(step.target));
            step.target[sizeof(step.target) - 1] = '\0';
            step.rep       = msg->rep;
            step.reps      = msg->reps;
            step.intensity = msg->intensity;
            step.none      = msg->none;
            if (!msg->next) {
                mStepCard = true;   // the card shows once the Track screen sees it
            }
            modelListener->onWorkoutStep(msg->next);
        } break;

        case CustomMessage::ACCESSORY_STATUS: {
            auto* msg = static_cast<CustomMessage::AccessoryStatusUpd*>(message);
            LOG_DEBUG("ACCESSORY_STATUS %u\n", msg->state);
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
