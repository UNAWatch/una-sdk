#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <ctime>   // std::time_t (mLastCalibUtc, startTrack, ...)

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/TrackMap/TrackMapBuilder.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/Metrics/GpsSpeedFilter.hpp"
#include "SDK/Metrics/DeltaCounter.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Filters/SimpleLPF.hpp"

#include "SDK/Calibration/OutdoorStrideCalibrator.hpp"

#include "SDK/Fit/FitWorkoutReader.hpp"
#include "SDK/Workout/Intervals.hpp"
#include "SDK/Workout/ScheduleState.hpp"
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutAlerts.hpp"
#include "SDK/Workout/WorkoutEngine.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include "SettingsSerializer.hpp"
#include "ActivitySummarySerializer.hpp"
#include "ActivityWriter.hpp"
#include "Commands.hpp"
#include "WristTiltDetector.hpp"

class Service : public WristTiltDetector::IListener
{
public:
    Service(SDK::Kernel &kernel);

    virtual ~Service();

    void run();

private:
    // -- Constants ------------------------------------------------------------

    static constexpr uint32_t skBacklightTimeout     = 5000;
    static constexpr uint32_t skSamplePeriod         = 1000;
    static constexpr uint32_t skSampleLatency        = 1000;

    static constexpr float    skMapDistanceThreshold = 10.0f; // meters
    static constexpr uint32_t skMapMaxPoints         = 70;

    static constexpr uint32_t skBatteryLogPeriodMs   = 5 * 60 * 1000;
    static constexpr float    skFusionSampleRateHz   = 100.0f;

    /// Geometry of the live-speed filter, in 1 Hz track ticks. The receiver
    /// already averages its speed over 10 s, so the window only bridges a tick
    /// that brings no sample; a longer one would average twice and add lag.
    /// The position chord is 2 s, 5-8 m at running pace: a longer one cuts the
    /// corners of the path and reads the scale low.
    static constexpr std::size_t skSpeedSmoothTicks = 2;
    static constexpr std::size_t skSpeedScaleTau    = 180;
    static constexpr std::size_t skSpeedChordTicks  = 2;

    /// Laps reserved in the summary for a run, plus one per workout step up
    /// to skMaxWorkoutLapReserve, so that a workout's laps do not reallocate
    /// the list mid-run.
    static constexpr std::size_t skLapReserve           = 10;
    static constexpr std::size_t skMaxWorkoutLapReserve = 128;

    /// The lead-in's beep each second near the end of a step: shorter than
    /// the step-change alert's, so the countdown and the change sound apart.
    static constexpr uint16_t skLeadInBeepMs = 80;

    /// The gauge arc's outer thirds: 30 s/km beyond a pace band, 15 bpm
    /// beyond a heart-rate band.
    static constexpr float skArcMarginSecPerKm = 30.0f;
    static constexpr float skArcMarginBpm      = 15.0f;

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;

    // -- Settings & persistence -----------------------------------------------

    Settings                  mSettings;
    bool                      mIsImperial = false;
    bool                      mTimeFormat12h = false;
    SettingsSerializer        mSettingsSerializer;
    ActivitySummary           mSummary;
    ActivitySummarySerializer mActivitySummarySerializer;
    ActivityWriter            mActivityWriter;
    SDK::TrackMapBuilder      mTrackMapBuilder;

    // -- Sensors --------------------------------------------------------------

    SDK::Sensor::Connection mSensorGpsLocation;
    SDK::Sensor::Connection mSensorGpsSpeed;
    SDK::Sensor::Connection mSensorGpsDistance;
    SDK::Sensor::Connection mSensorPressure;
    SDK::Sensor::Connection mSensorHr;
    SDK::Sensor::Connection mSensorBatteryLevel;
    SDK::Sensor::Connection mSensorBatteryMetrics;
    SDK::Sensor::Connection mSensorWristMotion;
    SDK::Sensor::Connection mSensorFusion;
    SDK::Sensor::Connection mSensorRunningCadence;
    SDK::Sensor::Connection mSensorGrade;
    bool                    mIsSensorsConnected = false;

    struct {
        float cadenceSpm      = 0.0f;
        bool  cadenceValid    = false;
    } mRunningCadence{};

    // -- Outdoor stride calibration inputs (latched per stream) ---------
    struct {
        float gradePct        = 0.0f;
        bool  gradeValid      = false;
    } mGradeData{};
    float       mGpsSpeedMs       = 0.0f; ///< Latest raw GPS speed (instantaneous source).
    bool        mGpsSpeedValid    = false;
    bool        mGpsSpeedFresh    = false; ///< A speed sample arrived since the last track tick.
    float       mGpsSpeedPeakMs   = 0.0f;  ///< Highest valid speed sample since the last track tick.
    bool        mGpsPosFresh      = false; ///< A position sample arrived since the last track tick.
    bool        mGpsDeadReckoning = false;
    std::time_t mLastCalibUtc     = 0;   ///< For per-tick delta_t.

