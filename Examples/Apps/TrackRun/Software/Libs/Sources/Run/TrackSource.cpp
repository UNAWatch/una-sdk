/*******************************************************************************
 * @file   TrackSource.cpp
 * @brief  Run: the run's distance, live speed and altitude (TrackSource.hpp).
 ******************************************************************************/

#include "Run/TrackSource.hpp"

#include <cmath>

namespace Run {

using Oval::Session;
using Oval::Tracker;

namespace {

int32_t diff(uint32_t later, uint32_t earlier)
{
    return static_cast<int32_t>(later - earlier);
}

} // namespace

TrackSource::TrackSource(const Oval::Config& cfg) noexcept : mSession(cfg) {}

PreRun TrackSource::preRun(const Oval::TrackStore* store) const noexcept
{
    PreRun p;
    double lat = 0.0, lon = 0.0;
    if (!mSession.primedPosition(lat, lon)) {
        return p; // no fix yet
    }
    const Oval::TrackRecord* rec = store != nullptr ? store->findNear(lat, lon) : nullptr;
    if (rec != nullptr) {
        p.status      = PreRun::Status::Stored;
        p.trackNumber = rec->number;
        p.uses        = rec->uses;
    } else {
        p.status = PreRun::Status::New;
    }
    return p;
}

bool TrackSource::start(uint32_t tMs, uint8_t lane, const Oval::LaneTable& table,
                        const Oval::TrackStore* store) noexcept
{
    if (!mSession.start(tMs, lane, table, store)) {
        return false;
    }
    mRunning     = true;
    mPaused      = false;
    mHaveReading = false;
    mLastT       = tMs;
    mCorrection  = false;
    mPace.reset();
    mWeak.reset();
    mTicks.reset();
    mTicks.anchor(tMs);
    mHeld      = false;
    mHeldValid = false;
    mHaveBaro  = false;
    mEffAsc    = 0.0f;
    mEffDesc   = 0.0f;
    return true;
}

void TrackSource::reading(const Oval::Sample& s) noexcept
{
    if (!mRunning) {
        return;
    }
    if (mHaveReading && diff(s.tMs, mLastT) <= 0) {
        return; // the Session ignores it too
    }
    mHaveReading = true;
    const float    before = mSession.totalDistanceM();
    const uint32_t prevT  = mLastT;
    const float    recent = mPace.speedMps();

    const Session::Events ev = mSession.update(s);
    mLastT = s.tMs;
    mWeak.add(s.tMs, s.fix, s.hdop, mSession.config().maxHdop);

    bool forced = mCorrection;
    for (uint8_t k = 0; k < ev.count; ++k) {
        if (ev.items[k].type == Session::EventType::SegmentChanged) {
            forced = true; // onto a track or off it, maybe from some seconds back
        }
    }

    // Live speed: a correction in the total is not running (no pace spike).
    const float after = mSession.totalDistanceM();
    mPace.add(s.tMs, after);
    if (!mPaused) {
        const uint32_t dt   = diff(s.tMs, prevT) > 0 ? static_cast<uint32_t>(diff(s.tMs, prevT)) : 0u;
        const float    jump = correctionJumpM(after - before, dt, recent, forced);
        if (jump > 0.0f) {
            mPace.rebase(jump);
        }
    }
    mCorrection = false;
}

void TrackSource::tick(uint32_t nowMs, const Baro& baro) noexcept
{
    if (!mRunning || mPaused) {
        return;
    }
    elevation(baro);
    float alt = std::nanf("");
    altitude(alt);
    // Kept for the climb since a join found late (since()).
    mTicks.add(nowMs, 0.0f, mPace.speedMps(), alt, mEffAsc, mEffDesc);
}

void TrackSource::pause(uint32_t tMs) noexcept
{
    if (!mRunning || mPaused) {
        return;
    }
    mSession.pause(tMs);
    mPaused = true;
}

void TrackSource::resume(uint32_t tMs) noexcept
{
    if (!mRunning || !mPaused) {
        return;
    }
    mSession.resume(tMs);
    mPaused = false;
    // The pre-pause window describes the effort before the break.
    mPace.reset();
}

bool TrackSource::setLane(uint8_t lane, uint32_t tMs) noexcept
{
    if (!mRunning) {
        return false;
    }
    bool ok = false;
    const float before = mSession.totalDistanceM();
    const Session::Events ev = mSession.setLane(lane, tMs, &ok);
    // A lane counts from here, so the total does not usually move; any rise
    // here, between two readings, is a correction, not running, and the
    // window moves with it.
    const float jump = mSession.totalDistanceM() - before;
    if (jump > 0.0f) {
        mPace.rebase(jump);
    }
    if (ev.count > 0 || ok) {
        mCorrection = true; // the next rise may be a jump too (a join replayed)
    }
    return ok;
}

void TrackSource::finish() noexcept
{
    if (!mRunning) {
        return;
    }
    (void)mSession.finish();
    mRunning = false;
}

void TrackSource::elevation(const Baro& baro) noexcept
{
    if (baro.valid) {
        if (mHaveBaro && !mHeld) {
            const float dA = baro.ascentM - mLastBaroAsc;
            const float dD = baro.descentM - mLastBaroDesc;
            mEffAsc += dA > 0.0f ? dA : 0.0f;
            mEffDesc += dD > 0.0f ? dD : 0.0f;
        }
        mLastBaroAsc  = baro.ascentM;
        mLastBaroDesc = baro.descentM;
        mBaroAlt      = baro.altM;
        mHaveBaro     = true;
    }
    const bool hold = mSession.holdElevation();
    if (hold && !mHeld) {
        // Held at the barometric altitude where the runner joined the track,
        // and any climb since a join found late is backed out.
        mHeld      = true;
        mHeldValid = false;
        float asc = 0.0f, desc = 0.0f, alt = 0.0f;
        const uint32_t from = mSession.joinTMs();
        if (mTicks.since(from, asc, desc, alt) && std::isfinite(alt)) {
            mHeldAlt   = alt;
            mHeldValid = true;
            mEffAsc    = mEffAsc > asc ? mEffAsc - asc : 0.0f;
            mEffDesc   = mEffDesc > desc ? mEffDesc - desc : 0.0f;
            mTicks.flattenFrom(from, alt);
        } else if (mHaveBaro) {
            mHeldAlt   = mBaroAlt;
            mHeldValid = true;
        } else {
            const float m = mSession.tracker().model().altitudeM();
            if (std::isfinite(m)) {
                mHeldAlt   = m;
                mHeldValid = true;
            }
        }
    } else if (hold && !mHeldValid && mHaveBaro) {
        // No altitude when the hold began (the barometer not read yet): held
        // at the first one.
        mHeldAlt   = mBaroAlt;
        mHeldValid = true;
    } else if (!hold && mHeld) {
        mHeld = false;
    }
}

bool TrackSource::altitude(float& altM) const noexcept
{
    if (mHeld && mHeldValid) {
        altM = mHeldAlt;
        return true;
    }
    if (mHaveBaro) {
        altM = mBaroAlt;
        return true;
    }
    return false;
}

bool TrackSource::lanePosition(double& lat, double& lon) const noexcept
{
    return mSession.lanePositions() && mSession.lanePosition(lat, lon);
}

TrackStatus TrackSource::status(uint32_t nowMs) const noexcept
{
    const Session& s = mSession;
    const Tracker& T = s.tracker();
    TrackStatus st;
    st.segment     = s.segment();
    st.trackNumber = s.trackNumber();
    st.lane        = s.lane();
    st.autoJoins   = s.autoJoins();
    st.weakGps     = mRunning && mWeak.weak(nowMs);

    const bool onTrack = st.segment == Session::Segment::Track && T.isRunning();
    if (!onTrack) {
        return st;
    }
    switch (T.calibrationStage()) {
    case Tracker::CalStage::Supplied:
        if (s.trackNumber() != 0) {
            st.cal = CalState::Stored;
        } else {
            // A track not stored, visited earlier in the run and joined again
            // on that visit's fit: follow the background calibration.
            switch (T.calibrator().stage()) {
            case Oval::Calibrator::Stage::Collecting:  st.cal = CalState::Calibrating; break;
            case Oval::Calibrator::Stage::Provisional:
            case Oval::Calibrator::Stage::Refined:     st.cal = CalState::Provisional; break;
            case Oval::Calibrator::Stage::Locked:      st.cal = CalState::Locked; break;
            case Oval::Calibrator::Stage::Rejected:    st.cal = CalState::NotStandard; break;
            }
        }
        break;
    case Tracker::CalStage::None:
        st.cal = T.mode() == Tracker::Mode::NotStandard ? CalState::NotStandard : CalState::Calibrating;
        break;
    case Tracker::CalStage::Provisional: st.cal = CalState::Provisional; break;
    case Tracker::CalStage::Locked:      st.cal = CalState::Locked; break;
    case Tracker::CalStage::Rejected:    st.cal = CalState::NotStandard; break;
    }
    st.calLaps = T.calibrationLaps();
    return st;
}

const char* TrackSource::traceMode() const noexcept
{
    if (mPaused) {
        return "pause";
    }
    const Tracker& T = mSession.tracker();
    if (mSession.segment() == Session::Segment::Track && T.isRunning() && T.mode() == Tracker::Mode::Calibrated) {
        return "cal";
    }
    return "count";
}

const char* TrackSource::traceSegment() const noexcept
{
    switch (mSession.segment()) {
    case Session::Segment::Road:  return "road";
    case Session::Segment::Track: return "track";
    case Session::Segment::None:  break;
    }
    return "";
}

} // namespace Run
