#include <gui/track_screen/TrackView.hpp>
#include <SDK/Utils/Utils.hpp>
#include <cstring>

using FaceId = App::MenuNav::TrackView::Id;

TrackView::TrackView()
{

}

void TrackView::setupScreen()
{
    TrackViewBase::setupScreen();

    buttons.setL1(Buttons::NONE);
    buttons.setL2(Buttons::NONE);
    buttons.setR1(Buttons::NONE);
    buttons.setR2(Buttons::AMBER);

    scrollIndicator.setConfig(ScrollIndicator::kSmall);

    add(mWorkoutGauge);
    add(mWorkoutStep);
    add(mWorkoutNext);
    mWorkoutGauge.setVisible(false);
    mWorkoutStep.setVisible(false);
    mWorkoutNext.setVisible(false);
}

void TrackView::tearDownScreen()
{
    TrackViewBase::tearDownScreen();
}

const uint16_t* TrackView::faces(uint16_t& count) const
{
    static const uint16_t kWorkout[] = {
        FaceId::ID_WORKOUT_GAUGE, FaceId::ID_WORKOUT_STEP, FaceId::ID_WORKOUT_NEXT,
        FaceId::ID_TRACK1, FaceId::ID_TRACK2, FaceId::ID_TRACK3,
    };
    static const uint16_t kIntervals[] = {
        FaceId::ID_INTERVALS, FaceId::ID_TRACK1, FaceId::ID_TRACK2, FaceId::ID_TRACK3,
    };
    static const uint16_t kFree[] = { FaceId::ID_TRACK1, FaceId::ID_TRACK2, FaceId::ID_TRACK3 };

    switch (mMode) {
    case Mode::Workout:
        count = sizeof(kWorkout) / sizeof(kWorkout[0]);
        return kWorkout;
    case Mode::Intervals:
        count = sizeof(kIntervals) / sizeof(kIntervals[0]);
        return kIntervals;
    default:
        count = sizeof(kFree) / sizeof(kFree[0]);
        return kFree;
    }
}

uint16_t TrackView::faceIndex(uint16_t id) const
{
    uint16_t count = 0;
    const uint16_t* list = faces(count);
    for (uint16_t i = 0; i < count; ++i) {
        if (list[i] == id) {
            return i;
        }
    }
    return count;
}

void TrackView::setMode(Mode mode)
{
    mMode = mode;
    uint16_t count = 0;
    faces(count);
    scrollIndicator.setCount(count);
}

void TrackView::setPositionId(uint16_t id)
{
    // A face the run does not have (Intervals ended, a workout completed)
    // falls back to the first one it has.
    uint16_t count = 0;
    const uint16_t* list = faces(count);
    uint16_t index = faceIndex(id);
    if (index >= count) {
        index = 0;
    }
    id = list[index];
    mCurrentFaceId = id;
    scrollIndicator.setActiveId(index);

    trackFaceIntervals.setVisible(id == FaceId::ID_INTERVALS);
    trackFaceTotal.setVisible(id == FaceId::ID_TRACK1);
    trackFaceLap.setVisible(id == FaceId::ID_TRACK2);
    trackFaceStatus.setVisible(id == FaceId::ID_TRACK3);
    mWorkoutGauge.setVisible(id == FaceId::ID_WORKOUT_GAUGE);
    mWorkoutStep.setVisible(id == FaceId::ID_WORKOUT_STEP);
    mWorkoutNext.setVisible(id == FaceId::ID_WORKOUT_NEXT);

    trackFaceIntervals.invalidate();
    trackFaceTotal.invalidate();
    trackFaceLap.invalidate();
    trackFaceStatus.invalidate();
    mWorkoutGauge.invalidate();
    mWorkoutStep.invalidate();
    mWorkoutNext.invalidate();
}

uint16_t TrackView::getPositionId()
{
    return mCurrentFaceId;
}

void TrackView::setConfig(bool isImperial, const uint8_t* thresholds, uint8_t thresholdCount)
{
    mIsImperial        = isImperial;
    mHrThresholdCount  = thresholdCount < App::Config::kHrThresholdsCount ? 
                            thresholdCount : static_cast<uint8_t>(App::Config::kHrThresholdsCount);
    memcpy(mHrThresholds, thresholds, mHrThresholdCount);
}

void TrackView::setTimeFormat(bool is12Hour)
{
    mIs12Hour = is12Hour;
}

