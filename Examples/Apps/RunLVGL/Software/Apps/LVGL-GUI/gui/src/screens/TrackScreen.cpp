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

#include <cstring>

#include "SDK/Utils/ClockTime.hpp"

using namespace SDK::GUI;
using F = Theme::Font;

namespace
{
constexpr uint16_t kFaceCount = App::MenuNav::TrackView::ID_COUNT;

// Status face clock geometry (TrackFaceStatus.hpp).
constexpr int32_t kTimeY       = 63;
constexpr int32_t kMeridiemY   = 105;
constexpr int32_t kMeridiemGap = 5;

// Interval phase accents (TrackFaceIntervals).
constexpr uint32_t kIvNeutral = Color::WHITE;
constexpr uint32_t kIvRun     = Color::CYAN;
constexpr uint32_t kIvRest    = Color::YELLOW_DARK;

const char* phaseTitle(Track::IntervalsPhase phase)
{
    switch (phase) {
        case Track::IntervalsPhase::RUN:       return "RUN";
        case Track::IntervalsPhase::REST:      return "REST";
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
} // namespace

TrackScreen::TrackScreen(Model& model)
    : Screen(model)
{
}

void TrackScreen::build()
{
    mIndicator = std::make_unique<Widgets::ScrollIndicator>(mRoot, Widgets::ScrollIndicator::kSmall);
    mButtons = std::make_unique<Widgets::Buttons>(mRoot);
    mButtons->set(Widgets::Buttons::NONE, Widgets::Buttons::NONE,
                  Widgets::Buttons::NONE, Widgets::Buttons::AMBER);

    buildFaceStatus();
    buildFaceLap();
    buildFaceTotal();
    buildFaceIntervals();
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

uint16_t TrackScreen::firstFace() const
{
    return mIntervalsMode ? FaceId::ID_INTERVALS : FaceId::ID_TRACK1;
}

void TrackScreen::onShow()
{
    mModel.menu().track.action.reset();

    // The intervals face exists only in an intervals workout; the stored face
    // position (default ID_INTERVALS) is clamped into the valid range.
    const Track::Data& data = mModel.getTrackData();
    mIntervalsMode = data.intervalsMode;
    mIndicator->setCount(mIntervalsMode ? kFaceCount : kFaceCount - 1);

    // Coming back from a cool-down alert, show the intervals face regardless of
    // where the user had scrolled, so the cool-down phase is visible.
    const bool forceIntervals = mIntervalsMode &&
        mModel.getPendingAlertIntervals().phase == Track::IntervalsPhase::COOL_DOWN;
    uint16_t face = forceIntervals ? static_cast<uint16_t>(FaceId::ID_INTERVALS) : mModel.menu().track.get();
    if (face < firstFace() || face >= kFaceCount) {
        face = firstFace();
    }
    showFace(face);

    mIsImperial       = mModel.isUnitsImperial();
    mIs12Hour         = mModel.is12HourFormat();
    mHrThresholdCount = mModel.getHrThresholdsCount() < App::Config::kHrThresholdsCount
                            ? mModel.getHrThresholdsCount()
                            : static_cast<uint8_t>(App::Config::kHrThresholdsCount);
    memcpy(mHrThresholds, mModel.getHrThresholds(), mHrThresholdCount);

    onTrackData(data);
    uint8_t h = 0, m = 0, s = 0;
    mModel.getTime(h, m, s);
    setTime(h, m);
    onBatteryLevel(mModel.getBatteryLevel());
    onGpsFix(mModel.hasGpsFix());
    onAccessoryStatus(mModel.getAccessoryState(), "");
}

void TrackScreen::onHide()
{
    mModel.menu().track.set(mFaceId);
}

void TrackScreen::showFace(uint16_t id)
{
    mFaceId = id;
    setHidden(mFaceIntervals, id != FaceId::ID_INTERVALS);
    setHidden(mFaceTotal,     id != FaceId::ID_TRACK1);
    setHidden(mFaceLap,       id != FaceId::ID_TRACK2);
    setHidden(mFaceStatus,    id != FaceId::ID_TRACK3);
    // In a free run the indicator has no slot for the hidden intervals face.
    mIndicator->setActive(static_cast<uint16_t>(mIntervalsMode ? id : id - 1));
}

void TrackScreen::onKey(uint8_t code)
{
    namespace Btn = SDK::GUI::Button;
    const uint16_t minId = firstFace();
    switch (code) {
        case Btn::L1:
            showFace(mFaceId <= minId ? static_cast<uint16_t>(kFaceCount - 1) : static_cast<uint16_t>(mFaceId - 1));
            break;
        case Btn::L2:
            showFace(mFaceId + 1 >= kFaceCount ? minId : static_cast<uint16_t>(mFaceId + 1));
            break;
        case Btn::R1:
            ScreenManager::instance().goTo(ScreenId::TrackAction);
            break;
        case Btn::R2:
            // In an intervals workout the lap button advances the phase, on any
            // face; laps are phase-driven. A free run records a manual lap.
            if (mIntervalsMode) {
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

    // Intervals face
    if (mIntervalsMode) {
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

    mHrSource = data.hrSource;
    updateHrIcon();
}

void TrackScreen::setIntervalsPhase(const Track::IntervalsData& iv)
{
    const bool run  = iv.phase == Track::IntervalsPhase::RUN;
    const bool rest = iv.phase == Track::IntervalsPhase::REST;

    mIntervalsTitle->setText(phaseTitle(iv.phase));
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

void TrackScreen::onLapChanged(uint8_t /*lapEnd*/)
{
    ScreenManager::instance().goTo(ScreenId::TrackLap);
}

void TrackScreen::onIntervalsPhaseAlert()
{
    ScreenManager::instance().goTo(ScreenId::TrackIntervalsAlert);
}

void TrackScreen::onIntervalsWorkoutCompleted()
{
    ScreenManager::instance().goTo(ScreenId::TrackIntervalsCompleted);
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
