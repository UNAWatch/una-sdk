/**
 ******************************************************************************
 * @file    TrackScreen.cpp
 * @brief   The live activity screen (see TrackScreen.hpp).
 ******************************************************************************
 */

#include "gui/screens/TrackScreen.hpp"
#include "gui/screens/ScreenManager.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"
#include "gui/Format.hpp"
#include "gui/Strings.hpp"
#include "gui/TrackText.hpp"
#include "gui/theme/Kit.hpp"

#include <cstring>

#include "SDK/Utils/ClockTime.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
// Status face clock geometry (TrackFaceStatus.hpp).
constexpr int32_t kTimeY       = 63;
constexpr int32_t kMeridiemY   = 105;
constexpr int32_t kMeridiemGap = 5;

// Interval phase accents (TrackFaceIntervals).
constexpr uint32_t kIvNeutral = Color::WHITE;
constexpr uint32_t kIvRun     = Color::CYAN;
constexpr uint32_t kIvRest    = Color::WHITE;  // amber means too slow on a workout

/// @p runWalk: Run/Walk, whose REST phase is a walk.
const char* phaseTitle(Track::IntervalsPhase phase, bool runWalk)
{
    switch (phase) {
        case Track::IntervalsPhase::RUN:       return "RUN";
        case Track::IntervalsPhase::REST:      return runWalk ? "WALK" : "REST";
        case Track::IntervalsPhase::COOL_DOWN: return "COOL DOWN";
        default:                               return "WARM UP";
    }
}

void setHidden(lv_obj_t* obj, bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

// Lap clock face (Track Run's design kit, theme/Kit.hpp).
constexpr int32_t kHeaderY       = 37;
constexpr int32_t kClockBaseY    = 110;   // 40 px from an hour on the same baseline
constexpr int32_t kLastLabelY    = 121;
constexpr int32_t kLastLapY      = 115;
constexpr int32_t kChipY         = 117;
constexpr int32_t kChipH         = 24;
constexpr int32_t kChipPadX      = 8;
constexpr int32_t kDividerY      = 148;
constexpr int32_t kLapRowY       = 151;   // this lap's metres and the pace
constexpr int32_t kUnitY         = 154;
constexpr int32_t kSplitLineY    = 153;
constexpr int32_t kBadgeY        = 176;   // Kit::LaneBadge, 20 x 24
constexpr int32_t kFooterY       = 179;
constexpr float   kRingLapM      = 400.0f; // The ring's lap with no distance auto-lap
constexpr uint8_t kCalLockLaps   = 4;      // Laps a new track's calibration takes

lv_obj_t* rowText(lv_obj_t* parent, F f, const char* s, int32_t y, uint32_t color)
{
    lv_obj_t* l = Kit::text(parent, f, s, color);
    lv_obj_set_y(l, y);
    return l;
}

void setText(lv_obj_t* label, const char* text)
{
    if (std::strcmp(lv_label_get_text(label), text) != 0) {
        lv_label_set_text(label, text);
    }
}

/// "1:21", or "1:02:03" from an hour.
void clockText(char* buf, size_t n, uint32_t s)
{
    if (s >= 3600u) {
        std::snprintf(buf, n, "%u:%02u:%02u", static_cast<unsigned>(s / 3600u),
                      static_cast<unsigned>(s / 60u % 60u), static_cast<unsigned>(s % 60u));
    } else {
        std::snprintf(buf, n, "%u:%02u", static_cast<unsigned>(s / 60u), static_cast<unsigned>(s % 60u));
    }
}
} // namespace

TrackScreen::TrackScreen(Model& model)
    : Screen(model)
    , mLaneHold(model)
{
}

void TrackScreen::build()
{
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::AMBER);

    buildFaceStatus();
    buildFaceClock();
    buildFaceLap();
    buildFacePace();
    buildFaceTotal();
    buildFaceIntervals();

    mWorkoutGauge = std::make_unique<WorkoutFaceGauge>(mRoot);
    mWorkoutStep  = std::make_unique<WorkoutFaceStep>(mRoot);
    mWorkoutNext  = std::make_unique<WorkoutStepCard>(mRoot);
}

