/**
 ******************************************************************************
 * @file    TrackScreen.hpp
 * @brief   The live activity screen: the totals, lap, pace and status faces,
 *          after the three workout faces in a structured workout or the
 *          intervals face in Intervals. L1/L2 page faces, R1 opens the action menu, R2
 *          marks a lap (or moves to the next step or interval phase).
 *
 * Port of the Run app's TrackView/TrackPresenter and its TrackFace* containers.
 ******************************************************************************
 */

#ifndef TRACK_SCREEN_HPP
#define TRACK_SCREEN_HPP

#include <memory>

#include "gui/screens/LaneHold.hpp"
#include "gui/screens/Screen.hpp"
#include "gui/widgets/Kit.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WorkoutFaces.hpp"

class TrackScreen : public Screen
{
public:
    explicit TrackScreen(Model& model);

    // Screen
    void onShow() override;
    void onHide() override;
    void onKey(uint8_t code) override;

    // ModelListener
    void onTrackData(const Track::Data& data) override;
    void onBatteryLevel(uint8_t level) override;
    void onTime(uint8_t hour, uint8_t minute, uint8_t sec) override;
    void onLapChanged(uint8_t lapEnd) override;
    void onIntervalsPhaseAlert() override;
    void onRunAlert() override;
    void onIntervalsWorkoutCompleted() override;
    void onGpsFix(bool acquired) override;
    void onAccessoryStatus(uint8_t state, const char* name) override;
    void onWorkoutData(const Model::WorkoutFace& face) override;
    void onWorkoutStep(bool next) override;

protected:
    void build() override;

private:
    using FaceId = App::MenuNav::TrackView::Id;

    /// The kind of run decides the faces: a structured workout's three, the
    /// Intervals one, then those every run has.
    enum class Mode { Free, Intervals, Workout };

    void buildFaceIntervals();
    void buildFaceTotal();
    void buildFaceLap();
    void buildFacePace();
    void buildFaceStatus();
    void buildFaceClock();
    void updateClock(const Track::Data& data);
    void updateMode(const Track::Data& data);
    /// The faces of the current mode, in L2 order.
    const uint16_t* faces(uint16_t& count) const;
    uint16_t faceIndex(uint16_t id) const;  ///< Its place in faces(), or count
    /// Show face @p id, or the mode's first face if the run has no such face.
    void showFace(uint16_t id);
    void showPopup();
    void setWorkoutFace(const Model::WorkoutFace& face);
    void setWorkoutStep(const Model::WorkoutStepInfo& step);
    void setTime(uint8_t h, uint8_t m);
    void updateHrIcon();
    void setIntervalsPhase(const Track::IntervalsData& iv);

    // Face containers (240 x 240, one visible at a time)
    lv_obj_t* mFaceIntervals = nullptr;
    lv_obj_t* mFaceTotal     = nullptr;
    lv_obj_t* mFaceLap       = nullptr;
    lv_obj_t* mFacePace      = nullptr;
    lv_obj_t* mFaceStatus    = nullptr;
    lv_obj_t* mFaceClock     = nullptr;

    // Lap clock face (Track Run): the lap's clock leads, the last lap and
    // its chip, this lap's metres and the pace, the lane and the track, and
    // the ring round the bezel filling towards the auto-lap.
    std::unique_ptr<Kit::TrackRing> mRing;
    lv_obj_t* mClockHeader = nullptr;
    std::unique_ptr<Kit::BigFigure> mLapClock;
    bool      mClockSmall  = false;   ///< The 40 px face, from an hour
    lv_obj_t* mLastLabel   = nullptr;
    lv_obj_t* mLastLap     = nullptr;
    lv_obj_t* mChip        = nullptr;
    lv_obj_t* mChipText    = nullptr;
    Kit::Row  mLastRow;
    lv_obj_t* mLapMetres   = nullptr;
    lv_obj_t* mLapUnit     = nullptr;
    lv_obj_t* mSplitLine   = nullptr;
    lv_obj_t* mClockPace   = nullptr;
    lv_obj_t* mPaceUnit    = nullptr;
    Kit::Row  mLapRow;
    std::unique_ptr<Kit::LaneBadge> mBadge;
    lv_obj_t* mWeakGps     = nullptr;
    lv_obj_t* mFooter      = nullptr;
    Kit::Row  mFooterRow;
    LaneHold  mLaneHold;

    // Intervals face
    std::unique_ptr<Widgets::Title>          mIntervalsTitle;
    std::unique_ptr<Widgets::IntervalsTimer> mIntervalsTimer;
    lv_obj_t* mIvRepeats   = nullptr;
    lv_obj_t* mIvRunIcon   = nullptr;
    lv_obj_t* mIvPaceIcon  = nullptr;
    lv_obj_t* mIvHeartIcon = nullptr;
    lv_obj_t* mIvPace      = nullptr;
    lv_obj_t* mIvHr        = nullptr;

    // Totals face
    lv_obj_t* mPaceValue     = nullptr;
    lv_obj_t* mDistanceValue = nullptr;
    lv_obj_t* mDistanceUnits = nullptr;
    lv_obj_t* mTimerValue    = nullptr;

    // Lap face
    lv_obj_t* mHrValue       = nullptr;
    lv_obj_t* mLapPaceValue  = nullptr;
    lv_obj_t* mLapDistValue  = nullptr;
    lv_obj_t* mLapTimerValue = nullptr;
    std::unique_ptr<Widgets::HeartRateZone> mHrZone;

    // Pace face: total distance; this lap's and the whole run's average pace; timer
    lv_obj_t* mPfDistanceValue = nullptr;
    lv_obj_t* mPfDistanceUnits = nullptr;
    lv_obj_t* mPfAvgLapValue   = nullptr;
    lv_obj_t* mPfAvgPaceValue  = nullptr;
    lv_obj_t* mPfTimerValue    = nullptr;

    // Status face
    lv_obj_t* mDayTime  = nullptr;
    lv_obj_t* mMeridiem = nullptr;
    lv_obj_t* mPercent  = nullptr;
    std::unique_ptr<Widgets::Battery>         mBattery;
    std::unique_ptr<Widgets::SensorStatusRow> mSensorRow;

    // Workout faces
    std::unique_ptr<WorkoutFaceGauge> mWorkoutGauge;
    std::unique_ptr<WorkoutFaceStep>  mWorkoutStep;
    std::unique_ptr<WorkoutStepCard>  mWorkoutNext;

    std::unique_ptr<Widgets::Buttons>         mButtons;
    std::unique_ptr<Widgets::ScrollIndicator> mIndicator;

    Mode     mMode          = Mode::Free;
    bool     mModeSet       = false;  ///< mMode set by the first updateMode()
    bool     mLeaving       = false;  ///< A pop-up or step card is to replace this screen
    bool     mHeartRateStep = false;  ///< The current step's target is a heart rate
    float    mLivePace      = 0.0f;   ///< s/km or s/mi
    float    mLapPace       = 0.0f;
    float    mLapHr         = 0.0f;
    uint16_t mFaceId        = FaceId::ID_TRACK1;
    bool     mIsImperial    = false;
    bool     mIs12Hour      = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
    uint8_t  mAccessoryState   = 0;
    uint8_t  mHrSource         = 0;
};

#endif // TRACK_SCREEN_HPP
