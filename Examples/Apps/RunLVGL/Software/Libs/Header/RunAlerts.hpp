/**
 ******************************************************************************
 * @file    RunAlerts.hpp
 * @brief   Settings > Run Alerts: the Run/Walk start, and the pace and
 *          heart-rate range alerts.
 *
 * Run/Walk is a shortcut onto the intervals engine: a plain Start with
 * Run/Walk on runs open repeats of run then walk, with no warm-up or
 * cool-down, from its own two times (Settings::RunAlerts), so the user's
 * Intervals settings are never touched.
 *
 * The range alerts reuse the SDK's workout pieces: a SpeedGauge and a
 * HeartRateGauge (smoothing, deadband and dwell) and ZoneAlerts (when a zone
 * is worth an alert). In front of ZoneAlerts each channel adds:
 *   - a hold: no alert in the first kHoldSec seconds with a reading after the
 *     run, a resume, or (pace) a new run phase or workout step starts;
 *   - no repeats: an Above or Below is not played again until a BackIn has
 *     come between (ZoneAlerts would repeat it every 30 s);
 *   - BackIn only as a tap, and only when the setting asks for it;
 *   - (HR) Low is armed only once the heart rate has reached the range since
 *     the start or the last resume, so a warming-up runner is not told so;
 *   - one pending alert per channel, played once nothing else is sounding.
 *
 * Neither allocates. A Monitor is about 1.2 KB, most of it the SpeedGauge:
 * keep it as a member, not on a task's stack.
 ******************************************************************************
 */

#ifndef RUN_ALERTS_HPP
#define RUN_ALERTS_HPP

#include <cstddef>
#include <cstdint>

#include "SDK/Workout/Intervals.hpp"
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutAlerts.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include "Settings.hpp"
#include "Track.hpp"
#include "RunAlertsPickers.hpp"  // the Settings > Run Alerts pickers' rules

namespace RunAlerts {

// =============================================================================
// Run/Walk start
// =============================================================================

/// What a press of Start runs.
enum class StartMode : uint8_t {
    Free,       ///< A free run
    Intervals,  ///< Intervals, from its own menu
    RunWalk,    ///< Run/Walk, on the intervals engine
    Workout,    ///< A workout from the list
};

/// No workout requested; the same value as CustomMessage::TrackStart::kNoWorkout.
constexpr int16_t kNoWorkout = -1;

/**
 * What Start runs: a workout from the list first, then Intervals started from
 * its own menu, then Run/Walk on a plain Start when it is on, else a free run.
 * With Run/Walk off this is exactly the choice the service made before it.
 * The GUIs use the same rule, so they agree with the service.
 */
inline StartMode startMode(bool intervalsRequested, int16_t workout, const Settings::RunAlerts& cfg)
{
    if (workout != kNoWorkout) {
        return StartMode::Workout;
    }
    if (intervalsRequested) {
        return StartMode::Intervals;
    }
    if (cfg.runWalkEn) {
        return StartMode::RunWalk;
    }
    return StartMode::Free;
}

/// The FIT workout name of a Run/Walk run, in place of "Intervals".
constexpr char kRunWalkName[] = "Run/Walk";

/// The intervals spec for Run/Walk: open repeats of run then walk, both
/// timed, no warm-up or cool-down.
SDK::Workout::IntervalsSpec runWalkSpec(const Settings::RunAlerts& cfg);

// =============================================================================
// Segments: when each channel is on
// =============================================================================

/// Whether the pace and heart-rate alerts are on in a stretch of the run.
struct Segment {
    bool pace;
    bool hr;
};

/// A free run, or the rest of the run after Intervals or a workout.
constexpr Segment kFreeRun = {true, true};

/// An Intervals or Run/Walk phase: pace only while running.
Segment forPhase(Track::IntervalsPhase phase);

/// A workout step: pace only on an active step without its own speed target,
/// heart rate only on a step without its own heart-rate target.
Segment forStep(SDK::Workout::Intensity intensity, bool speedTargeted, bool hrTargeted);

// =============================================================================
// Alerts
// =============================================================================

/// Seconds with a reading before a channel may alert: from the start of the
/// run, every resume, and (pace) every new run phase or workout step. Equal to
/// the SpeedGauge's blend time constant, and longer than its hold-off and
/// dwell and than ZoneAlerts' quiet start, so the first alert can come on the
/// 31st second with a reading.
constexpr uint32_t kHoldSec = 30;

/// An alert waits while the buzzer started less than this long ago.
constexpr uint32_t kQuietMs = 2500;

/// Off for a pace limit: the open fast side of the speed band, m/s.
constexpr float kOpenFastMps = 1000.0f;
/// Off for a heart-rate limit: the open sides of the bpm band.
constexpr float kOpenLowBpm  = 0.0f;
constexpr float kOpenHighBpm = 255.0f;

enum class Kind : uint8_t {
    None,
    PaceFast,  ///< Quicker than Too Fast
    PaceSlow,  ///< Slower than Too Slow
    HrHigh,    ///< Above HR High
    HrLow,     ///< Below HR Low
    BackIn,    ///< Back in range, with the tap on: a vibration tap only
};

struct Alert {
    Kind  kind  = Kind::None;
    float value = 0.0f;  ///< Pace alerts: the smoothed pace, s/m (0 if standing). HR: bpm.
};

/// What the watch does for an alert.
struct Feedback {
    /// The pattern to play once: the workout zone alert's Above or Below.
    /// None: no sound.
    SDK::Workout::ZoneAlerts::Alert sound = SDK::Workout::ZoneAlerts::Alert::None;
    bool                tap       = false;  ///< A vibration tap only
    bool                backlight = false;  ///< Turn the backlight on
    bool                message   = false;  ///< Send CustomMessage::RunAlert (the pop-up)
    Track::RunAlertKind kind      = Track::RunAlertKind::PACE_FAST;  ///< For the message
};

/**
 * What each alert does. The ones that sound turn the backlight on and show
 * the pop-up; back in range is a vibration tap only, with the backlight left
 * as it is.
 */
Feedback feedback(Kind kind);

/// One active second's readings.
struct Input {
    float    speedMps   = 0.0f;   ///< Live speed (GpsSpeedFilter::getSpeed())
    bool     speedValid = false;  ///< A speed reading with a GPS fix, distance caught up
    uint32_t distanceCm = 0;      ///< The run's active distance
    float    hrBpm      = 0.0f;   ///< The displayed heart rate
    bool     hrValid    = false;  ///< A trusted heart-rate reading
};

/**
 * When the buzzer last started, so an alert never plays over another one.
 */
class BuzzClock {
public:
    void started(uint32_t nowMs)
    {
        mStarted = true;
        mAtMs    = nowMs;
    }