void TrackScreen::buildFaceIntervals()
{
    lv_obj_t* f = mFaceIntervals = Theme::container(mRoot, 0, 0, 240, 240);
    mIvRepeats     = Theme::label(f, F::Italic18, "", 80, 202, 80);
    mIvRunIcon     = Theme::image(f, &img_runningman_46x46, 97, 167);
    mIvHr          = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 162, 160);
    mIvPace        = Theme::label(f, F::SemiBold35, Strings::kNoValue, 40, 162, 160);
    mIvHeartIcon   = Theme::image(f, &img_heart_30x30, 105, 137);
    mIvPaceIcon    = Theme::image(f, &img_pace_30x30, 105, 137);
    mIntervalsTimer = std::make_unique<Widgets::IntervalsTimer>(f, 25, 41);
    mIntervalsTitle = std::make_unique<Widgets::Title>(f, "WARM UP");
}

void TrackScreen::buildFaceTotal()
{
    lv_obj_t* f = mFaceTotal = Theme::container(mRoot, 0, 0, 240, 240);
    Theme::label(f, F::Italic18, "Pace", 0, 10, 240);
    // Value boxes are wider than the design's so a slow pace ("22:00") is not clipped.
    mPaceValue = Theme::label(f, F::SemiBold40, Strings::kNoValue, 0, 30, 240);
    Theme::hline(f, 35, 79, 170);
    Theme::label(f, F::Italic18, "Distance", 0, 88, 240);
    mDistanceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 53, 111, 134);
    mDistanceUnits = Theme::label(f, F::Regular18, Strings::kKm, 178, 129, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 159, 170);
    mTimerValue = Theme::label(f, F::SemiBold35, "0:00:00", 35, 162, 170);
    Theme::label(f, F::Italic18, "Timer", 0, 204, 240);
}

void TrackScreen::buildFaceLap()
{
    lv_obj_t* f = mFaceLap = Theme::container(mRoot, 0, 0, 240, 240);
    mHrZone  = std::make_unique<Widgets::HeartRateZone>(f, 15, 4);
    mHrValue = Theme::label(f, F::SemiBold40, Strings::kNoValue, 75, 29, 90);
    Theme::label(f, F::Regular18, "HR", 160, 50, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 79, 170);
    Theme::label(f, F::Italic18, "Lap Pace", 20, 88, 88);
    // Same centres as the design (68 and 173) but wide enough for "22:00".
    mLapPaceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 9, 111, 118);
    Theme::vline(f, 119, 79, 80);
    Theme::label(f, F::Italic18, "Lap Dist.", 132, 88, 88);
    mLapDistValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 114, 111, 118);
    Theme::hline(f, 35, 159, 170);
    mLapTimerValue = Theme::label(f, F::SemiBold35, "0:00:00", 35, 162, 170);
    Theme::label(f, F::Italic18, "Lap Time", 75, 204, 91);
}

void TrackScreen::buildFacePace()
{
    lv_obj_t* f = mFacePace = Theme::container(mRoot, 0, 0, 240, 240);
    // Top: the totals face's distance row (same x, widths and fonts), raised
    // into the top slot under the caption, at the totals face's top caption
    // and value y. "99.99 km" stays at least 11 px inside the round display.
    Theme::label(f, F::Italic18, "Distance", 0, 10, 240);
    mPfDistanceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 53, 30, 134);
    mPfDistanceUnits = Theme::label(f, F::Regular18, Strings::kKm, 178, 48, 45, LV_TEXT_ALIGN_LEFT);
    Theme::hline(f, 35, 79, 170);
    // Middle: the lap face's two-column row.
    Theme::label(f, F::Italic18, "Avg Lap", 20, 88, 88);
    mPfAvgLapValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 9, 111, 118);
    Theme::vline(f, 119, 79, 80);
    Theme::label(f, F::Italic18, "Avg Pace", 132, 88, 88);
    mPfAvgPaceValue = Theme::label(f, F::SemiBold35, Strings::kNoValue, 114, 111, 118);
    Theme::hline(f, 35, 159, 170);
    // Bottom: the totals face's timer.
    mPfTimerValue = Theme::label(f, F::SemiBold35, "0:00:00", 35, 162, 170);
    Theme::label(f, F::Italic18, "Timer", 0, 204, 240);
}

