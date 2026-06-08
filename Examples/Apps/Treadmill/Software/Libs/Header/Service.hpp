#ifndef SERVICE_HPP
#define SERVICE_HPP

#include <ctime>   // std::time_t (mLastTickUtc, startTrack, ...)

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/TrackMap/TrackMapBuilder.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/Metrics/DeltaCounter.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Filters/SimpleLPF.hpp"

#include "SDK/Calibration/CadenceStrideModel.hpp"
#include "SDK/Calibration/TreadmillSpeedEstimator.hpp"
#include "SDK/Calibration/CadenceSpikeFilter.hpp"

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

    // -- Infrastructure -------------------------------------------------------

    SDK::Kernel&          mKernel;
    bool                  mGuiStarted;
    CustomMessage::Sender mGuiSender;

    // -- Settings & persistence -----------------------------------------------

    Settings                  mSettings;
    bool                      mIsImperial = false;
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
    } mRunningCadence{};   ///< Raw wrist cadence from the kernel sensor.

    // Spike-filtered cadence (app-side fix for the wrist estimator's brief
    // valid-but-low glitches). The raw mRunningCadence above is de-spiked once
    // per active tick into mCadence, which drives speed/distance AND the
    // recorded FIT cadence so all three stay consistent. Dropouts bypass the
    // filter so the estimator's hold-forward still owns them.
    SDK::Calibration::CadenceSpikeFilter mCadenceFilter;
    struct {
        float cadenceSpm   = 0.0f;
        bool  cadenceValid = false;
    } mCadence{};

    // -- Cadence/stride estimation inputs ---------------------------------
    float       mHeightM      = 1.70f; ///< Profile height (m) for phase-1 demographic SL; from settings.
    std::time_t mLastTickUtc  = 0;     ///< For per-tick delta_t fed to the estimator.

    // -- Metrics --------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::MonotonicCounter<std::time_t>          mTimeCounter;
    SDK::Metric::MonotonicCounter<float>                mDistanceCounter;
    SDK::Metric::VariableCounter                        mSpeedCounter;
    SDK::Metric::VariableCounter                        mHrCounter;
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
    bool         mSessionNotEmpty     = false;
    bool         mLapNotEmpty         = false;
    Track::Data  mTrackData{};

    // -- Interval training state ----------------------------------------------

    bool        mIntervalsMode        = false;
    bool        mIntervalsCompleted   = false; ///< Set after workout completed; blocks further phase processing
    std::time_t mPhaseStartActiveSec  = 0;     ///< mTimeCounter.getValueActive() at phase start
    float       mPhaseStartActiveDist = 0.0f;  ///< mDistanceCounter.getValueActive() at phase start

    // -- Wrist tilt -----------------------------------------------------------

    WristTiltDetector mWristTiltDetector;

    // -- Cadence/stride estimation (read-side) ----------------------------
    // mModel must be declared before mEstimator: the estimator holds a
    // reference to the model and is initialised from it.
    SDK::Calibration::CadenceStrideModel      mModel;
    SDK::Calibration::TreadmillSpeedEstimator mEstimator;

    // -- Post-run Calibrate & Save (§5) -----------------------------------
    /// True between "stop recording" and the GUI's calibrate/skip decision;
    /// the FIT session is finalised only once this resolves. Calibrate & Save is
    /// offered on every run; the delta-LUT learning gate (tier + minimum
    /// distance) lives in CadenceStrideModel::applyPostRunCalibration.
    bool  mAwaitingCalibration = false;
    float mCalibSteps[SDK::Calibration::StrideLut::kBinCount] = {}; ///< S_i snapshot.
    float mCalibDEstimatedM = 0.0f;                                 ///< Pre-correction distance.
    ActivityWriter::TrackData mPendingFitTrack{};                   ///< Session msg, finalised on calibrate.

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
    void handleEvent(const CustomMessage::TrackCalibrate& event);
    void handleEvent(const CustomMessage::CalibDataRequest& event);
    void handleEvent(const CustomMessage::CalibClear& event);

    // -- Track control --------------------------------------------------------

    void sendInitialInfoToGui();
    void startTrack(std::time_t utc);
    void processTrack();
    void saveLap();
    void stopTrack(bool discard);
    void finalizeActivity(float distanceActualM); ///< Apply §5 calibration (<=0 = skip) + write FIT session.
    void pauseTrack(bool pause);
    void buildPartialSummary();
    ActivityWriter::RecordData prepareRecordData();
    LapDivSource getLapDivSource();

    // -- Interval training ----------------------------------------------------

    void startIntervalsPhase(Track::IntervalsPhase phase);
    void advanceIntervalsPhase(bool manual = false);
    void processIntervals();
    void onIntervalsPhaseChange(bool alert, bool manual);

    // -- Notifications --------------------------------------------------------

    void setCapabilities();
    void notifyLapEnd();
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1, uint16_t silenceMs = 100);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 100);

    // -- WristTilt callback ---------------------------------------------------

    virtual void onWristTilt(uint32_t timestampMs) override;
};

#endif // SERVICE_HPP
