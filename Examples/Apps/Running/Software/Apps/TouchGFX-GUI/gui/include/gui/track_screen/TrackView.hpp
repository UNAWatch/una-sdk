#ifndef TRACKVIEW_HPP
#define TRACKVIEW_HPP

#include <gui_generated/track_screen/TrackViewBase.hpp>
#include <gui/track_screen/TrackPresenter.hpp>
#include <gui/containers/WorkoutFaceGauge.hpp>
#include <gui/containers/WorkoutFaceStep.hpp>
#include <gui/containers/WorkoutStepCard.hpp>

class TrackView : public TrackViewBase
{
public:
    TrackView();
    virtual ~TrackView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /// The kind of run decides the faces: a structured workout's three, the
    /// Intervals one, then those every run has.
    enum class Mode { Free, Intervals, Workout };
    void setMode(Mode mode);
    void setPositionId(uint16_t id);
    uint16_t getPositionId();

    void setConfig(bool isImperial, const uint8_t* thresholds, uint8_t thresholdCount);
    void setTimeFormat(bool is12Hour);
    void setTrackData(const Track::Data& data);

    void setTime(uint8_t h, uint8_t m);
    void setBatteryLevel(uint8_t level);
    void setGpsFix(bool state);
    void setAccessoryStatus(uint8_t state);

    void setWorkoutFace(const Model::WorkoutFace& face);
    void setWorkoutStep(const Model::WorkoutStepInfo& step);
    void setWorkoutNext(const Model::WorkoutStepInfo& step);

protected:

    virtual void handleKeyEvent(uint8_t key) override;

    // In-activity HR icon reflects the live source, recomputed from both the
    // accessory link state (engaged?) and the latest HR-sample source.
    void updateHrIcon();

    // The faces of the current mode, in L2 order.
    const uint16_t* faces(uint16_t& count) const;
    uint16_t        faceIndex(uint16_t id) const;  ///< Its place in faces(), or count

    WorkoutFaceGauge mWorkoutGauge;
    WorkoutFaceStep  mWorkoutStep;
    WorkoutStepCard  mWorkoutNext;

    Mode     mMode          = Mode::Free;
    bool     mHeartRateStep = false;  ///< The current step's target is a heart rate
    float    mLivePace      = 0.0f;   ///< s/km or s/mi
    float    mLapPace       = 0.0f;
    float    mLapHr         = 0.0f;
    uint16_t mCurrentFaceId = 0;
    bool     mIsImperial    = false;
    bool     mIs12Hour      = false;
    uint8_t  mHrThresholds[App::Config::kHrThresholdsCount] = {};
    uint8_t  mHrThresholdCount = 0;
    uint8_t  mAccessoryState = 0;  // last SDK::Accessory::State (engaged?)
    uint8_t  mHrSource       = 0;  // last HR-sample source (steady vs flashing)
};

#endif // TRACKVIEW_HPP