void TrackScreen::buildFaceStatus()
{
    lv_obj_t* f = mFaceStatus = Theme::container(mRoot, 0, 0, 240, 240);
    mSensorRow = std::make_unique<Widgets::SensorStatusRow>(f, 0, 20, 240, 24);
    Theme::hline(f, 35, 62, 170);
    // Digits and AM/PM are sized to their text and centred as a group in setTime().
    mDayTime = Theme::label(f, F::SemiBold60, "--:--", 0, kTimeY, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    mMeridiem = Theme::label(f, F::Medium18, "AM", 0, kMeridiemY, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT);
    lv_obj_add_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
    Theme::hline(f, 35, 136, 170);
    mBattery = std::make_unique<Widgets::Battery>(f, 76, 153);
    mPercent = Theme::label(f, F::Medium25, "0%", 42, 187, 157);
}

const uint16_t* TrackScreen::faces(uint16_t& count) const
{
    // The lap clock leads a free run, and follows a workout's own faces.
    static const uint16_t kWorkout[] = {
        FaceId::ID_WORKOUT_GAUGE, FaceId::ID_WORKOUT_STEP, FaceId::ID_WORKOUT_NEXT,
        FaceId::ID_TRACK_CLOCK, FaceId::ID_TRACK1, FaceId::ID_TRACK2, FaceId::ID_TRACK_PACE, FaceId::ID_TRACK3,
    };
    static const uint16_t kIntervals[] = {
        FaceId::ID_INTERVALS, FaceId::ID_TRACK_CLOCK, FaceId::ID_TRACK1, FaceId::ID_TRACK2,
        FaceId::ID_TRACK_PACE, FaceId::ID_TRACK3,
    };
    static const uint16_t kFree[] = {
        FaceId::ID_TRACK_CLOCK, FaceId::ID_TRACK1, FaceId::ID_TRACK2, FaceId::ID_TRACK_PACE, FaceId::ID_TRACK3,
    };

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

uint16_t TrackScreen::faceIndex(uint16_t id) const
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

void TrackScreen::updateMode(const Track::Data& data)
{
    const Mode mode = data.workoutMode   ? Mode::Workout
                    : data.intervalsMode ? Mode::Intervals
                                         : Mode::Free;
    if (mModeSet && mode == mMode) {
        return;
    }
    mModeSet = true;
    mMode    = mode;
    uint16_t count = 0;
    faces(count);
    mIndicator->setCount(count);
    // A face the run no longer has (a workout completed or ended) falls back
    // to the first one it has.
    showFace(mFaceId);
}

void TrackScreen::onShow()
{
    mLeaving = false;
    mModel.menu().track.action.reset();

    // Set the mode before the face so that showFace() can fall back to the
    // first face the run has, and the scroll indicator count fits.
    const Track::Data& data = mModel.getTrackData();
    mFaceId = mModel.menu().track.get();
    updateMode(data);

    // Coming back from a cool-down alert, show the intervals face regardless of
    // where the user had scrolled, so the cool-down phase is visible.
    const bool forceIntervals = mMode == Mode::Intervals &&
        mModel.getPendingAlertIntervals().phase == Track::IntervalsPhase::COOL_DOWN;
    showFace(forceIntervals ? static_cast<uint16_t>(FaceId::ID_INTERVALS) : mModel.menu().track.get());

    mIsImperial       = mModel.isUnitsImperial();
    mIs12Hour         = mModel.is12HourFormat();
    mHrThresholdCount = mModel.getHrThresholdsCount() < App::Config::kHrThresholdsCount
                            ? mModel.getHrThresholdsCount()
                            : static_cast<uint8_t>(App::Config::kHrThresholdsCount);
    memcpy(mHrThresholds, mModel.getHrThresholds(), mHrThresholdCount);

    onTrackData(data);
    setWorkoutStep(mModel.getWorkoutStep(false));
    mWorkoutNext->showNext(mModel.getWorkoutStep(true));
    setWorkoutFace(mModel.getWorkoutFace());
    uint8_t h = 0, m = 0, s = 0;
    mModel.getTime(h, m, s);
    setTime(h, m);
    onBatteryLevel(mModel.getBatteryLevel());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");

    // A step card or pop-up that came while another screen showed opens
    // before this screen (ScreenManager::goTo()). This catches one that came
    // after that choice was made.
    if (mModel.hasStepCard() || mModel.waitingPopup() != Model::Popup::None) {
        showPopup();
    }
}

void TrackScreen::onHide()
{
    mModel.menu().track.set(mFaceId);
}

void TrackScreen::showFace(uint16_t id)
{
    uint16_t count = 0;
    const uint16_t* list = faces(count);
    uint16_t index = faceIndex(id);
    if (index >= count) {
        index = 0;
    }
    id      = list[index];
    mFaceId = id;
    setHidden(mFaceIntervals,         id != FaceId::ID_INTERVALS);
    setHidden(mFaceTotal,             id != FaceId::ID_TRACK1);
    setHidden(mFaceLap,               id != FaceId::ID_TRACK2);
    setHidden(mFacePace,              id != FaceId::ID_TRACK_PACE);
    setHidden(mFaceStatus,            id != FaceId::ID_TRACK3);
    setHidden(mFaceClock,             id != FaceId::ID_TRACK_CLOCK);
    setHidden(mWorkoutGauge->root(),  id != FaceId::ID_WORKOUT_GAUGE);
    setHidden(mWorkoutStep->root(),   id != FaceId::ID_WORKOUT_STEP);
    setHidden(mWorkoutNext->root(),   id != FaceId::ID_WORKOUT_NEXT);
    mIndicator->setActive(index);
}

void TrackScreen::showPopup()
{
    // The step card or pop-up replaces this screen on the next frame. Until
    // then the faces are left as they are, so a new step does not show on
    // them before its card. If the switch could not be scheduled they carry
    // on, and the pop-up still waits for the next try.
    mLeaving = ScreenManager::instance().goToWaitingPopup();
}

void TrackScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    mLaneHold.onKey(code);   // a hold of Lap: the lane picker
    uint16_t count = 0;
    const uint16_t* list = faces(count);
    const uint16_t index = faceIndex(mFaceId);
    switch (code) {
        case Btn::L1:
            showFace(list[index == 0 || index >= count ? count - 1 : index - 1]);
            break;
        case Btn::L2:
            showFace(list[index + 1 >= count ? 0 : index + 1]);
            break;
        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::TrackAction);
            break;
        case Btn::R2:
            // In Intervals or a structured workout the lap button moves to the
            // next phase or step, on any face; laps are step-driven. When the
            // workout completes the service drops the mode, so R2 then records
            // a manual lap. A free run always records a manual lap.
            if (mMode != Mode::Free) {
                mModel.intervalsNextPhase();
            } else {
                mModel.saveLap();
                ScreenManager::instance().goTo(ScreenId::TrackLap);
            }
            break;
        default:
            break;
    }
}

