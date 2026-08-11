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
    // samples.
    //
    // The dwell was 3 samples originally. A 33 min ride with 13 auto-pauses,
    // replayed through the detector offline, put the whole measured latency in
    // the dwell -- 3.2 s on average from the sample that crossed the threshold
    // to the pause, and nothing else. Two samples takes 1 s off every stop. The
    // cost measured on that ride was a single spurious pause, on a genuine
    // 1.5 km/h crawl the rider then rode out of; it lasts until the resume
    // dwell clears and loses no distance at that speed.
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
    static constexpr uint8_t skAutoPauseDwellSec  = 2;     // sustained samples either way
    static constexpr uint8_t skAutoPauseStaleSec  = 5;     // hold state after this long with no valid speed

    // -- Braking fast path ----------------------------------------------------
    //
    // The dwell alone cannot help a rider who stops from speed: they spend the
    // approach above skAutoPauseSpeedMps, so the detector cannot start counting
    // until they are almost stationary. On the analysed ride the stops from
    // 17-27 km/h were the slowest to register, which matches the field report
    // that stopping from a higher speed took longer.
    //
    // So: arm on a sample that is BOTH slow and braking hard, and pause when
    // the next sample is still slow. A rider under 7 km/h shedding more than
    // skAutoPauseBrakingDecelMps2, and still slow a second later, is stopping;
    // waiting for them to coast out the last 5 km/h adds nothing. The
    // confirmation is what separates that from braking hard for a junction and
    // rolling through, which the arming sample alone cannot do -- the traces
    // are identical until the rider either puts a foot down or accelerates
    // away. On the analysed ride the fast path pauses 3 s earlier than the
    // dwell on every stop from speed, and never fires on a steady slow rider:
    // their speed is low but not falling. Replaying at 1.5 m/s^2 gave results
    // identical to 2.5, which means that ride has no samples in the
    // discriminating band -- so 2.5 is chosen as the conservative end of an
    // interval this data cannot separate, not as a measured optimum.
    //
    // Deceleration is measured against the last sample whose value actually
    // changed, aged in ticks. GPS speed repeats its previous value when the
    // receiver has no fresh RMC (30 % of samples on the analysed ride), and a
    // naive first difference would read those as zero deceleration and then as
    // a huge one on the update that follows.
    static constexpr float   skAutoPauseBrakingCeilMps   = 2.0f;  // only below this, m/s (7.2 km/h)
    static constexpr float   skAutoPauseBrakingDecelMps2 = 2.5f;  // and shedding at least this
    static constexpr uint8_t skAutoPauseDecelMaxAgeTicks = 3;     // beyond this the reference is too old to trust

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

        /// Reference for the braking fast path: the last speed whose value
        /// actually changed, and how many ticks ago it was taken. Ageing in
        /// ticks (rather than assuming one) keeps the rate honest; once the
        /// reference passes skAutoPauseDecelMaxAgeTicks it is dropped outright
        /// (prevValid = false) rather than divided by a clamped age, which
        /// would overstate the deceleration instead of discounting it.
        float   prevSpeedMps = 0.0f;
        uint8_t prevAgeTicks = 0;
        bool    prevValid    = false;

        /// The previous sample armed the braking fast path; the next sample
        /// still being slow confirms it. Dwell-like evidence, so it clears
        /// wherever the dwell counters do.
        bool    brakingArmed = false;

        /// Clear the dwell counters only; freshness tracking is unaffected.
        void resetDwell()
        {
            belowSec     = 0;
            aboveSec     = 0;
            brakingArmed = false;
        }

        void reset()
        {
            staleSec     = skAutoPauseStaleSec;
            prevSpeedMps = 0.0f;
            prevAgeTicks = 0;
            prevValid    = false;
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