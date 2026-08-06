#ifndef SERVICE_HPP
#define SERVICE_HPP

#include "SDK/Kernel/Kernel.hpp"
#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/SensorLayer/SensorDataBatch.hpp"
#include "SDK/TrackMap/TrackMapBuilder.hpp"
#include "SDK/Metrics/MonotonicTime.hpp"
#include "SDK/Metrics/MonotonicCounter.hpp"
#include "SDK/Metrics/VariableCounter.hpp"
#include "SDK/Metrics/SpeedSmoother.hpp"
#include "SDK/Metrics/DeltaCounter.hpp"
#include "SDK/Metrics/ThrottledSample.hpp"
#include "SDK/Filters/SimpleLPF.hpp"

#include "SettingsSerializer.hpp"
#include "ActivitySummarySerializer.hpp"
#include "ActivityWriter.hpp"
#include "Commands.hpp"

class Service
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

    /// Window, in 1 Hz track ticks, over which the live speed / pace readout is
    /// averaged. Ten seconds cuts the GPS speed noise to about a third -- enough
    /// to hold a target speed by -- while still tracking a real change of effort
    /// fast enough to be useful.
    static constexpr std::size_t skPaceSmoothingTicks = 10;

    // -- Auto-pause (GPS-speed driven) ----------------------------------------
    //
    // The thresholds are hysteretic: the track auto-pauses below
    // skAutoPauseSpeedMps but only auto-resumes above skAutoResumeSpeedMps, so a
    // rider hovering around a single threshold cannot flap between states.
    //
    // skAutoPauseSpeedMps mirrors the "genuinely moving" floor already used by
    // mSpeedCounter.init() and by the kernel's grade estimator
    // (GradeConfig::kMinWindowSpeedMps). Below it, speed is already excluded
    // from the activity average, so pausing there agrees with what we record.
    //
    // skAutoResumeSpeedMps sits deliberately well below the slowest sustained
    // pedalling (~1.4 m/s / 5 km/h): a rider pulling away up a hill from a
    // standing start MUST resume, and silently dropping a climb is far worse
    // than counting a few stopped seconds at a light. It is still ~2.7x above
    // the stationary GPS noise peak measured on a bench with a fix (~0.33 m/s),
    // which the dwell requirement then filters further.
    //
    // GPS speed arrives at 1 Hz (skSamplePeriod) and updateAutoPause() runs on
    // the same 1 Hz track tick, so a dwell in seconds is also a dwell in
    // samples. Three samples tolerates a single glitched or dropped reading.
    //
    // The detector reads the mGpsSpeed* latches rather than keeping its own
    // copy of the sample: those already carry the isSpeedValid() gate it needs.
    // It must not clear mGpsSpeedFresh -- processTrack() does that after
    // feeding the pace smoother, so updateAutoPause() has to run before it.
    //
    // NOTE: the resume dwell is not free. MonotonicCounter rebases on resume, so
    // the distance and time accrued while waiting out skAutoPauseDwellSec are
    // dropped from the active totals rather than deferred -- roughly 3 m per
    // stop at these values. Shorten the resume side first if that ever shows up
    // in ride totals.
    //
    // These values are provisional: they are derived from bench noise plus
    // cycling speed ranges, not yet from a logged ride. Every transition is
    // logged at INFO so a serial capture can confirm or correct them.
    static constexpr float   skAutoPauseSpeedMps  = 0.5f;  // pause below, m/s (1.8 km/h)
    static constexpr float   skAutoResumeSpeedMps = 0.9f;  // resume above, m/s (3.2 km/h)
    static constexpr uint8_t skAutoPauseDwellSec  = 3;     // sustained ticks either way
    static constexpr uint8_t skAutoPauseStaleSec  = 5;     // hold state after this long with no valid speed

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
    bool                    mIsSensorsConnected = false;

    // -- Latched GPS speed (drives the smoothed live readout) ------------------

    float mGpsSpeedMs    = 0.0f;  ///< Latest raw GPS speed sample.
    bool  mGpsSpeedValid = false; ///< Sample came from a current, non-dead-reckoned fix.
    bool  mGpsSpeedFresh = false; ///< A speed sample arrived since the last track tick.

    // -- Metrics --------------------------------------------------------------

    SDK::Metric::MonotonicTime<SDK::Interface::ISystem> mTimeTracker;
    SDK::Metric::MonotonicCounter<std::time_t>          mTimeCounter;
    SDK::Metric::MonotonicCounter<float>                mDistanceCounter;
    SDK::Metric::VariableCounter                        mSpeedCounter;
    /// Smooths the GPS speed for the live speed / pace readout only; the FIT
    /// record series and the maxima stay on the unsmoothed samples in
    /// mSpeedCounter. The averages are not involved either way -- they come
    /// from the distance and time totals, not from a mean of these samples.
    SDK::Metric::SpeedSmoother<skPaceSmoothingTicks>    mSpeedSmoother;
    SDK::Metric::VariableCounter                        mHrCounter;
    uint8_t                                             mHrSource = 0;      ///< Latest HR source (HeartRateEx::Source) for the icon + FIT hr_source.
    uint8_t                                             mHrOpticalBpm = 0;  ///< Latest raw optical (PPG) bpm, for the FIT hr_optical series.
    uint8_t                                             mHrExternalBpm = 0; ///< Latest raw external (strap) bpm, for the FIT hr_external series.
    SDK::Filter::SimpleLPF                              mAltitudeFilter;
    SDK::Metric::DeltaCounter                           mAltitudeCounter;

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

    /**
     * @brief Who owns the current pause.
     *
     * Auto-pause must never resume a pause the rider asked for: while they sit
     * on the action overlay deciding whether to save or discard, movement of
     * the wrist (or the bike being wheeled) must not restart recording.
     */
    enum class PauseSource {
        NONE = 0,   ///< Not paused.
        MANUAL,     ///< Rider opened the pause/action overlay.
        AUTO,       ///< Auto-pause detected a stop.
    };

    /**
     * @brief Auto-pause detector state, evaluated once per second.
     *
     * Kept separate from mSpeedCounter because the counter is deliberately
     * paused along with the rest of the metrics, whereas the detector has to
     * keep watching speed *while* paused in order to notice the rider moving
     * off again.
     */
    struct AutoPauseState {
        /// Ticks since the last valid speed sample. Starts (and resets)
        /// SATURATED, so the detector treats speed as stale until a real sample
        /// proves otherwise -- mGpsSpeedMs is an initialiser, not a measurement,
        /// until then, and acting on it would auto-pause any ride started
        /// before the first GPS fix.
        uint8_t staleSec = skAutoPauseStaleSec;
        uint8_t belowSec = 0;   ///< Consecutive samples under the pause threshold.
        uint8_t aboveSec = 0;   ///< Consecutive samples over the resume threshold.

        /// Clear the dwell counters only; freshness tracking is unaffected.
        void resetDwell()
        {
            belowSec = 0;
            aboveSec = 0;
        }

        void reset()
        {
            staleSec = skAutoPauseStaleSec;
            resetDwell();
        }
    } mAutoPause{};

    LapDivSource mLapDivSource        = LapDivSource::OFF;
    PauseSource  mPauseSource         = PauseSource::NONE;
    Track::State mTrackState          = Track::State::INACTIVE;
    bool         mPreviousGpsFixState = false;
    bool         mGpsInitialConnectFailed = false;  ///< GPS_LOCATION subscribe lost the startup ack race; the retry logs the recovery.
    bool         mGpsWanted = false;                 ///< GPS_LOCATION should stay connected (pre-activity + active track); cleared in disconnect() so the retry never re-wakes the GNSS post-activity.
    bool         mSessionNotEmpty     = false;
    bool         mLapNotEmpty         = false;
    Track::Data  mTrackData{};

    // -- Lifecycle ------------------------------------------------------------

    void connectGps();
    void connectSensors();
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

    // -- Track control --------------------------------------------------------

    void sendInitialInfoToGui();
    void startTrack(std::time_t utc);
    void processTrack();
    void saveLap(float autoLapDistanceM = 0.0f);
    void stopTrack(bool discard);
    void pauseTrack(bool pause, PauseSource source);
    void updateAutoPause();
    void buildPartialSummary();
    ActivityWriter::RecordData prepareRecordData();
    LapDivSource getLapDivSource();

    // -- Notifications --------------------------------------------------------

    void setCapabilities();
    void requestAccessoryPrepare();   // opt in to external HR (pre-warm at GUI start)
    void requestAccessoryRelease();
    void notifyFirstFix();
    void notifyLapEnd();
    void notifyAutoPause(bool paused);
    void notifyNewActivity();
    void backlightOn(uint32_t timeoutMs = skBacklightTimeout);
    void playBuzzerPattern(uint16_t beepMs, uint8_t count = 1, uint16_t silenceMs = 100);
    void playVibroPattern(SDK::Message::RequestVibroPlay::Effect effect, uint8_t count = 1, uint16_t silenceMs = 100);

};

#endif // SERVICE_HPP