void TrackScreen::onTrackData(const Track::Data& data)
{
    if (mLeaving) {
        return;
    }
    // A step card or pop-up whose switch could not be scheduled is tried
    // again with each update, not left until the next event.
    if (mModel.hasStepCard() || mModel.waitingPopup() != Model::Popup::None) {
        showPopup();
        if (mLeaving) {
            return;
        }
    }
    // A workout that completes or is ended drops to a free run.
    updateMode(data);

    char buf[16];

    // Totals face
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
    lv_label_set_text(mPaceValue, buf);
    Fmt::distanceTotal(buf, sizeof(buf), Fmt::distUnits(data.distance, mIsImperial));
    lv_label_set_text(mDistanceValue, buf);
    lv_label_set_text(mDistanceUnits, Fmt::units(mIsImperial));
    Fmt::hms(buf, sizeof(buf), data.totalTime);
    lv_label_set_text(mTimerValue, buf);

    // Lap face
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.lapPace, mIsImperial));
    lv_label_set_text(mLapPaceValue, buf);
    Fmt::distanceLap(buf, sizeof(buf), Fmt::distUnits(data.lapDistance, mIsImperial));
    lv_label_set_text(mLapDistValue, buf);
    Fmt::hms(buf, sizeof(buf), data.lapTime);
    lv_label_set_text(mLapTimerValue, buf);
    Fmt::heartRate(buf, sizeof(buf), data.hr);
    lv_label_set_text(mHrValue, buf);
    mHrZone->setHR(data.hr < App::Display::kMinHR ? 0.0f : data.hr, mHrThresholds, mHrThresholdCount);

    // Pace face: this lap's average pace beside the whole run's
    Fmt::distanceTotal(buf, sizeof(buf), Fmt::distUnits(data.distance, mIsImperial));
    lv_label_set_text(mPfDistanceValue, buf);
    lv_label_set_text(mPfDistanceUnits, Fmt::units(mIsImperial));
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.lapPace, mIsImperial));
    lv_label_set_text(mPfAvgLapValue, buf);
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.avgPace, mIsImperial));
    lv_label_set_text(mPfAvgPaceValue, buf);
    Fmt::hms(buf, sizeof(buf), data.totalTime);
    lv_label_set_text(mPfTimerValue, buf);

    // Workout faces: the live pace under a step with no target, the lap
    // averages on the step face.
    mLivePace = Fmt::paceUnits(data.pace, mIsImperial);
    mLapPace  = Fmt::paceUnits(data.lapPace, mIsImperial);
    mLapHr    = data.avgLapHR;
    mWorkoutStep->setAverages(mLapPace, mLapHr, mHeartRateStep);

    // Intervals face
    if (mMode == Mode::Intervals) {
        const Track::IntervalsData& iv = data.intervals;
        setIntervalsPhase(iv);
        if (iv.metric == Track::IntervalsMetric::DISTANCE) {
            mIntervalsTimer->setPhaseDistance(Fmt::distUnits(iv.distRemaining, mIsImperial), mIsImperial);
        } else {
            mIntervalsTimer->setPhaseTime(iv.phaseTimerSec, iv.metric);
        }
        Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
        lv_label_set_text(mIvPace, buf);
        Fmt::heartRate(buf, sizeof(buf), data.hr);
        lv_label_set_text(mIvHr, buf);
    }

    updateClock(data);

    mHrSource = data.hrSource;
    updateHrIcon();
}

