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

// mTrackData.intervals is pre-populated from settings so that the destination
// screen has valid data before the first TRACK_DATA_UPDATE arrives from the
// Service (~1 s after start). Mirrors Service::startTrack().
void Model::trackStart(bool intervalsMode)
{
    mLapCompare.reset();   // a new run: no lap before its first
    mAutoJoinsSeen = 0;
    mLastCal       = Track::OnTrack::CAL_NONE;
    mTrackData.onTrack = Track::OnTrack{};
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

void Model::setLane(uint8_t lane)
{
    if (!isTrackActive()) {
        mPreRun.lane = lane;   // the start screen shows it at once
    } else {
        mTrackData.onTrack.lane = lane;
    }
    SDK::send_msg<CustomMessage::SetLane>(mKernel, lane);
}

uint8_t Model::currentLane() const
{
    return isTrackActive() ? mTrackData.onTrack.lane : mPreRun.lane;
}

void Model::requestTrackList()
{
    mTracks.received = false;
    SDK::send_msg<CustomMessage::TrackListReq>(mKernel);
}

void Model::forgetTrack(uint8_t number)
{
    SDK::send_msg<CustomMessage::ForgetTrack>(mKernel, number);
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
    mLap.delta    = mLapCompare.add(mLap.num, static_cast<uint32_t>(mLap.time), mLap.distance);
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
                if (mTrackState != Track::State::PAUSED) {
                    mPausedLapTaken = false;
                }
                modelListener->onTrackState(mTrackState);
            }
        } break;

        case CustomMessage::TRACK_DATA_UPDATE: {
            auto* msg  = static_cast<CustomMessage::TrackDataUpd*>(message);
            mTrackData = msg->data;
            // A known track the run joined by itself: the notice, once. Read
            // from the live state, so a lost message costs nothing.
            const Track::OnTrack& o = mTrackData.onTrack;
            if (o.autoJoins > mAutoJoinsSeen) {
                mAutoJoinsSeen       = o.autoJoins;
                mNotice.kind         = NoticeInfo::Kind::Joined;
                mNotice.trackNumber  = o.trackNumber;
                mNotice.lane         = o.lane;
                waitPopup(Popup::Notice);
            }
            // A new track's calibration locked: what it saves as.
            if (o.cal == Track::OnTrack::LOCKED && mLastCal != Track::OnTrack::LOCKED && o.trackNumber == 0
                && mLastCal != Track::OnTrack::CAL_NONE) {
                mNotice.kind      = NoticeInfo::Kind::Locked;
                mNotice.newNumber = o.newTrackNumber;
                waitPopup(Popup::Notice);
            }
            mLastCal = o.cal;
            modelListener->onTrackData(mTrackData);
        } break;

        case CustomMessage::PRE_RUN_TRACK: {
            auto* msg = static_cast<CustomMessage::PreRunTrack*>(message);
            mPreRun.status      = msg->status;
            mPreRun.trackNumber = msg->trackNumber;
            mPreRun.lane        = msg->lane;
            mPreRun.weakGps     = msg->weakGps;
            mPreRun.uses        = msg->uses;
            modelListener->onPreRun();
        } break;

        case CustomMessage::TRACK_LIST: {
            auto* msg = static_cast<CustomMessage::TrackList*>(message);
            mTracks.count     = msg->count <= CustomMessage::TrackList::kMax ? msg->count : CustomMessage::TrackList::kMax;
            mTracks.tzOffsetS = msg->tzOffsetS;
            for (uint8_t i = 0; i < mTracks.count; ++i) {
                mTracks.tracks[i] = msg->tracks[i];
            }
            mTracks.received = true;
            modelListener->onTrackList();
        } break;

        case CustomMessage::LAP_END: {
            auto* msg = static_cast<CustomMessage::LapEnded*>(message);
            if (mTrackState == Track::State::PAUSED) {
                mPausedLapTaken = true;
            }
            // The track data still holds the lap just ended.
            keepLap();
            waitPopup(Popup::Lap);
            modelListener->onLapChanged(static_cast<uint8_t>(msg->lapNum));
        } break;

        case CustomMessage::INTERVALS_PHASE_ALERT: {
            auto* msg              = static_cast<CustomMessage::IntervalsPhaseAlert*>(message);
            mPendingAlertIntervals = msg->intervals;
            waitPopup(Popup::Phase);
            modelListener->onIntervalsPhaseAlert();
        } break;

        case CustomMessage::RUN_ALERT: {
            auto* msg       = static_cast<CustomMessage::RunAlert*>(message);
            mRunAlert.kind  = static_cast<Track::RunAlertKind>(msg->kind);
            mRunAlert.value = msg->value;
            waitPopup(Popup::RunAlert);
            modelListener->onRunAlert();
        } break;

        case CustomMessage::INTERVALS_WORKOUT_COMPLETED: {
            waitPopup(Popup::Completed);
            modelListener->onIntervalsWorkoutCompleted();
        } break;

        case CustomMessage::SUMMARY: {
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
            memcpy(step.notes, msg->notes, sizeof(step.notes));
            step.notes[sizeof(step.notes) - 1] = '\0';
            memcpy(step.duration, msg->duration, sizeof(step.duration));
            step.duration[sizeof(step.duration) - 1] = '\0';
            memcpy(step.target, msg->target, sizeof(step.target));
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
