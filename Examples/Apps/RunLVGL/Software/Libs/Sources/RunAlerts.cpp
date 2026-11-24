/**
 ******************************************************************************
 * @file    RunAlerts.cpp
 * @brief   The Run/Walk start, and the pace and heart-rate range alerts.
 ******************************************************************************
 */

#include "RunAlerts.hpp"

namespace RunAlerts {

using SDK::Workout::Zone;

// =============================================================================
// Run/Walk start
// =============================================================================

SDK::Workout::IntervalsSpec runWalkSpec(const Settings::RunAlerts& cfg)
{
    using End = SDK::Workout::IntervalsSpec::Phase::End;

    SDK::Workout::IntervalsSpec spec;
    spec.repeats      = 0;  // until the run is ended
    spec.run.end      = End::Time;
    spec.run.timeMs   = cfg.runWalkRunSec * 1000u;
    spec.rest.end     = End::Time;
    spec.rest.timeMs  = cfg.runWalkWalkSec * 1000u;
    spec.warmUp       = false;
    spec.coolDown     = false;
    spec.lastRest     = true;
    return spec;
}

// =============================================================================
// Feedback
// =============================================================================

Feedback feedback(Kind kind)
{
    using Sound = SDK::Workout::ZoneAlerts::Alert;

    Feedback f;
    switch (kind) {
    case Kind::PaceFast:
        f.sound = Sound::Above;
        f.kind  = Track::RunAlertKind::PACE_FAST;
        break;
    case Kind::PaceSlow:
        f.sound = Sound::Below;
        f.kind  = Track::RunAlertKind::PACE_SLOW;
        break;
    case Kind::HrHigh:
        f.sound = Sound::Above;
        f.kind  = Track::RunAlertKind::HR_HIGH;
        break;
    case Kind::HrLow:
        f.sound = Sound::Below;
        f.kind  = Track::RunAlertKind::HR_LOW;
        break;
    case Kind::BackIn:
        // A tap only: no sound, no pop-up, and the backlight left as it is.
        f.tap = true;
        return f;
    default:
        return f;
    }
    f.backlight = true;
    f.message   = true;
    return f;
}

// =============================================================================
// Segments
// =============================================================================

Segment forPhase(Track::IntervalsPhase phase)
{
    // A walk or rest, a warm-up or a cool-down is slow on purpose.
    return Segment{phase == Track::IntervalsPhase::RUN, true};
}

Segment forStep(SDK::Workout::Intensity intensity, bool speedTargeted, bool hrTargeted)
{
    // The step's own target alerts on its own; rest, warm-up and cool-down
    // steps are slow on purpose.
    return Segment{intensity == SDK::Workout::Intensity::Active && !speedTargeted, !hrTargeted};
}

// =============================================================================
// Gate
// =============================================================================

void Monitor::Gate::restart()
{
    mAlerts.stepStarted();
    mHeld    = 0;
    mOut     = Zone::Unknown;
    mPending = Kind::None;
}

void Monitor::Gate::resume()
{
    mHeld = 0;
    if (isOut(mPending)) {
        mOut = Zone::Unknown;
    }
    mPending = Kind::None;
}

void Monitor::Gate::tick(Zone zone, bool reading, Kind above, Kind below, bool backInTap)
{
    if (reading && mHeld <= kHoldSec) {
        ++mHeld;
    }
    // Held: ZoneAlerts keeps quiet while the zone is Unknown, and keeps an
    // alert outstanding, so coming back in after the hold still counts.
    const Zone z = mHeld > kHoldSec ? zone : Zone::Unknown;

    const SDK::Workout::ZoneAlerts::Alert alert = mAlerts.tick(z);
    switch (alert) {
    case SDK::Workout::ZoneAlerts::Alert::Above:
    case SDK::Workout::ZoneAlerts::Alert::Below: {
        // ZoneAlerts repeats an alert every 30 s; once is enough.
        const Zone out = alert == SDK::Workout::ZoneAlerts::Alert::Above ? Zone::Above : Zone::Below;
        if (out != mOut) {
            mOut     = out;
            mPending = out == Zone::Above ? above : below;
        }
        break;
    }
    case SDK::Workout::ZoneAlerts::Alert::BackIn:
        if (mOut == Zone::Unknown) {
            break;  // nothing was heard to come back from
        }
        mOut = Zone::Unknown;
        if (isOut(mPending)) {
            mPending = Kind::None;  // back before it was heard: say nothing
        } else if (backInTap) {
            mPending = Kind::BackIn;
        }
        break;
    default:
        break;
    }
}

Kind Monitor::Gate::takePending()
{
    const Kind kind = mPending;
    mPending = Kind::None;
    return kind;
}

// =============================================================================
// Monitor
// =============================================================================

void Monitor::configure(const Settings::RunAlerts& cfg)
{
    mBackInTap = cfg.backInTap;

    // A limit that is Off leaves its side of the band open.
    mPaceSet     = cfg.paceFastSecPerKm > 0.0f || cfg.paceSlowSecPerKm > 0.0f;
    mPaceLowMps  = cfg.paceSlowSecPerKm > 0.0f ? 1000.0f / cfg.paceSlowSecPerKm : 0.0f;
    mPaceHighMps = cfg.paceFastSecPerKm > 0.0f ? 1000.0f / cfg.paceFastSecPerKm : kOpenFastMps;
    mPaceOn      = false;
    mPaceStartCm = 0;
    mPaceGate.restart();

    mHrSet     = cfg.hrHighBpm > 0 || cfg.hrLowBpm > 0;
    mHrLowBpm  = cfg.hrLowBpm > 0 ? static_cast<float>(cfg.hrLowBpm) : kOpenLowBpm;
    mHrHighBpm = cfg.hrHighBpm > 0 ? static_cast<float>(cfg.hrHighBpm) : kOpenHighBpm;
    mHrOn      = false;
    mLowArmed  = false;
    mHrGate.restart();
}

void Monitor::startSegment(Segment segment, uint32_t distanceCm)
{
    // Pace starts afresh wherever it is on: a new window from here, the
    // hold, and the last stretch's alert forgotten.
    mPaceOn = segment.pace && mPaceSet;
    mPaceGate.restart();
    if (mPaceOn) {
        mPaceStartCm = distanceCm;
        mSpeedGauge.startStep(mPaceLowMps, mPaceHighMps);
    }

    // Heart rate carries on across stretches where it stays on.
    const bool hrOn = segment.hr && mHrSet;
    if (hrOn && !mHrOn) {
        mHrGauge.startStep(mHrLowBpm, mHrHighBpm);
        mHrGate.restart();
        mLowArmed = false;
    } else if (!hrOn) {
        mHrGate.restart();
    }
    mHrOn = hrOn;
}

void Monitor::resume()
{
    if (mPaceOn) {
        mSpeedGauge.resume();
    }
    mPaceGate.resume();

    if (mHrOn) {
        mHrGauge.resume();
    }
    mHrGate.resume();
    mLowArmed = false;
}

Alert Monitor::tick(const Input& in, bool quiet)
{
    if (mPaceOn) {
        const uint32_t stepCm = in.distanceCm > mPaceStartCm ? in.distanceCm - mPaceStartCm : 0u;
        mSpeedGauge.tick(in.speedMps, in.speedValid, stepCm);
        mPaceGate.tick(mSpeedGauge.zone(), in.speedValid, Kind::PaceFast, Kind::PaceSlow, mBackInTap);
    }

    if (mHrOn) {
        mHrGauge.tick(in.hrBpm, in.hrValid);
        Zone zone = mHrGauge.zone();
        if (zone == Zone::In || zone == Zone::Above) {
            mLowArmed = true;
        } else if (zone == Zone::Below && !mLowArmed) {
            zone = Zone::Unknown;
        }
        mHrGate.tick(zone, in.hrValid, Kind::HrHigh, Kind::HrLow, mBackInTap);
    }

    Alert alert;
    if (quiet) {
        return alert;
    }
    // One at a time, pace first: the other plays once this one has.
    alert.kind = mPaceGate.takePending();
    if (alert.kind != Kind::None) {
        alert.value = alert.kind == Kind::BackIn ? 0.0f : paceValue();
        return alert;
    }
    alert.kind = mHrGate.takePending();
    if (alert.kind != Kind::None && alert.kind != Kind::BackIn) {
        alert.value = mHrGauge.valueBpm();
    }
    return alert;
}

float Monitor::paceValue() const
{
    const float mps = mSpeedGauge.valueMps();
    return mps > 0.05f ? 1.0f / mps : 0.0f;
}

} // namespace RunAlerts