void TrackScreen::buildFaceClock()
{
    using namespace Kit;
    lv_obj_t* f = mFaceClock = Theme::container(mRoot, 0, 0, 240, 240);
    mRing = std::make_unique<TrackRing>(f);

    mClockHeader = Theme::label(f, kHeader, "", 35, kHeaderY, 170, LV_TEXT_ALIGN_CENTER, kText);
    mLapClock    = std::make_unique<BigFigure>(f, kClockBaseY, kHuge, kText);

    // "Last 1:21" and its chip (Run's, LapCompare), centred as one group.
    mLastLabel = rowText(f, kSmall, "Last", kLastLabelY, kText);
    mLastLap   = rowText(f, kAction, Strings::kNoValue, kLastLapY, kText);
    mChip      = Theme::box(f, 0, kChipY, 60, kChipH, SDK::GUI::Color::WHITE);
    mChipText  = Theme::label(mChip, F::Medium18, "", kChipPadX, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT,
                              SDK::GUI::Color::BLACK);
    mLastRow.add(mLastLabel);
    mLastRow.add(mLastLap, 8);
    mLastRow.add(mChip, 8);

    divider(f, kDividerY);

    // This lap's metres and the pace in the watch's unit, one centred group.
    mLapMetres = rowText(f, kChoice, "0", kLapRowY, kText);
    mLapUnit   = rowText(f, kSmall, "m", kUnitY, kText);
    mSplitLine = Theme::box(f, 0, kSplitLineY, 1, 20, kTeal);
    mClockPace = rowText(f, kChoice, Strings::kNoValue, kLapRowY, kText);
    mPaceUnit  = rowText(f, kSmall, "/km", kUnitY, kText);
    mLapRow.add(mLapMetres);
    mLapRow.add(mLapUnit, 5);
    mLapRow.add(mSplitLine, 5);
    mLapRow.add(mClockPace, 5);
    mLapRow.add(mPaceUnit, 5);

    // The lane badge and the track; weak GPS in place of the badge.
    mBadge   = std::make_unique<LaneBadge>(f, 0, kBadgeY);
    mWeakGps = Theme::imageTinted(f, &img_sensorgpslight, 0, kBadgeY, kAccent);
    mFooter  = rowText(f, kSmall, "", kFooterY, kText);
    mFooterRow.add(mBadge->obj());
    mFooterRow.add(mWeakGps);
    mFooterRow.add(mFooter, 5);
}