    /// True while the buzzer started less than kQuietMs ago.
    bool quiet(uint32_t nowMs) const
    {
        return mStarted && static_cast<uint32_t>(nowMs - mAtMs) < kQuietMs;
    }

private:
    bool     mStarted = false;
    uint32_t mAtMs    = 0;
};

/**
 * The pace and heart-rate range alerts for one run.
 *
 * configure() at the start of the run, then startSegment() for the first
 * stretch and at every change of phase or step; tick() once per active second
 * (never while paused), and resume() when the run resumes. A channel with no
 * limit set never ticks or alerts.
 */
class Monitor {
public:
    /// A new run with these settings. Both channels are off until startSegment().
    void configure(const Settings::RunAlerts& cfg);

    /// A new stretch of the run starts at @p distanceCm. The pace channel
    /// starts afresh wherever it is on; the heart-rate channel only when it
    /// comes on, so walk phases do not restart it.
    void startSegment(Segment segment, uint32_t distanceCm);

    /// The run resumed: hold both channels again and disarm HR Low. An alert
    /// already played stays played, so pausing does not repeat it.
    void resume();

    /// One active second. @p quiet: something else is sounding, so an alert
    /// due now waits. Returns the alert to play now, if any; pace before HR.
    Alert tick(const Input& in, bool quiet);

    bool paceOn() const { return mPaceOn; }
    bool hrOn() const { return mHrOn; }

private:
    /// The rules in front of ZoneAlerts, for one channel.
    class Gate {
    public:
        /// Start afresh: hold, and forget any alert.
        void restart();
        /// Hold again after a resume. An alert still waiting was never heard:
        /// it is dropped and may come again.
        void resume();
        /// One second with the gauge's zone (Below already masked while HR Low
        /// is disarmed).
        void tick(SDK::Workout::Zone zone, bool reading, Kind above, Kind below, bool backInTap);

        Kind takePending();

    private:
        static bool isOut(Kind kind) { return kind != Kind::None && kind != Kind::BackIn; }

        SDK::Workout::ZoneAlerts mAlerts;
        uint32_t                 mHeld    = 0;  ///< Seconds with a reading since (re)starting
        SDK::Workout::Zone       mOut     = SDK::Workout::Zone::Unknown;  ///< Above or Below, until back in
        Kind                     mPending = Kind::None;
    };

    float paceValue() const;

    bool     mBackInTap = false;

    bool     mPaceSet    = false;  ///< A pace limit is set
    bool     mPaceOn     = false;
    float    mPaceLowMps  = 0.0f;
    float    mPaceHighMps = kOpenFastMps;
    uint32_t mPaceStartCm = 0;
    SDK::Workout::SpeedGauge mSpeedGauge;
    Gate     mPaceGate;

    bool     mHrSet     = false;   ///< A heart-rate limit is set
    bool     mHrOn      = false;
    float    mHrLowBpm  = kOpenLowBpm;
    float    mHrHighBpm = kOpenHighBpm;
    bool     mLowArmed  = false;
    SDK::Workout::HeartRateGauge mHrGauge;
    Gate     mHrGate;
};

} // namespace RunAlerts

#endif // RUN_ALERTS_HPP