void TrackView::setTrackData(const Track::Data& data)
{
    auto paceConv = [this](float secPerM) -> float {
        if (secPerM < 1e-6f) return 0.0f;
        const float secPerKm = secPerM * 1000.0f;
        return mIsImperial ? secPerKm / SDK::Utils::kmToMiles(1.0f) : secPerKm;
    };

    auto distConv = [this](float metres) -> float {
        const float km = metres / 1000.0f;
        return mIsImperial ? SDK::Utils::kmToMiles(km) : km;
    };

    trackFaceTotal.setPace(paceConv(data.pace));
    trackFaceTotal.setDistance(distConv(data.distance), mIsImperial);
    trackFaceTotal.setTimer(data.totalTime);

    mLivePace = paceConv(data.pace);
    mLapPace  = paceConv(data.lapPace);
    mLapHr    = data.avgLapHR;
    mWorkoutStep.setAverages(mLapPace, mLapHr, mHeartRateStep);

    trackFaceLap.setPace(paceConv(data.lapPace));
    trackFaceLap.setDistance(distConv(data.lapDistance));
    trackFaceLap.setTimer(data.lapTime);
    trackFaceLap.setHR(data.hr, mHrThresholds, mHrThresholdCount);

    if (mMode == Mode::Intervals) {
        const Track::IntervalsData& iv = data.intervals;

        trackFaceIntervals.setPhase(iv.phase, iv.repeat, iv.totalRepeats);

        if (iv.metric == Track::IntervalsMetric::DISTANCE) {
            trackFaceIntervals.setPhaseDistance(distConv(iv.distRemaining), mIsImperial);
        } else {
            trackFaceIntervals.setPhaseTime(iv.phaseTimerSec, iv.metric);
        }

        trackFaceIntervals.setPace(paceConv(data.pace));
        trackFaceIntervals.setHR(data.hr);
    }

    mHrSource = data.hrSource;
    updateHrIcon();
}

void TrackView::setTime(uint8_t h, uint8_t m)
{
    trackFaceStatus.setTime(h, m, mIs12Hour);
}

void TrackView::setBatteryLevel(uint8_t level)
{
    trackFaceStatus.setBatteryLevel(level);
}

void TrackView::setWorkoutFace(const Model::WorkoutFace& face)
{
    mHeartRateStep = static_cast<SDK::Workout::Target>(face.target) == SDK::Workout::Target::HeartRate;
    mWorkoutGauge.set(face, mIsImperial, mLivePace);
    mWorkoutStep.setRepeat(face.rep, face.reps);
    mWorkoutStep.setAverages(mLapPace, mLapHr, mHeartRateStep);
}

void TrackView::setWorkoutStep(const Model::WorkoutStepInfo& step)
{
    mWorkoutStep.setStep(step);
    mWorkoutStep.setRepeat(step.rep, step.reps);
}

void TrackView::setWorkoutNext(const Model::WorkoutStepInfo& step)
{
    mWorkoutNext.showNext(step);
}

void TrackView::handleKeyEvent(uint8_t key)
{
    uint16_t count = 0;
    const uint16_t* list = faces(count);
    const uint16_t index = faceIndex(mCurrentFaceId);

    if (key == SDK::GUI::Button::L1) {
        setPositionId(list[index == 0 || index >= count ? count - 1 : index - 1]);
    }

    if (key == SDK::GUI::Button::L2) {
        setPositionId(list[index + 1 >= count ? 0 : index + 1]);
    }

    if (key == SDK::GUI::Button::R1) {
        application().gotoTrackActionScreenNoTransition();
    }

    if (key == SDK::GUI::Button::R2) {
        // In Intervals or a structured workout the lap button moves to the next
        // phase or step, on ANY face -- laps are step-driven, not manual. Gate on
        // the mode, not the face shown (which the user can scroll away from). When
        // the workout completes the Service drops the mode, so R2 then records a
        // manual lap. A free run always records a manual lap.
        if (mMode != Mode::Free) {
            presenter->intervalsNextPhase();
        } else {
            presenter->saveLap();
            application().gotoTrackLapScreenNoTransition();
        }
    }
}

void TrackView::setGpsFix(bool state)
{
    trackFaceStatus.setGps(SDK::GUI::SensorStatusRow::gpsState(state));
}

void TrackView::setAccessoryStatus(uint8_t state)
{
    mAccessoryState = state;
    updateHrIcon();
}

void TrackView::updateHrIcon()
{
    // In-activity: icon follows the live HR source, not the raw link state.
    trackFaceStatus.setHr(SDK::GUI::SensorStatusRow::hrStateFromSource(
            mAccessoryState, mHrSource));
}