void TrackScreen::updateClock(const Track::Data& data)
{
    const Track::OnTrack& o = data.onTrack;
    char buf[40];

    // Header: the lap in progress, or a new track's calibration while it runs.
    const unsigned lap = static_cast<unsigned>(data.lapNum + 1u);
    if (o.segment == Track::OnTrack::TRACK
        && (o.cal == Track::OnTrack::CALIBRATING || o.cal == Track::OnTrack::PROVISIONAL)) {
        std::snprintf(buf, sizeof(buf), "Calibrating %u/%u",
                      static_cast<unsigned>(o.calLaps < kCalLockLaps ? o.calLaps : kCalLockLaps),
                      static_cast<unsigned>(kCalLockLaps));
    } else if (o.segment == Track::OnTrack::ROAD) {
        std::snprintf(buf, sizeof(buf), "Road  Lap %u", lap);
    } else {
        std::snprintf(buf, sizeof(buf), "Lap %u", lap);
    }
    setText(mClockHeader, buf);

    // The lap's clock; from an hour the 40 px face, to fit.
    const uint32_t lapS = static_cast<uint32_t>(data.lapTime);
    clockText(buf, sizeof(buf), lapS);
    mLapClock->set(buf);
    const bool hour = lapS >= 3600u;
    if (hour != mClockSmall) {
        mClockSmall = hour;
        mLapClock->setFont(hour ? Kit::kBig : Kit::kHuge);
    }

    // The last lap and its chip, as the lap pop-up has them.
    const Model::LapInfo& last = mModel.getLap();
    const bool haveLast = data.lapNum > 0 && last.time > 0;
    if (haveLast) {
        clockText(buf, sizeof(buf), static_cast<uint32_t>(last.time));
        setText(mLastLap, buf);
    } else {
        setText(mLastLap, Strings::kNoValue);
    }
    const LapCompare::Delta& d = last.delta;
    const bool chip = haveLast && d.verdict != LapCompare::Verdict::None;
    setHidden(mChip, !chip);
    if (chip) {
        char line[24];
        LapCompare::line(line, sizeof(line), d);
        setText(mChipText, line);
        lv_obj_update_layout(mChipText);
        lv_obj_set_width(mChip, lv_obj_get_width(mChipText) + 2 * kChipPadX);
        const uint32_t fill = d.verdict == LapCompare::Verdict::Faster ? SDK::GUI::Color::LIME_SOFT
                            : d.verdict == LapCompare::Verdict::Slower ? SDK::GUI::Color::SALMON
                                                                        : SDK::GUI::Color::WHITE;
        lv_obj_set_style_bg_color(mChip, Theme::rgb(fill), LV_PART_MAIN);
    }
    mLastRow.layout();

    // This lap's metres, and the live pace.
    std::snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(data.lapDistance > 0.0f ? data.lapDistance : 0.0f));
    setText(mLapMetres, buf);
    Fmt::pace(buf, sizeof(buf), Fmt::paceUnits(data.pace, mIsImperial));
    setText(mClockPace, buf);
    setText(mPaceUnit, mIsImperial ? "/mi" : "/km");
    mLapRow.layout();

    // The lane and the track; weak GPS in place of the badge, and on the
    // road no badge.
    TrackText::where(buf, sizeof(buf), o);
    // "Not a standard track" and the badge are wider than the bezel allows
    // at the bottom: the run screen says it short.
    setText(mFooter, o.cal == Track::OnTrack::NOT_STANDARD && o.segment == Track::OnTrack::TRACK
                         ? "Not standard" : buf);
    mBadge->set(o.lane);
    setHidden(mWeakGps, !o.weakGps);
    setHidden(mBadge->obj(), o.weakGps || o.lane == 0 || o.segment != Track::OnTrack::TRACK);
    mFooterRow.layout();

    // The ring: the share of this lap run, towards the distance auto-lap
    // (400 m without one).
    using Mode = Kit::TrackRing::Mode;
    const Settings& s = mModel.getSettings();
    const float lapM  = s.alertDistanceId != Settings::Alerts::Distance::ID_OFF
                            ? Settings::Alerts::Distance::toMeters(s.alertDistanceId, false) : kRingLapM;
    const float share = data.lapDistance > 0.0f ? data.lapDistance / lapM : 0.0f;
    const uint16_t permille = share >= 1.0f ? 1000u : static_cast<uint16_t>(share * 1000.0f);
    Mode mode = Mode::Lap;
    if (o.segment == Track::OnTrack::ROAD) {
        mode = Mode::Road;
    } else if (o.cal == Track::OnTrack::CALIBRATING || o.cal == Track::OnTrack::PROVISIONAL) {
        mode = Mode::Calibrating;
    } else if (o.segment == Track::OnTrack::NONE) {
        mode = Mode::Idle;
    }
    mRing->set(mode, permille, o.calLaps < kCalLockLaps ? o.calLaps : kCalLockLaps);
}