    // -- Metrics --------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::MonotonicCounter<std::time_t>          mTimeCounter;
    SDK::Metric::MonotonicCounter<float>                mDistanceCounter;
    SDK::Metric::VariableCounter                        mSpeedCounter;
    /// Corrects the receiver's Doppler speed against the distance its own
    /// position track covers, and smooths it. This is the single source for
    /// everything derived from GPS speed -- the readout, the FIT record series,
    /// the maxima, the implied step length and the stride calibrator -- so that
    /// none of them inherit the receiver's environment-dependent under-read.
    /// The averages are not involved either way: they come from the distance and
    /// time totals, not from a mean of these samples.
    SDK::Metric::GpsSpeedFilter<skSpeedSmoothTicks,
                                skSpeedScaleTau,
                                skSpeedChordTicks>      mSpeedFilter;
    SDK::Metric::VariableCounter                        mHrCounter;
    uint8_t                                             mHrSource = 0;      ///< Latest HR source (HeartRateEx::Source) for the icon + FIT hr_source.
    uint8_t                                             mHrOpticalBpm = 0;  ///< Latest raw optical (PPG) bpm, for the FIT hr_optical series.
    uint8_t                                             mHrExternalBpm = 0; ///< Latest raw external (strap) bpm, for the FIT hr_external series.
    SDK::Filter::SimpleLPF                              mAltitudeFilter;
    SDK::Metric::DeltaCounter                           mAltitudeCounter;

    // Battery SoC and voltage are sampled independently;
    // a FIT record is written only when both are due.
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatterySoc;     ///< State of charge, percent
    SDK::Metric::ThrottledSample<float, SDK::Interface::ISystem> mBatteryVoltage; ///< Voltage, volts

    // -- GPS state ------------------------------------------------------------

    struct {
        bool     fix;       // Actual GPS fix
        float    latitude;  // degrees
        float    longitude; // degrees
        float    altitude;  // meters
        uint32_t timestamp; // ms

        void reset()
        {
            fix       = false;
            latitude  = 0.0f;
            longitude = 0.0f;
            altitude  = 0.0f;
            timestamp = 0;
        }
    } mGps{};

    float mSeaLevelPressure = 0.0f; // Pa

    // -- Track state ----------------------------------------------------------

    enum class LapDivSource {
        OFF = 0,
        DISTANCE,
        TIME,
    };

    LapDivSource mLapDivSource        = LapDivSource::OFF;
    Track::State mTrackState          = Track::State::INACTIVE;
    bool         mPreviousGpsFixState = false;
    bool         mGpsInitialConnectFailed = false;  ///< GPS_LOCATION subscribe lost the startup ack race; the retry logs the recovery.
    bool         mGpsWanted = false;                 ///< GPS_LOCATION should stay connected (pre-activity + active track); cleared in disconnect() so the retry never re-wakes the GNSS post-activity.
    bool         mSessionNotEmpty     = false;
    bool         mLapNotEmpty         = false;
    uint32_t     mLapCarryMs          = 0;      ///< Time the last lap handed to this one.
    Track::Data  mTrackData{};

    // -- Interval training state ----------------------------------------------

    bool mIntervalsMode      = false;
    bool mIntervalsCompleted = false; ///< Set after workout completed; blocks further phase processing

    /// The intervals settings as a workout program, which mEngine runs step
    /// by step. The program is about 15 KB: the Service is statically
    /// allocated, never on a stack.
    SDK::Workout::IntervalsSpec   mIntervalsSpec;
    SDK::Workout::IntervalsLayout mIntervalsLayout;
    SDK::Workout::Program         mProgram;
    SDK::Workout::Engine          mEngine;
    SDK::Workout::LeadInAlert     mLeadIn;

    // -- Wrist tilt -----------------------------------------------------------

    WristTiltDetector mWristTiltDetector;

    // -- Outdoor stride calibrator ---------------------------------------

    SDK::Calibration::OutdoorStrideCalibrator mCalibrator;

    // -- Structured workouts --------------------------------------------------

    /// The workout files, listed when the GUI starts and no run is going
    /// (about 10 KB). A run reads its workout into mProgram, and so do the
    /// list and the details screen, which only happen before a run.
    SDK::Workout::Library        mLibrary;
    SDK::Fit::FitWorkoutReader   mReader;
    SDK::Workout::ScheduleState  mSchedule;

    bool              mWorkoutMode       = false; ///< A structured workout is running.
    int16_t           mWorkoutRequested  = CustomMessage::TrackStart::kNoWorkout;
    bool              mWorkoutReachedEnd = false; ///< Completion reached; ending early after it still counts.
    SDK::Workout::Ymd mWorkoutDate       = 0;     ///< The running workout's planned day, 0 if not scheduled.
    char              mWorkoutFile[SDK::Workout::Library::kFileBytes] = {};

    SDK::Workout::SpeedGauge     mSpeedGauge;
    SDK::Workout::GpsCatchUp     mGpsCatchUp;  ///< Holds the speed gauge off until the distance catches up.
    SDK::Workout::HeartRateGauge mHrGauge;
    SDK::Workout::ZoneAlerts     mZoneAlerts;
    float                        mBandLow  = 0.0f; ///< The step's target band, m/s or bpm.
    float                        mBandHigh = 0.0f;