void TrackScreen::setIntervalsPhase(const Track::IntervalsData& iv)
{
    const bool run  = iv.phase == Track::IntervalsPhase::RUN;
    const bool rest = iv.phase == Track::IntervalsPhase::REST;

    mIntervalsTitle->setText(phaseTitle(iv.phase, iv.runWalk));
    mIntervalsTimer->setColor(run ? kIvRun : rest ? kIvRest : kIvNeutral);
    mIntervalsTimer->setLineVisible(run || rest);

    // Bottom row: pace while running, heart rate while resting, the runner otherwise.
    setHidden(mIvRunIcon,   run || rest);
    setHidden(mIvPaceIcon,  !run);
    setHidden(mIvPace,      !run);
    setHidden(mIvHeartIcon, !rest);
    setHidden(mIvHr,        !rest);

    // Repeat counter only during RUN / REST; "n" alone for open-ended repeats.
    setHidden(mIvRepeats, !(run || rest));
    if (iv.totalRepeats == 0) {
        lv_label_set_text_fmt(mIvRepeats, "%u", static_cast<unsigned>(iv.repeat));
    } else {
        lv_label_set_text_fmt(mIvRepeats, "%u/%u", static_cast<unsigned>(iv.repeat),
                              static_cast<unsigned>(iv.totalRepeats));
    }
}

void TrackScreen::onWorkoutData(const Model::WorkoutFace& face)
{
    if (mLeaving) {
        return;
    }
    setWorkoutFace(face);
}

void TrackScreen::onWorkoutStep(bool next)
{
    if (mLeaving) {
        return;
    }
    if (next) {
        mWorkoutNext->showNext(mModel.getWorkoutStep(true));
        return;
    }
    // onShow() shows the new step when the card hands back.
    if (mModel.hasStepCard()) {
        showPopup();
        return;
    }
    setWorkoutStep(mModel.getWorkoutStep(false));
}

void TrackScreen::setWorkoutFace(const Model::WorkoutFace& face)
{
    mHeartRateStep = static_cast<SDK::Workout::Target>(face.target) == SDK::Workout::Target::HeartRate;
    mWorkoutGauge->set(face, mIsImperial, mLivePace);
    mWorkoutStep->setRepeat(face.rep, face.reps);
    mWorkoutStep->setAverages(mLapPace, mLapHr, mHeartRateStep);
}

void TrackScreen::setWorkoutStep(const Model::WorkoutStepInfo& step)
{
    mWorkoutStep->setStep(step);
    mWorkoutStep->setRepeat(step.rep, step.reps);
}

void TrackScreen::onBatteryLevel(uint8_t level)
{
    mBattery->setLevel(level);
    lv_label_set_text_fmt(mPercent, "%u%%", level);
}

void TrackScreen::onTime(uint8_t hour, uint8_t minute, uint8_t /*sec*/)
{
    setTime(hour, minute);
}

void TrackScreen::setTime(uint8_t h, uint8_t m)
{
    const SDK::Clock::Hour12 civil = SDK::Clock::to12Hour(h);
    lv_label_set_text_fmt(mDayTime, "%u:%02u", mIs12Hour ? civil.hour : h, m);

    // Centre digits (+ suffix) as one group on the 240 px face, as the Run app does.
    lv_obj_update_layout(mDayTime);
    const int32_t timeW = lv_obj_get_width(mDayTime);
    int32_t groupW = timeW;
    int32_t merW   = 0;
    if (mIs12Hour) {
        lv_label_set_text(mMeridiem, civil.pm ? "PM" : "AM");
        lv_obj_remove_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
        lv_obj_update_layout(mMeridiem);
        merW   = lv_obj_get_width(mMeridiem);
        groupW = timeW + kMeridiemGap + merW;
    } else {
        lv_obj_add_flag(mMeridiem, LV_OBJ_FLAG_HIDDEN);
    }
    const int32_t left = (240 - groupW) / 2;
    lv_obj_set_pos(mDayTime, left, kTimeY);
    if (mIs12Hour) {
        lv_obj_set_pos(mMeridiem, left + timeW + kMeridiemGap, kMeridiemY);
    }
}

// The model has put each pop-up on its waiting list (Model::waitingPopup());
// one that comes while another pop-up shows opens when that one is done.

void TrackScreen::onLapChanged(uint8_t /*lapEnd*/)
{
    showPopup();
}

void TrackScreen::onIntervalsPhaseAlert()
{
    showPopup();
}

void TrackScreen::onRunAlert()
{
    showPopup();
}

void TrackScreen::onIntervalsWorkoutCompleted()
{
    showPopup();
}

void TrackScreen::onGpsFix(bool acquired)
{
    mSensorRow->setGps(Widgets::SensorStatusRow::gpsState(acquired));
}

void TrackScreen::onAccessoryStatus(uint8_t state, const char* /*name*/)
{
    mAccessoryState = state;
    updateHrIcon();
}

void TrackScreen::updateHrIcon()
{
    mSensorRow->setHr(Widgets::SensorStatusRow::hrStateFromSource(mAccessoryState, mHrSource));
}