    /// HR zone thresholds from the system settings; the last is maximum HR.
    uint8_t mHrThresholds[CustomMessage::kHrThresholdsCount] = {};

    // -- Lifecycle ------------------------------------------------------------

    void connectGps();
    void connectSensors(); // All except GPS
    void disconnect();
    void onStartGUI();
    void onStopGUI();

    // -- Sensor data dispatch -------------------------------------------------

    void handleSensorsData(uint16_t handle, SDK::Sensor::DataBatch& data);

    // -- Event handlers -------------------------------------------------------

    void handleEvent(const CustomMessage::TrackStart& event);
    void handleEvent(const CustomMessage::TrackStop& event);
    void handleEvent(const CustomMessage::SettingsSave& event);
    void handleEvent(const CustomMessage::TrackPause& event);
    void handleEvent(const CustomMessage::TrackResume& event);
    void handleEvent(const CustomMessage::ManualLap& event);
    void handleEvent(const CustomMessage::IntervalsNextPhase& event);
    void handleEvent(const CustomMessage::WorkoutDetailsRequest& event);
    void handleEvent(const CustomMessage::WorkoutEnd& event);

    // -- Track control --------------------------------------------------------

    void sendInitialInfoToGui();
    void startTrack(std::time_t utc);
    void processTrack();

    /// What ended a lap, and the workout step it was, for the FIT lap.
    struct LapEnd {
        SDK::Fit::LapTrigger trigger      = SDK::Fit::LapTrigger::Manual;
        uint16_t             wktStepIndex = SDK::Fit::kMessageIndexInvalid;
        SDK::Fit::Intensity  intensity    = SDK::Fit::Intensity::Invalid;
        /// A workout step that ran out (ranOut) ended between two ticks.
        /// carryMs is the time counted beyond its end, for the next lap; stepM
        /// is the distance it covered, the rest of the lap's counted distance
        /// going to the next lap.
        bool                 ranOut       = false;
        uint32_t             carryMs      = 0;
        float                stepM        = 0.0f;
    };
    void saveLap(const LapEnd& end, float autoLapDistanceM = 0.0f);
    void stopTrack(bool discard);
    void pauseTrack(bool pause);
    void buildPartialSummary();
    ActivityWriter::RecordData prepareRecordData();
    LapDivSource getLapDivSource();

    // -- Interval training ----------------------------------------------------

    /// Build the program from the intervals settings and start it.
    void startIntervals();
    /// R2: end the current interval phase or workout step and start the next.
    void nextStep();
    void processIntervals();
    /// A step ended (@p how says why): close its lap, and alert.
    void onIntervalsStepEnded(SDK::Workout::Engine::Change how);
    /// Refresh mTrackData.intervals from the engine's current step.
    void updateIntervalsData();
    void notifyStepChange();
    Track::IntervalsPhase intervalsPhase(uint16_t step) const;

    /// The lap for workout step @p step, ended by @p trigger.
    LapEnd workoutLap(uint16_t step, SDK::Fit::LapTrigger trigger) const;
    /// The lap for the step the engine has just ended, from where it ended.
    LapEnd endedStepLap(SDK::Workout::Engine::Change how) const;
    /// What ended step @p step, as the engine reported it.
    SDK::Fit::LapTrigger lapTrigger(SDK::Workout::Engine::Change how, uint16_t step) const;

    // -- Structured workouts --------------------------------------------------

    /// List the workouts, unless a run is going.
    void scanWorkouts();
    /// Send the list, with today's workout unless it was completed.
    void sendWorkoutList();
    /// Read entry @p index of the list into mProgram.
    bool readWorkout(uint16_t index);
    bool startWorkout(uint16_t index);
    void processWorkout();
    void onWorkoutStepEnded(SDK::Workout::Engine::Change how);
    /// Set the gauges and alerts up for the engine's current step.
    void startWorkoutStep();
    void endWorkout();
    void sendWorkoutData();
    void sendWorkoutSteps();
    void sendWorkoutStep(const SDK::Workout::Step* step, bool next);
    void playZoneAlert(SDK::Workout::ZoneAlerts::Alert alert);
    /// The zone of the current step's target, Unknown with none.
    SDK::Workout::Zone targetZone() const;
    /// A heart-rate target as a bpm band, from the watch's HR thresholds.
    bool hrBand(const SDK::Workout::Step& step, float& low, float& high) const;
    bool hasTrustedHr() const;
    /// The watch's local date, or 0 if its clock is not set.
    SDK::Workout::Ymd localDate();

    /// The run's active time and distance, as the workout engine counts them.
    uint32_t activeTimeMs() const;
    uint32_t activeDistanceCm() const;

    // -- Notifications --------------------------------------------------------

    void setCapabilities();
    void requestAccessoryPrepare();   // opt in to external HR (pre-warm at GUI start)
    void requestAccessoryRelease();
    void notifyFirstFix();
    void notifyLapEnd();
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1, uint16_t silenceMs = 100, uint8_t volume = 100);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 100);

    // -- WristTilt callback ---------------------------------------------------

    virtual void onWristTilt(uint32_t timestampMs) override;
};

#endif // SERVICE_HPP
