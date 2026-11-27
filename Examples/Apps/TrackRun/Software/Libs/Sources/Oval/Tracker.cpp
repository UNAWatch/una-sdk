/*******************************************************************************
 * @file   Tracker.cpp
 * @brief  Oval: counting mode, calibration and calibrated mode together. See
 *         Tracker.hpp and docs/OVAL.md.
 ******************************************************************************/

#include "Oval/Tracker.hpp"

#include <cmath>

namespace Oval {

namespace {

/** How much of each reading goes into the smoothed position the running
 *  direction is judged by. */
constexpr float kDirSmooth = 0.3f;

/** Lining up waits for this many calibration points (about 18 m), and each
 *  step of its solve moves the offset at most this far. */
constexpr uint16_t kAlignMinPoints = 6;
constexpr float    kAlignMaxStepM  = 10.0f;
constexpr float    kAlignSimilarFit = 0.25f; // fits this close are alike

/** The mid-run switch waits while counting mode's distance runs this far (a
 *  fraction of the lap) ahead of where the model puts the runner: it has not
 *  seen a pass of the line, and the lap is still its to credit. */
constexpr float kSwitchOverdueFrac = 0.5f;

/** The HDOP at which Config::alignMaxScaleM was set (the return-visit tests' noise
 *  is the realistic HDOP's noise at 1.0). Points poorer than this on average
 *  widen the limit in proportion. */
constexpr float kAlignScaleRefHdop = 1.0f;

/** The turn test at the end of lining up (solveTurn). A stored
 *  track that lines up best turned by kTurnLimitDeg or more, and by
 *  kTurnSigma standard errors, is another track: the visit's own locked fit
 *  of a right track is within 1.5 degrees of the stored one, a mis-stored
 *  heading within about 6 (ReturnVisit.ChangedTrackReplacesTheStoredOne),
 *  and a neighbour beside it turned 15 degrees or more. */
constexpr float kTurnLimitDeg   = 10.0f;
constexpr float kTurnSigma      = 4.0f;
constexpr int   kTurnIterations = 8;
constexpr float kTurnMaxStepRad = 0.0873f; // 5 degrees a step

/** After lining up, the turn test is repeated every kTurnEvery
 *  calibration points (30 m) until the visit's first fit confirms the stored
 *  track, or, with no fit, until counting mode has counted kConfirmLaps laps.
 *  A turn drops the track only on kTurnMinPathM of points or more, found by
 *  kTurnRuns tests in a row the same way round (turnTest). */
constexpr uint16_t kTurnEvery    = 10;
constexpr uint16_t kConfirmLaps  = 2;
constexpr float    kTurnMinPathM = 200.0f;
constexpr int8_t   kTurnRuns     = 2;

inline float clampf(float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

/** Solves the symmetric 3 x 3 system A x = b, A as a00 a01 a02 a11 a12 a22. */
bool solve3(const float A[6], const float b[3], float x[3]) noexcept
{
    const float c00 = A[3] * A[5] - A[4] * A[4];
    const float c01 = A[2] * A[4] - A[1] * A[5];
    const float c02 = A[1] * A[4] - A[2] * A[3];
    const float det = A[0] * c00 + A[1] * c01 + A[2] * c02;
    if (!(std::fabs(det) > 1e-12f)) {
        return false;
    }
    const float c11 = A[0] * A[5] - A[2] * A[2];
    const float c12 = A[1] * A[2] - A[0] * A[4];
    const float c22 = A[0] * A[3] - A[1] * A[1];
    x[0] = (c00 * b[0] + c01 * b[1] + c02 * b[2]) / det;
    x[1] = (c01 * b[0] + c11 * b[1] + c12 * b[2]) / det;
    x[2] = (c02 * b[0] + c12 * b[1] + c22 * b[2]) / det;
    return true;
}

/** t0 + f x (t1 - t0) on the wrapping clock, for f in [0, 1]. */
inline uint32_t lerpTime(uint32_t t0, uint32_t t1, float f) noexcept
{
    const int32_t dt = msDiff(t1, t0);
    if (dt <= 0) {
        return t0;
    }
    return t0 + static_cast<uint32_t>(static_cast<float>(dt) * clampf(f, 0.0f, 1.0f) + 0.5f);
}

} // namespace

Tracker::Tracker(const Config& cfg) noexcept
    : mCfg(cfg), mCounter(cfg), mCal(cfg)
{
    reset();
}

void Tracker::setConfig(const Config& cfg) noexcept
{
    mCfg = cfg;
    mCounter.setConfig(cfg);
    mCal.setConfig(cfg);
    reset();
}

void Tracker::reset() noexcept
{
    mCounter.reset();
    mTable = LaneTable();
    clearRun();
}

void Tracker::clearRun() noexcept
{
    mCal.reset(mTable);
    mModel.clear();
    mTiming.clear();

    mRunning  = false;
    mHaveT    = false;
    mSupplied = false;
    mStartT   = 0;
    mLastT    = 0;
    mLane     = 1;
    mMode     = Mode::Counting;

    mPaused        = false;
    mHadPause      = false;
    mPauseStartT   = 0;
    mPausedTotalMs = 0;
    mLastPauseT    = 0;
    mLastPauseMs   = 0;
    mResumeT       = 0;
    mPauses.reset();

    mCreditedM      = 0.0;
    mLaps           = 0;
    mMissed         = 0;
    mLapStartT      = 0;
    mLapStartActive = 0;
    mLastLapMs      = 0;
    mLaneChanged    = false;
    mMarkCount      = 0;
    mHdop           = -1.0f;
    mHeldCount      = 0;
    mHaveLastP      = false;
    mLastP          = Vec2();

    mShownM       = 0.0f;
    mLargestHoldM = 0.0f;

    mPlaced     = false;
    mWantSwitch = false;
    mSwitchT    = 0;
    mLine       = false;
    mTau0       = 0.0f;
    mTau0Final  = false;
    mArmedStart = false;
    mFreezeStartP = false;
    mX          = 0.0f;
    mV          = 0.0f;
    mFilterT    = 0;
    mUmax       = 0.0f;
    mLapMetres  = 0.0f;
    mAltered    = false;
    mOutliers   = 0;
    mNeedAnchor = false;
    mLastCounterTotal = 0.0f;
    mRawValid   = false;
    mRawTotal   = 0.0f;

    mOff       = false;
    mLeft      = false;
    mOffRun    = 0;
    mOnRun     = 0;
    mOffSinceT = 0;
    mOffFirstT = 0;
    mOffV      = 0.0f;
    mRejoinRel = 0.0f;
    mRejoinT   = 0;
    mSafeUmax   = 0.0f;
    mSafeT      = 0;
    mHaveSafe   = false;
    mLastZ      = 0.0f;
    mHaveZ      = false;

    mHaveLateral = false;
    mOffLanesM   = 0.0f;
    mLateralM    = 0.0f;

    mDriftOn = false;
    mDrift   = Vec2();
    mDA[0] = mDA[1] = mDA[2] = 0.0f;
    mDB[0] = mDB[1] = 0.0f;
    mDW = mDN = 0.0f;

    mRefine[0] = Refine();
    mRefine[1] = Refine();
    mPressed   = false;

    mStored.clear();
    mStoredGiven = false;
    mAlign       = Alignment();
    mAlignA[0] = mAlignA[1] = mAlignA[2] = 0.0f;
    mAlignPts    = 0;
    mTurnPts     = 0;
    mTurnRun     = 0;
    mHaveStartP  = false;
    mStartP      = Vec2();
    mHaveDirP    = false;
    mDirP        = Vec2();
    mDirAcc      = 0.0f;
    mHaveT0      = false;
    mT0FirstS    = 0.0f;
    mVisit       = VisitCheck();
}

bool Tracker::isGood(const Sample& s) const noexcept
{
    return s.fix && std::isfinite(s.lat) && std::isfinite(s.lon)
        && !(s.hdop > mCfg.maxHdop);
}

// ---------------------------------------------------------------------------
// Start, stop, pause, lane
// ---------------------------------------------------------------------------

bool Tracker::start(uint32_t tMs, uint8_t lane, const LaneTable& table, const TrackModel* supplied) noexcept
{
    if (!table.isValidLane(lane)) {
        return false;
    }
    if (!mCounter.start(tMs, lane, table)) {
        return false;
    }
    mTable = table;
    clearRun();
    mRunning   = true;
    mLane      = lane;
    mStartT    = tMs;
    mLastT     = tMs;
    mLapStartT = tMs;

    if (supplied != nullptr && supplied->isValid() && supplied->hasGeo()) {
        // A stored track: calibrated from the first reading. It is first
        // lined up with the day's GPS (alignStep), then drift tracking takes
        // over.
        mModel          = *supplied;
        mStored         = *supplied;
        mStoredGiven    = true;
        mSupplied       = true;
        mAlign.supplied = true;
        mAlign.aligning = true;
        mMode           = Mode::Calibrated;
        mSwitchT        = tMs;
    } else {
        mMode = Mode::Calibrating;
    }
    return true;
}

Tracker::Events Tracker::dropStoredPick(uint32_t tMs) noexcept
{
    Events ev;
    if (mRunning && mAlign.aligning) {
        abandonStored(tMs, ev);
    }
    return ev;
}

Tracker::Events Tracker::finish() noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    onCounterEvents(mCounter.finish(), mLastT, ev);
    checkRefines(mLastT, true, ev);
    return ev;
}

void Tracker::pause(uint32_t tMs) noexcept
{
    mCounter.pause(tMs);
    if (!mRunning || mPaused) {
        return;
    }
    // The runner ran on from the last reading to the pause: that much is
    // credited, at the speed then, as the odometer credits its share of the
    // interval. Short of the line, so the lap still ends on a reading.
    if (mLine && !mOff && mMode == Mode::Calibrated && mV >= mCfg.stillMps) {
        const int32_t sinceMs = msDiff(tMs, mFilterT);
        if (sinceMs > 0 && sinceMs <= static_cast<int32_t>(mCfg.maxCrossGapMs)) {
            const float onto   = mX + mV * 0.001f * static_cast<float>(sinceMs);
            const float before = lapLen() - 0.5f;
            const float to     = onto < before ? onto : before;
            if (to > mUmax) {
                mLapMetres += to - mUmax;
                mUmax       = to;
            }
        }
    }
    mPaused      = true;
    mPauseStartT = tMs;
    mPauses.pause(tMs);
}

void Tracker::resume(uint32_t tMs) noexcept
{
    mCounter.resume(tMs);
    if (!mRunning || !mPaused) {
        return;
    }
    mPauses.resume(tMs);
    int32_t len = msDiff(tMs, mPauseStartT);
    if (len < 0) {
        len = 0;
    }
    mPausedTotalMs += static_cast<uint32_t>(len);
    mLastPauseT  = mPauseStartT;
    mLastPauseMs = static_cast<uint32_t>(len);
    mResumeT     = tMs;
    mHadPause    = true;
    mOffRun      = 0; // judged afresh from the resume
    mOnRun       = 0;
    if (mOff) {
        // Off the track at the pause: back on, the spell credits nothing
        // along the track, as for a pause on it.
        mOffV = 0.0f;
    }
    mPaused      = false;
    // Where the runner went while paused is not credited.
    mNeedAnchor = mLine;
}

bool Tracker::setLane(uint8_t lane) noexcept
{
    if (!mTable.isValidLane(lane)) {
        return false;
    }
    if (mRunning) {
        mCounter.setLane(lane);
    }
    if (mRunning && mLine && lane != mLane) {
        // The runner stays where they are; only the line under them changes.
        // Positions are carried over by station, so the rest of the lap is
        // measured on the new lane's line.
        const float oldOff = lineOffset();
        const float oldLen = lapLen();
        const float s0     = startS();
        const float kx     = std::floor(mX / oldLen);
        const float ku     = std::floor(mUmax / oldLen);
        const float tauX   = mModel.tauOf(s0 + mX, oldOff);
        const float tauU   = mModel.tauOf(s0 + mUmax, oldOff);
        mLane = lane;
        const float newLen = lapLen();
        mX        = relOf(tauX) + kx * newLen;
        mUmax     = relOf(tauU) + ku * newLen;
        mAltered  = true;
        mHaveSafe = false;
        mHaveZ    = false;
    }
    if (mRunning && !mLine && lane != mLane && mHaveLastP) {
        // Before calibrated mode: remember where, so the lap can be
        // re-measured lane by lane if calibrated mode takes over in it.
        // With no position yet there is nothing to measure from: the lap
        // is then measured in the lane it ends in.
        if (mMarkCount < kMaxLaneMarks) {
            mMarks[mMarkCount].p          = mLastP;
            mMarks[mMarkCount].laneBefore = mLane;
        }
        if (mMarkCount < 255) {
            ++mMarkCount;
        }
    }
    mLane = lane;
    if (mRunning) {
        mLaneChanged = true;
    }
    return true;
}

Tracker::Events Tracker::restartLap(uint32_t tMs) noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    const uint32_t now = (mHaveT && msDiff(tMs, mLastT) < 0) ? mLastT : tMs;
    // Lap times still waiting to be refined are settled with the readings so
    // far, before the line they are measured against moves.
    checkRefines(now, true, ev);

    // Counting mode starts its gate again round where the runner is. A pass
    // it was still holding is credited first; its part-lap stays credited.
    const bool   line   = mLine && !mLeft;
    const double before = mCounter.creditedDistanceM();
    const LapCounter::Events cev = mCounter.restartLap(now);
    double laps = 0.0;
    for (uint8_t k = 0; k < cev.count; ++k) {
        laps += static_cast<double>(cev.items[k].lapLengthM);
    }
    const double counterPart = static_cast<double>(mCounter.creditedDistanceM()) - before - laps;
    onCounterEvents(cev, now, ev);

    if (line) {
        // The start line moves to the furthest point reached; what was run
        // of the lap is credited as distance.
        mCreditedM += static_cast<double>(mLapMetres);
        const float off  = lineOffset();
        const float tauU = mModel.tauOf(startS() + mUmax, off);
        mStartP     = mModel.pointAt(tauU, off) + mDrift;
        mHaveStartP = true;
        mTau0       = tauU;
        mTau0Final  = true;
        mArmedStart = true;
        mX         -= mUmax;
        mLastZ     -= mUmax;
        mRejoinRel -= mUmax; // a rejoin under way: its first reading, from the new line
        mUmax       = 0.0f;
        mLapMetres  = 0.0f;
        mHaveSafe   = false;
    } else if (counterPart > 0.0 && !(mMode == Mode::Calibrated || (mLine && mLeft))) {
        // The total is counting mode's (measuredDistanceM()): its part-lap,
        // credited by the counter at the press, moves into the credited
        // metres. Calibrated, or off the track with the line kept, the total
        // is this lap's own metres, which the press leaves as they are.
        mCreditedM += counterPart;
    }
    mAltered        = false;
    mLaneChanged    = false;
    mMarkCount      = 0;
    mLapStartT      = now;
    mLapStartActive = activeAt(now);
    mPressed        = true;
    return ev;
}

float Tracker::measuredDistanceM() const noexcept
{
    if (mMode == Mode::Calibrated || (mLine && mLeft)) {
        return static_cast<float>(mCreditedM) + mLapMetres;
    }
    return static_cast<float>(mCreditedM) + mCounter.lapProgressM();
}

// ---------------------------------------------------------------------------
// Per reading
// ---------------------------------------------------------------------------

Tracker::Events Tracker::update(const Sample& s) noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    if (mHaveT && msDiff(s.tMs, mLastT) <= 0) {
        return ev;
    }
    mHaveT = true;
    mLastT = s.tMs;

    // Counting mode always runs, so it is ready whenever it is needed.
    const LapCounter::Events cev = mCounter.update(s);

    const LocalFrame& frame = mCounter.frame();
    const bool good = isGood(s) && frame.hasOrigin();
    mHdop = s.hdop;
    const Vec2 p    = good ? frame.toLocal(s.lat, s.lon) : Vec2();
    if (good) {
        mLastP     = p;
        mHaveLastP = true;
    }

    if (mSupplied && !mPlaced && frame.hasOrigin()) {
        mPlaced = mModel.placeIn(frame);
        if (mPlaced) {
            mStored = mModel;
        }
    }

    // Calibration collects good readings while running on the track. On a
    // stored track it runs in the background, for the visit check.
    // While the stored track is still to be confirmed, being off its line
    // says as much about the stored track as about the runner (a neighbour
    // turned from this one), so the points are kept: the turn test and the
    // visit's first fit need them.
    if (good && !mPaused && (!mOff || storedUnconfirmed()) && !mCal.isFinal()) {
        onCalibration(mCal.add(p, mLane, s.altM, s.hdop), s.tMs, ev);
    }
    // Lining a stored track up with the day's GPS, from those points.
    if (mAlign.aligning && mPlaced && mCal.pointCount() != mAlignPts) {
        alignStep(s.tMs, ev);
    }
    // Lined up, but not yet confirmed by the visit's first fit: is
    // it turned from this track after all? Every kTurnEvery points (30 m).
    if (mAlign.done && storedUnconfirmed() && mPlaced && mCal.pointCount() >= mTurnPts + kTurnEvery) {
        turnStep(s.tMs, ev);
    }
    if (mAlign.done && storedUnconfirmed() && mCounter.lapsCompleted() >= kConfirmLaps) {
        confirmStored(2); // no fit by then to check it against: confirmed by laps
    }

    onCounterEvents(cev, s.tMs, ev);

    mRawValid = false;
    if (good && mModel.isValid() && (!mSupplied || mPlaced) && mMode != Mode::NotStandard) {
        lineReading(p, s.tMs, ev);
    }

    // The shown total: calibrated progress on the line, or counting mode's
    // estimate. It never goes back; when the measured figure falls below it
    // (after a switch, a rejoin or a refit) it holds until that catches up.
    float cand = 0.0f;
    if (mMode == Mode::Calibrated || (mLine && mLeft)) {
        cand = static_cast<float>(mCreditedM) + mLapMetres;
    } else {
        cand = static_cast<float>(mCreditedM) + mCounter.lapProgressM();
    }
    if (cand > mShownM) {
        mShownM = cand;
    } else if (mShownM - cand > mLargestHoldM) {
        mLargestHoldM = mShownM - cand;
    }

    LapTiming::Entry e;
    e.tMs = s.tMs;
    if (good) {
        e.p      = p;
        e.flags |= LapTiming::kPosition;
    }
    if (mRawValid) {
        e.total  = mRawTotal;
        e.flags |= LapTiming::kCalibrated;
    } else if (mMode != Mode::Calibrated) {
        e.total  = mShownM;
        e.flags |= LapTiming::kCounting;
    }
    mTiming.push(e);

    checkRefines(s.tMs, false, ev);
    return ev;
}

void Tracker::addEvent(Events& ev, EventType type, uint32_t tMs) noexcept
{
    if (ev.count >= kMaxEvents) {
        return;
    }
    Event& e = ev.items[ev.count++];
    e = Event();
    e.type   = type;
    e.tMs    = tMs;
    e.totalM = static_cast<float>(mCreditedM);
    e.mode   = mMode;
}

void Tracker::setMode(Mode m, uint32_t tMs, Events& ev) noexcept
{
    if (m == mMode) {
        return;
    }
    mMode = m;
    addEvent(ev, EventType::ModeChanged, tMs);
}

void Tracker::completeLap(EventType type, uint32_t endMs, float lengthM, Events& ev) noexcept
{
    const uint32_t endActive = activeAt(endMs);
    const uint32_t dur = endActive > mLapStartActive ? endActive - mLapStartActive : 0;
    mCreditedM += static_cast<double>(lengthM);
    ++mLaps;
    if (type == EventType::MissedLap) {
        ++mMissed;
    }
    if (storedUnconfirmed() && mLaps <= kMaxHeldLaps) {
        mShownLen[mLaps - 1]   = lengthM;
        mShownTotal[mLaps - 1] = mCreditedM;
    }
    mLastLapMs = dur;
    if (ev.count < kMaxEvents) {
        Event& e        = ev.items[ev.count++];
        e               = Event();
        e.type          = type;
        e.lapNumber     = mLaps;
        e.tMs           = endMs;
        e.lapDurationMs = dur;
        e.lapLengthM    = lengthM;
        e.totalM        = static_cast<float>(mCreditedM);
        e.mode          = mMode;
    }
    mLapStartT      = endMs;
    mLapStartActive = endActive;
    mLaneChanged    = false;
    mMarkCount      = 0;
    mPressed        = false;
}

void Tracker::onCounterEvents(const LapCounter::Events& cev, uint32_t nowMs, Events& ev) noexcept
{
    (void)nowMs;
    for (uint8_t k = 0; k < cev.count; ++k) {
        const LapCounter::Event& c = cev.items[k];
        // In calibrated mode its own laps count; counting mode's are taken
        // only for passes before calibrated mode took over. While a stored
        // track is lined up and until the visit's first fit confirms it,
        // they are kept, in case it is a wrong pick.
        if (mMode == Mode::Calibrated && !msAfter(mSwitchT, c.tMs)) {
            if (suppliedUnsettled() && mHeldCount < kMaxHeldLaps) {
                mHeld[mHeldCount++] = c;
            }
            continue;
        }
        // The same passage of the line already credited (a calibrated lap,
        // or a lap credited before a fallback): real laps are far apart.
        if (mLaps > 0 && msDiff(c.tMs, mLapStartT) < static_cast<int32_t>(mCfg.minLapMs / 2)) {
            continue;
        }
        const uint32_t startActive = mLapStartActive;
        completeLap(c.type == LapCounter::EventType::Lap ? EventType::Lap : EventType::MissedLap, c.tMs,
                    c.lapLengthM, ev);
        if (c.type == LapCounter::EventType::Lap) {
            scheduleRefine(false, c.tMs, 0.0f, startActive);
        }
        if (mLine && mLeft) {
            // Off the track, counting mode saw the runner pass the start
            // line: the calibrated lap starts again from there.
            const float len = lapLen();
            mX         -= len;
            mUmax      -= len;
            mLapMetres  = 0.0f;
            mAltered    = true;
        }
    }
}

void Tracker::onCalibration(Calibrator::Change c, uint32_t tMs, Events& ev) noexcept
{
    switch (c) {
    case Calibrator::Change::None:
        return;
    case Calibrator::Change::Provisional:
    case Calibrator::Change::Refined:
    case Calibrator::Change::Locked: {
        if (mSupplied) {
            // The stored track is in use: this run's own fit is only for
            // checking it. Its first fit, if not yet confirmed: is
            // the stored track turned from this one?
            if (mAlign.done && storedUnconfirmed()) {
                firstFitCheck(tMs, ev);
                if (!mSupplied) {
                    return; // dropped: this fit carries the run on
                }
            }
            if (c == Calibrator::Change::Locked) {
                checkVisit();
            }
            return;
        }
        TrackModel m = mCal.model();
        m.setGeo(mCounter.frame());
        if (c == Calibrator::Change::Provisional) {
            addEvent(ev, EventType::CalibrationProvisional, tMs);
        }
        if (c == Calibrator::Change::Locked) {
            addEvent(ev, EventType::CalibrationLocked, tMs);
        }
        if (mLine) {
            rebaseModel(m, mDrift);
        } else {
            mModel = m;
            if (mMode == Mode::Calibrating) {
                mWantSwitch = true;
            }
        }
        if (c == Calibrator::Change::Locked) {
            // The locked fit is where the GPS trace was over the calibration
            // laps; from here a slow offset is tracked against it.
            mDriftOn = true;
            mDrift   = Vec2();
            mDA[0] = mDA[1] = mDA[2] = 0.0f;
            mDB[0] = mDB[1] = 0.0f;
            mDW = mDN = 0.0f;
        }
        return;
    }
    case Calibrator::Change::Rejected:
        if (mSupplied) {
            if (mAlign.done) {
                confirmStored(3); // nothing to check it against
            }
            return; // the visit cannot be checked; the stored track stays in use
        }
        addEvent(ev, EventType::CalibrationRejected, tMs);
        mModel.clear();
        mWantSwitch  = false;
        mLine        = false;
        mOff         = false;
        mLeft        = false;
        mHaveLateral = false;
        mOffLanesM   = 0.0f;
        setMode(Mode::NotStandard, tMs, ev);
        return;
    }
}

// ---------------------------------------------------------------------------
// Calibrated mode
// ---------------------------------------------------------------------------

float Tracker::relOf(float tau) const noexcept
{
    const float len = lapLen();
    float r = std::fmod(mModel.sOf(tau, lineOffset()) - startS(), len);
    if (r < 0.0f) {
        r += len;
    }
    return r;
}

float Tracker::remeasureLap(float tauNow) const noexcept
{
    // The current lap so far on the model, each stretch on the lane it was
    // run in: from the start line to each lane change, then to here.
    if (mMarkCount == 0) {
        return relOf(tauNow);
    }
    if (mMarkCount > kMaxLaneMarks) {
        return mCounter.lapProgressM(); // too many changes to follow: counting mode's figure
    }
    // A mark that projects a little behind where its stretch starts (GPS
    // noise at a change made standing on the line) would read as nearly a
    // whole lap: a stretch within kBehindM of a lap that takes the lap past
    // counting mode's own figure is that, and counts as none.
    constexpr float kBehindM = 25.0f;
    const float     counted  = mCounter.lapProgressM();
    float total = 0.0f;
    float from  = mTau0;
    for (uint8_t i = 0; i <= mMarkCount; ++i) {
        const uint8_t lane = i < mMarkCount ? mMarks[i].laneBefore : mLane;
        const float   off  = mTable.lineOffsetM(lane);
        const float   to   = i < mMarkCount ? mModel.project(mMarks[i].p, off).tau : tauNow;
        const float   len  = mModel.lapLengthM(off);
        float d = std::fmod(mModel.sOf(to, off) - mModel.sOf(from, off), len);
        if (d < 0.0f) {
            d += len;
        }
        if (d > len - kBehindM && total + d > counted + kBehindM) {
            d = 0.0f;
        }
        total += d;
        from = to;
    }
    return total;
}

float Tracker::nearest(float rel, float ref) const noexcept
{
    const float len = lapLen();
    return rel + std::floor((ref - rel) / len + 0.5f) * len;
}

float Tracker::recentSpeed(uint32_t tMs) const noexcept
{
    // Speed along the line over the last few seconds of readings, by a line
    // fit, so the filter starts at the runner's pace rather than from rest.
    const float off = lineOffset();
    float   st = 0.0f, sy = 0.0f, stt = 0.0f, sty = 0.0f, prev = 0.0f;
    uint8_t n = 0;
    for (uint8_t i = 0; i < mTiming.size(); ++i) {
        const LapTiming::Entry& e  = mTiming.at(i);
        const int32_t           dt = msDiff(e.tMs, tMs);
        if (!(e.flags & LapTiming::kPosition) || dt < -5000) {
            continue;
        }
        const float rel = relOf(mModel.project(e.p - mDrift, off).tau);
        const float y   = n == 0 ? rel : nearest(rel, prev);
        const float t   = 0.001f * static_cast<float>(dt);
        prev = y;
        st  += t;
        sy  += y;
        stt += t * t;
        sty += t * y;
        ++n;
    }
    if (n < 3) {
        return 0.0f;
    }
    const float fn  = static_cast<float>(n);
    const float sxx = stt - st * st / fn;
    if (!(sxx > 1e-3f)) {
        return 0.0f;
    }
    return clampf((sty - st * sy / fn) / sxx, 0.0f, mCfg.maxSpeedMps);
}

void Tracker::beginLine(float rel, uint32_t tMs) noexcept
{
    mLine       = true;
    mX          = rel;
    mV          = recentSpeed(tMs);
    mFilterT    = tMs;
    mUmax       = rel;
    mOutliers   = 0;
    mOff        = false;
    mLeft       = false;
    mOffRun     = 0;
    mOnRun      = 0;
    mNeedAnchor = false;
    mHaveSafe   = false;
    mHaveZ      = false;
    mLastCounterTotal = mCounter.totalDistanceM();
}

void Tracker::anchor(float rel, uint32_t tMs, float allowM, Events& ev, bool afterPause) noexcept
{
    // Back on the line after a pause or time off the track. Ground covered
    // meanwhile is not credited beyond allowM: only what the runner runs from
    // here. After time off the track a move within outlierM is taken as
    // noise.
    const float len = lapLen();
    if (afterPause) {
        // As in Run: what was run before the pause and what is run from the
        // resume both count, and the way between does not. The lap's credit
        // runs on from this reading, ahead of the mark or behind it, so its
        // noise is as likely to add a few metres as to take them off.
        mUmax    = rel;
        mAltered = true;
    } else if (rel > mUmax + mCfg.outlierM + allowM) {
        // Further on than allowM: allowM of it is credited (after a short
        // spell off, as far as running on at the speed before would have
        // gone), the rest is not. Crediting none of it would make a false
        // alarm that ran a little over cost the whole spell.
        mLapMetres += allowM;
        if (rel >= len) {
            // The start line was passed meanwhile: that lap ends here, with
            // what was run of it.
            completeLap(EventType::MissedLap, tMs, mLapMetres, ev);
            rel        -= len;
            mUmax      -= len;
            mLapMetres  = 0.0f;
        }
        mUmax    = rel > mUmax ? rel : mUmax;
        mAltered = true;
    }
    mX        = rel;
    mV        = recentSpeed(tMs);
    mFilterT  = tMs;
    mOutliers = 0;
    mHaveSafe = false;
    mHaveZ    = false;
}

void Tracker::rebaseModel(const TrackModel& next, Vec2 nextDrift) noexcept
{
    // A refined fit: keep the runner where they are on the track, now
    // measured on the new model. The lap's progress moves with them; a drop
    // is held on the display until it catches up. Positions are compared in
    // GPS coordinates, so the offset between GPS and each model is allowed for.
    const float off   = lineOffset();
    const float tauX  = mModel.tauOf(startS() + mX, off);
    const Vec2  here  = mModel.pointAt(tauX, off) + mDrift;
    const float revs  = std::floor(mX / lapLen());
    mModel = next;
    mDrift = nextDrift;
    mTau0  = mModel.project(startPointNow() - mDrift, off).tau;
    const float xNew  = relOf(mModel.project(here - mDrift, off).tau) + revs * lapLen();
    const float x     = nearest(xNew, mX);
    const float d     = x - mX;
    mX        = x;
    mUmax    += d;
    mLapMetres = mLapMetres + d > 0.0f ? mLapMetres + d : 0.0f;
    mHaveSafe  = false;
    mHaveZ     = false;
}

void Tracker::lineReading(Vec2 p, uint32_t tMs, Events& ev) noexcept
{
    const float off = lineOffset();
    const TrackModel::Projection pr = mModel.project(p - mDrift, off);
    mLateralM    = pr.lateralM;
    mHaveLateral = true;
    // Off the track is off the lanes, not off the runner's own line: a lane
    // set wrong (a stored track's last lane, lanes away) is still the track,
    // and its distance is the runner's to put right.
    const float inner = mTable.lineOffsetM(1) - off;
    const float outer = mTable.lineOffsetM(mTable.laneCount()) - off;
    mOffLanesM = pr.lateralM < inner ? inner - pr.lateralM : (pr.lateralM > outer ? pr.lateralM - outer : 0.0f);
    // A reading poorer than unknownHdop is judged by distances widened by
    // its relative error.
    const float widen = hdopWiden(mHdop, mCfg);
    // While a stored track is being lined up, its offset is still settling:
    // the off-track check waits (alignment drops a wrong pick on its own).
    // Leaving: further than offLanesM beyond the lanes. Coming back: within
    // three quarters of it, so nowhere is the runner both off and back on.
    const bool onTrack = mAlign.aligning
        || mOffLanesM <= (mOff ? 0.75f : 1.0f) * mCfg.offLanesM * widen;

    // The start line follows counting mode's start point until the runner
    // is under way (that point can still move in the first seconds), and on
    // a stored track until it has been lined up: the start point carries the
    // day's GPS offset like every reading.
    if (!mTau0Final) {
        const float tau0 = mModel.project(startPointNow() - mDrift, off).tau;
        if (mLine) {
            float d = mModel.sOf(tau0, off) - startS();
            const float len = lapLen();
            d -= std::floor(d / len + 0.5f) * len;
            mX    -= d;
            mUmax -= d;
            if (mArmedStart) {
                // Lap 1 is measured from the start line: it moves with it. No
                // lap has ended yet, so none moves; the display holds a drop.
                mLapMetres = mLapMetres - d > 0.0f ? mLapMetres - d : 0.0f;
                mUmax      = mUmax > 0.0f ? mUmax : 0.0f;
            } else {
                // Lap 1 is not under way: its high-water mark is the start
                // line wherever that now is. Moving it with the
                // line credited, or held back, the distance the line moved.
                mUmax = 0.0f;
            }
        }
        mTau0 = tau0;
        if (!mHaveT0) {
            mHaveT0   = true;
            mT0FirstS = mModel.sOf(tau0, off);
        }
        if (mAlign.supplied && !mAlign.aligning && mArmedStart) {
            mTau0Final = true;
            if (mAlign.done) {
                const float len = lapLen();
                float       m   = mModel.sOf(mTau0, off) - mT0FirstS;
                m -= std::floor(m / len + 0.5f) * len;
                mAlign.t0MoveM = m;
            }
        }
    }
    const float rel = relOf(pr.tau);
    const float counterTotal = mCounter.totalDistanceM();
    const float counterStep  = counterTotal - mLastCounterTotal;
    mLastCounterTotal = counterTotal;

    if (!mLine) {
        if (mSupplied) {
            // Calibrated from the first reading: laps count from where Start
            // was pressed, so the runner starts at 0 on the start line.
            const float len = lapLen();
            beginLine(rel > 0.5f * len ? rel - len : rel, tMs);
            mUmax = 0.0f;
            return;
        }
        if (mMode != Mode::Calibrating || !mWantSwitch || mPaused || !onTrack || !mCounter.hasGate()
            || mCounter.gateState() == LapCounter::GateState::Passed) {
            return;
        }
        // Switch mid-run, but only well away from the start line, so which
        // lap the runner is on is not in doubt. Credited laps stay; the
        // current lap is re-measured on the model from the start line.
        mTau0      = mModel.project(mCounter.startPoint(), off).tau;
        mTau0Final = true;
        const float r    = relOf(pr.tau);
        const float frac = r / lapLen();
        if (frac < mCfg.switchFromFrac || frac > mCfg.switchToFrac) {
            return;
        }
        // Counting mode may owe a lap: a spell of poor readings over the far
        // side of the track can leave its gate unarmed, so it ignores the next
        // pass of the line and credits that lap later by its missed-lap rules.
        // Re-measuring the lap from the start line now would lose
        // it, so the switch waits until counting mode has credited it.
        if (mCounter.lapFraction() - frac >= kSwitchOverdueFrac) {
            return;
        }
        beginLine(r, tMs);
        mLapMetres  = remeasureLap(pr.tau);
        mAltered    = mLaneChanged;
        mArmedStart = true;
        mWantSwitch = false;
        mSwitchT    = tMs;
        setMode(Mode::Calibrated, tMs, ev);
        return;
    }

    if (mPaused) {
        return; // nothing counts while paused; resume() restarts the line
    }

    // Off the track: nothing is credited. A far reading or two is GPS noise;
    // offTrackReadings in a row is the runner leaving. After offTrackMs it is
    // counting mode's turn.
    if (!onTrack) {
        mOnRun = 0;
        if (!mOff) {
            if (++mOffRun == 1) {
                mOffFirstT = tMs;
            }
            if (mOffRun < (mCfg.offTrackReadings > 0 ? mCfg.offTrackReadings : 1)) {
                return;
            }
            mOff       = true;
            mOffSinceT = tMs;
            // The last reading on the runner's line counts only if it is
            // recent: with a lane set that is not the one run, the runner is
            // never close to its line, and an old one would date the exit
            // laps back. Without it, the exit is the first reading off.
            if (mHaveSafe && msDiff(tMs, mSafeT) > static_cast<int32_t>(kSafeMaxAgeMs)) {
                mHaveSafe = false;
            }
            mOffV      = mV > 0.0f ? mV : 0.0f;
            // The filter's momentum kept crediting as the runner turned away:
            // go back to the last reading close to the line. The display
            // holds; only what is credited from here changes.
            if (mHaveSafe && mSafeUmax < mUmax && mUmax - mSafeUmax < 3.0f * mCfg.outlierM) {
                mLapMetres -= mUmax - mSafeUmax;
                mUmax       = mSafeUmax;
                if (mLapMetres < 0.0f) {
                    mLapMetres = 0.0f;
                }
            }
        }
        if (!mLeft && msDiff(tMs, mOffSinceT) >= static_cast<int32_t>(mCfg.offTrackMs)) {
            mLeft = true;
            setMode(Mode::Counting, tMs, ev);
        }
        return;
    }
    mOffRun = 0;
    if (mOff) {
        if (mOnRun == 0) {
            mRejoinRel = rel;
            mRejoinT   = tMs;
        }
        if (++mOnRun < (mCfg.rejoinReadings > 0 ? mCfg.rejoinReadings : 1)) {
            return;
        }
        // Back on the track: restart from the first reading back. After a
        // short spell (still calibrated) the way along the track is credited
        // as far as running on at the speed before would have gone, so a
        // false alarm costs nothing; after a real absence (counting mode)
        // none of it is.
        mOff   = false;
        mOnRun = 0;
        float allow = 0.0f;
        if (mLeft) {
            mLeft = false;
            setMode(Mode::Calibrated, tMs, ev);
        } else {
            const int32_t offMs = msDiff(mRejoinT, offTrackFromTMs());
            allow = mOffV * 0.001f * static_cast<float>(offMs > 0 ? offMs : 0);
        }
        // The filter restarts at the first reading back, so this one moves
        // it on by the time since.
        anchor(nearest(mRejoinRel, mUmax), mRejoinT, allow, ev);
        mNeedAnchor = false;
    } else if (mNeedAnchor) {
        mNeedAnchor = false;
        anchor(nearest(rel, mUmax), tMs, 0.0f, ev, true);
        return;
    }

    // Unwrap the reading next to the prediction. After a gap in the
    // readings, counting mode's distance says roughly how far the runner went.
    const float dt  = 0.001f * static_cast<float>(msDiff(tMs, mFilterT) > 0 ? msDiff(tMs, mFilterT) : 0);
    const bool  gap = msDiff(tMs, mFilterT) > static_cast<int32_t>(mCfg.maxCrossGapMs);
    const float pred = gap ? mX + counterStep : mX + mV * dt;
    const float z    = nearest(rel, pred);
    const float zPrev    = mLastZ;
    const bool  havePrev = mHaveZ;
    const float progress = advance(z, pred, tMs, gap, ev);
    // A reading close to the runner's own line whose raw position still
    // moved on along it marks where the runner still was running the track
    // (the way off, across the other lanes, is not).
    if (std::fabs(pr.lateralM) <= mCfg.offTrackM * widen / 3.0f && havePrev && z > zPrev) {
        mSafeUmax = mUmax;
        mSafeT    = tMs;
        mHaveSafe = true;
    }
    if (progress > 0.0f && mDriftOn) {
        trackDrift(pr.tau, pr.lateralM, progress);
    }

    // Running a stored track the other way round: the runner's own movement
    // (lightly smoothed GPS positions, so neither noise nor the offset still
    // being lined up can fake it) has gone alignDirM against the stored
    // direction along the track, early in lap 1.
    if (mAlign.aligning) {
        const Vec2 prev = mDirP;
        mDirP = mHaveDirP ? mDirP + (p - mDirP) * kDirSmooth : p;
        if (mHaveDirP && length(mDirP - prev) < mCfg.outlierM) {
            const Vec2 a = mModel.pointAt(pr.tau, off);
            Vec2       t = mModel.pointAt(pr.tau + 0.01f, off) - a;
            const float l = length(t);
            if (l > 1e-4f) {
                mDirAcc += dot(mDirP - prev, t * (1.0f / l));
            }
        }
        mHaveDirP = true;
        if (mUmax < mCfg.alignFlipMaxM && mDirAcc <= -mCfg.alignDirM) {
            reverseStored();
        }
    }
}

float Tracker::advance(float z, float pred, uint32_t tMs, bool gap, Events& ev) noexcept
{
    const int32_t  dtMs  = msDiff(tMs, mFilterT);
    const float    dt    = dtMs > 0 ? 0.001f * static_cast<float>(dtMs) : 0.001f;
    const uint32_t prevT = mFilterT;
    const float    xPrev = mX;

    // Alpha-beta filter, with a gate against one-off jumps along the line.
    const float r    = z - pred;
    const float gate = mCfg.outlierM * hdopWiden(mHdop, mCfg) + 0.5f * mCfg.maxSpeedMps * dt;
    if (std::fabs(r) > gate && !gap) {
        if (++mOutliers < 3) {
            return 0.0f;
        }
        mX = z; // three in a row agree: the filter was wrong
        mV = 0.0f;
    } else if (gap) {
        // After a gap in the readings (a dropout, or a spell of HDOP over
        // maxHdop) the runner went on at about the average speed over it.
        // Carrying that on credits the way covered at once, and a lap that
        // ended in the gap is interpolated across it, as counting mode does
        // (otherwise it would be credited a reading later and timed there).
        const float vGap = (z - xPrev) / dt;
        mX = z;
        mV = clampf(vGap, 0.0f, mCfg.maxSpeedMps);
    } else {
        mX = pred + mCfg.abAlpha * r;
        mV = clampf(mV + mCfg.abBeta / dt * r, -mCfg.maxSpeedMps, mCfg.maxSpeedMps);
    }
    mOutliers = 0;
    mFilterT  = tMs;
    mLastZ     = z;
    mHaveZ     = true;

    // The raw total of this reading for lap timing and rep endings:
    // credited distance plus the unsmoothed position, continuous through
    // lap ends.
    mRawValid = true;
    mRawTotal = static_cast<float>(mCreditedM) + mLapMetres + (z - mUmax);

    // Lap 1 with a supplied model counts once the runner is under way. A
    // runner already moving at the press is under way from the press.
    if (!mArmedStart) {
        if (mX < mCfg.gateResetM && !mCounter.rollingStart()) {
            return 0.0f;
        }
        mArmedStart = true;
        mTau0Final  = !mAlign.aligning;
        mFreezeStartP = !mHaveStartP;
    }
    // Counting mode's start point can still move after this (its check for a
    // stale start can mistake a runner moving off for a settling fix); from
    // here only the day's offset refines the start line. While counting mode
    // is still judging the start from the first readings, it is
    // followed until it has decided: a stale start point it corrects then
    // moves the line as above.
    if (mFreezeStartP && !mCounter.startPending()) {
        mFreezeStartP = false;
        mStartP       = mCounter.startPoint();
        mHaveStartP   = true;
    }

    // Distance is a high-water mark on the smoothed position: noise and
    // jogging back and forth add nothing. Standing still adds nothing either,
    // even when noise moves the position on.
    float progress = 0.0f;
    if (mX > mUmax && mV >= mCfg.stillMps) {
        progress    = mX - mUmax;
        mLapMetres += progress;
        mUmax       = mX;
    }

    // Laps: each time the high-water mark passes the start line.
    float x0 = xPrev;
    for (int guard = 0; guard < 4; ++guard) {
        const float len = lapLen();
        if (mUmax < len) {
            break;
        }
        const uint32_t since = activeAt(tMs) > mLapStartActive ? activeAt(tMs) - mLapStartActive : 0;
        if (!gap && since < mCfg.minLapMs) {
            break;
        }
        const float    f   = (mX > x0) ? (len - x0) / (mX - x0) : 1.0f;
        const uint32_t tc  = lerpTime(prevT, tMs, f);
        const float    lap = (mAltered || mLaneChanged) ? mLapMetres - (mUmax - len) : len;
        const uint32_t startActive = mLapStartActive;
        completeLap(gap ? EventType::MissedLap : EventType::Lap, tc, lap, ev);
        if (!gap) {
            scheduleRefine(true, tc, static_cast<float>(mCreditedM), startActive);
        }
        mX         -= len;
        mUmax      -= len;
        x0         -= len;
        mLapMetres  = mUmax;
        mAltered    = false;
        mHaveSafe   = false;
        mLastZ     -= len;
    }
    return progress;
}

void Tracker::trackDrift(float tau, float lateral, float progressM) noexcept
{
    // A translation between GPS and the model, from the lateral offsets
    // weighted over about the last lap. A shift of the runner's own line
    // (another lane) is the same all the way round and cancels out over a
    // lap; a translation does not. It moves at most driftPerLapM a lap.
    const float len    = lapLen();
    const float lambda = std::exp(-progressM / len);
    const Vec2  n      = mModel.normalAt(tau);
    const float la     = lateral + dot(n, mDrift);
    // Each reading weighted by its HDOP, relative to the mean
    // weight over the same span, so the prior below keeps its strength.
    const float hw = hdopWeight(mHdop, mCfg);
    mDW = lambda * mDW + hw;
    mDN = lambda * mDN + 1.0f;
    const float w = hw * (mDN / mDW);
    mDA[0] = lambda * mDA[0] + w * n.x * n.x;
    mDA[1] = lambda * mDA[1] + w * n.x * n.y;
    mDA[2] = lambda * mDA[2] + w * n.y * n.y;
    mDB[0] = lambda * mDB[0] + w * la * n.x;
    mDB[1] = lambda * mDB[1] + w * la * n.y;

    // Solve (A + eps I) t = b + eps d: with too few directions seen, the
    // estimate stays near the current offset.
    const float eps = 2.0f;
    const float a   = mDA[0] + eps, b = mDA[1], c = mDA[2] + eps;
    const float det = a * c - b * b;
    if (!(det > 1e-6f)) {
        return;
    }
    const float rx = mDB[0] + eps * mDrift.x;
    const float ry = mDB[1] + eps * mDrift.y;
    const Vec2  t{(c * rx - b * ry) / det, (a * ry - b * rx) / det};
    Vec2        step = t - mDrift;
    const float cap  = mCfg.driftPerLapM * progressM / len;
    const float l    = length(step);
    if (l > cap && l > 0.0f) {
        step = step * (cap / l);
    }
    mDrift = mDrift + step;
}

// ---------------------------------------------------------------------------
// Return visits: lining a stored track up with the day's GPS
// ---------------------------------------------------------------------------

Vec2 Tracker::shrinkOffset(Vec2 d, const float A[3], float scale, bool* unsure) const noexcept
{
    // The offset used is the estimate, direction by direction, only as far
    // as the points are sure of it: along each eigenvector of A (eigenvalue
    // l), the component c is used in full beyond alignSigHi standard errors
    // (scale / sqrt(l)), not at all within alignSigLo, and in proportion
    // between. A day's offset of several metres is used whole; a track
    // stored in the right place is not moved by noise; and along a straight
    // before the first bend, where nothing is known yet, nothing moves.
    const float a    = A[0] + 1.0f, b = A[1], c = A[2] + 1.0f;
    const float tr   = a + c;
    const float det  = a * c - b * b;
    const float disc = 0.25f * tr * tr - det;
    const float root = std::sqrt(disc > 0.0f ? disc : 0.0f);
    const float l1   = 0.5f * tr + root;
    const float l2   = 0.5f * tr - root;
    // Eigenvector of l1.
    Vec2 e1{b, l1 - a};
    if (length(e1) < 1e-6f) {
        e1 = (a >= c) ? Vec2{1.0f, 0.0f} : Vec2{0.0f, 1.0f};
    }
    e1 = e1 * (1.0f / length(e1));
    const Vec2 e2{-e1.y, e1.x};
    const float lo   = mCfg.alignSigLo;
    const float span = mCfg.alignSigHi > lo + 0.01f ? mCfg.alignSigHi - lo : 0.01f;
    bool between = false;
    auto used = [&](float comp, float l) {
        const float se = scale / std::sqrt(l > 1e-3f ? l : 1e-3f);
        const float f  = (std::fabs(comp) / (se > 1e-3f ? se : 1e-3f) - lo) / span;
        between = between || (f > 0.0f && f < 1.0f);
        return comp * (f < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f));
    };
    const Vec2 out = e1 * used(dot(d, e1), l1) + e2 * used(dot(d, e2), l2);
    if (unsure != nullptr) {
        *unsure = between;
    }
    return out;
}

bool Tracker::solveAlignment(const TrackModel& m, Vec2& d, float A[3], float& scale) const noexcept
{
    // A free 2D offset d between GPS and the model (the shape stays fixed),
    // from the calibrator's points, each against its own lane's line: Gauss-
    // Newton on the lateral offsets, with Huber weights. A weak prior holds d
    // where it is in a direction no point has faced yet (along a straight).
    const uint16_t n = mCal.pointCount();
    if (n < 3 || !m.isValid()) {
        return false;
    }
    const float huber = mCfg.alignHuberM > 0.1f ? mCfg.alignHuberM : 0.1f;
    const float prior = 1.0f;
    // Each point is also weighted by its HDOP, relative to the mean
    // weight, so the prior keeps its strength.
    float wSum = 0.0f;
    for (uint16_t i = 0; i < n; ++i) {
        wSum += mCal.pointWeight(i);
    }
    const float wNorm = static_cast<float>(n) / wSum;
    for (int it = 0; it < 6; ++it) {
        float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, b0 = 0.0f, b1 = 0.0f;
        float u0 = 0.0f, u1 = 0.0f, u2 = 0.0f;
        for (uint16_t i = 0; i < n; ++i) {
            const float off = mTable.lineOffsetM(mCal.pointLane(i));
            const TrackModel::Projection pr = m.project(mCal.point(i) - d, off);
            const Vec2  nn = m.normalAt(pr.tau);
            const float r  = pr.lateralM;
            const float w  = (std::fabs(r) <= huber ? 1.0f : huber / std::fabs(r)) * (mCal.pointWeight(i) * wNorm);
            a0 += w * nn.x * nn.x;
            a1 += w * nn.x * nn.y;
            a2 += w * nn.y * nn.y;
            b0 += w * nn.x * r;
            b1 += w * nn.y * r;
            u0 += nn.x * nn.x;
            u1 += nn.x * nn.y;
            u2 += nn.y * nn.y;
        }
        // Moving the model by step lowers each lateral offset by n . step.
        const float a = a0 + prior, b = a1, c = a2 + prior;
        const float det = a * c - b * b;
        if (!(det > 1e-6f)) {
            return false;
        }
        Vec2 step{(c * b0 - b * b1) / det, (a * b1 - b * b0) / det};
        const float sl = length(step);
        if (sl > kAlignMaxStepM) {
            step = step * (kAlignMaxStepM / sl); // small steps: no leaping to the far straight
        }
        d = d + step;
        A[0] = u0;
        A[1] = u1;
        A[2] = u2;
        if (!std::isfinite(d.x) || !std::isfinite(d.y)) {
            return false;
        }
        if (length(step) < 0.01f) {
            break;
        }
    }
    TrackModel shifted = m;
    shifted.set(m.centre() + d, m.headingRad(), m.kerbRadiusM(), m.clockwise());
    scale = mCal.scaleFor(shifted, 0.0f);
    return true;
}

bool Tracker::solveTurn(const TrackModel& m, Vec2 d, float& turnRad, float& seRad) const noexcept
{
    // The stored model, moved by d, is also turned about its centre
    // by turnRad to fit this run's points best (Gauss-Newton on the lateral
    // offsets, three unknowns, Huber and HDOP weights as solveAlignment). A
    // neighbour turned 15 to 50 degrees can be lined up by an offset alone
    // well enough to pass the checks above; turned as well, it fits,
    // and the turn shows it is another track. Its standard error comes from
    // the scatter of the points about the turned model.
    const uint16_t n = mCal.pointCount();
    if (n < 6 || !m.isValid()) {
        return false;
    }
    const float huber = mCfg.alignHuberM > 0.1f ? mCfg.alignHuberM : 0.1f;
    float wSum = 0.0f;
    for (uint16_t i = 0; i < n; ++i) {
        wSum += mCal.pointWeight(i);
    }
    const float wNorm = static_cast<float>(n) / wSum;
    float th = 0.0f;
    float N[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // a00 a01 a02 a11 a12 a22
    float rr = 0.0f, ww = 0.0f;
    for (int it = 0; it < kTurnIterations; ++it) {
        TrackModel mm = m;
        mm.set(m.centre() + d, m.headingRad() + th, m.kerbRadiusM(), m.clockwise());
        const Vec2 c = mm.centre();
        float b[3] = {0.0f, 0.0f, 0.0f};
        for (float& v : N) {
            v = 0.0f;
        }
        rr = ww = 0.0f;
        for (uint16_t i = 0; i < n; ++i) {
            const float off = mTable.lineOffsetM(mCal.pointLane(i));
            const TrackModel::Projection pr = mm.project(mCal.point(i), off);
            const Vec2  nn = mm.normalAt(pr.tau);
            const Vec2  x  = mm.pointAt(pr.tau, off) - c;
            const float r  = pr.lateralM;
            const float w  = (std::fabs(r) <= huber ? 1.0f : huber / std::fabs(r)) * (mCal.pointWeight(i) * wNorm);
            // Moving the model by (dx, dy) and turning it by dth lowers r by
            // n . (dx, dy) + dth n . perp(x).
            const float j[3] = {nn.x, nn.y, nn.x * -x.y + nn.y * x.x};
            N[0] += w * j[0] * j[0];
            N[1] += w * j[0] * j[1];
            N[2] += w * j[0] * j[2];
            N[3] += w * j[1] * j[1];
            N[4] += w * j[1] * j[2];
            N[5] += w * j[2] * j[2];
            b[0] += w * j[0] * r;
            b[1] += w * j[1] * r;
            b[2] += w * j[2] * r;
            rr += w * r * r;
            ww += w;
        }
        // The same weak hold on the offset as solveAlignment.
        float A[6] = {N[0] + 1.0f, N[1], N[2], N[3] + 1.0f, N[4], N[5] + 1e-3f};
        float step[3];
        if (!solve3(A, b, step)) {
            return false;
        }
        Vec2 sd{step[0], step[1]};
        const float sl = length(sd);
        if (sl > kAlignMaxStepM) {
            sd = sd * (kAlignMaxStepM / sl);
        }
        const float st = clampf(step[2], -kTurnMaxStepRad, kTurnMaxStepRad);
        d  = d + sd;
        th += st;
        if (!std::isfinite(d.x) || !std::isfinite(d.y) || !std::isfinite(th)) {
            return false;
        }
        if (sl < 0.01f && std::fabs(st) < 1e-4f) {
            break;
        }
    }
    // The variance of the turn: the scatter times the turn's part of the
    // inverse normal matrix (by its cofactor).
    const float det = N[0] * (N[3] * N[5] - N[4] * N[4]) - N[1] * (N[1] * N[5] - N[4] * N[2])
        + N[2] * (N[1] * N[4] - N[3] * N[2]);
    if (!(det > 1e-9f) || !(ww > 3.0f)) {
        return false;
    }
    const float s2 = rr / (ww - 3.0f);
    turnRad = th;
    seRad   = std::sqrt(s2 * (N[0] * N[3] - N[1] * N[1]) / det);
    return true;
}

float Tracker::alignScaleWiden() const noexcept
{
    // The robust scale of the points grows with their noise, which grows with
    // their HDOP: at an HDOP of 1.4 a right track fits 1.4 times as loosely.
    // The wrong-pick limit widens with the root mean square HDOP of the
    // points over kAlignScaleRefHdop, never narrows, and stays as it was for
    // points with no HDOP (without the widening, 29 right tracks in 20,000
    // sweep runs were dropped at 4 to 5 m noise, all with that HDOP at 1.09 to 1.50).
    float   sum = 0.0f;
    uint16_t n  = 0;
    for (uint16_t i = 0; i < mCal.pointCount(); ++i) {
        const float h = mCal.pointHdop(i);
        if (h > 0.0f) {
            sum += h * h;
            ++n;
        }
    }
    if (n == 0) {
        return 1.0f;
    }
    const float k = std::sqrt(sum / static_cast<float>(n)) / kAlignScaleRefHdop;
    return k > 1.0f ? k : 1.0f;
}

void Tracker::alignStep(uint32_t tMs, Events& ev) noexcept
{
    mAlignPts = mCal.pointCount();
    if (mAlignPts < kAlignMinPoints) {
        return;
    }
    // From where the last solve ended and from no offset: the better fit
    // wins, so one poor early solve cannot lead the next ones astray. Where a
    // straight meets a bend two offsets can fit about as well; a day's GPS
    // offset is more likely small, so then the smaller one wins.
    Vec2  d     = mAlign.offset;
    float A[3]  = {0.0f, 0.0f, 0.0f};
    float scale = 0.0f;
    const bool fromLast = solveAlignment(mModel, d, A, scale);
    Vec2  d0     = Vec2();
    float A0[3]  = {0.0f, 0.0f, 0.0f};
    float scale0 = 0.0f;
    const bool fromZero = solveAlignment(mModel, d0, A0, scale0);
    if (!fromLast && !fromZero) {
        return;
    }
    const bool similar = fromLast && fromZero && std::fabs(scale0 - scale) <= kAlignSimilarFit * std::fmin(scale0, scale);
    if (!fromLast || (fromZero && (similar ? length(d0) < length(d) : scale0 < scale))) {
        d     = d0;
        scale = scale0;
        A[0]  = A0[0];
        A[1]  = A0[1];
        A[2]  = A0[2];
    }
    bool unsure = false;
    mDrift = shrinkOffset(d, A, scale, &unsure);
    mAlignA[0] = A[0];
    mAlignA[1] = A[1];
    mAlignA[2] = A[2];
    mAlign.offset = d;
    mAlign.scaleM = scale;

    // Not this track: far further off than any day's GPS offset (counting
    // only what the points are sure of, the offset in use: along a straight
    // the estimate can wander far before a bend pins it), or a poor fit
    // whatever the offset.
    const float done = progressM();
    const float path = static_cast<float>(mAlignPts) * mCal.spacingM();
    if ((done >= mCfg.alignMinM || path >= mCfg.alignMaxM)
        && (length(mDrift) > mCfg.alignMaxOffsetM || scale > mCfg.alignMaxScaleM * alignScaleWiden())) {
        abandonStored(tMs, ev);
        return;
    }

    // Lined up once the points face enough directions (roughly the first
    // bend: the smaller eigenvalue of the sum of n n^T against its trace),
    // and each direction of the offset is either used whole or not at all;
    // in between, more points settle it. In any case by alignMaxM, before
    // lap 1 can end.
    const float tr   = A[0] + A[2];
    const float det  = A[0] * A[2] - A[1] * A[1];
    const float disc = 0.25f * tr * tr - det;
    const float lmin = 0.5f * tr - std::sqrt(disc > 0.0f ? disc : 0.0f);
    if ((done >= mCfg.alignMinM && lmin >= mCfg.alignCondFrac * tr && !unsure) || done >= mCfg.alignMaxM) {
        // Lined up by an offset, but would it fit better turned? A
        // neighbour turned 15 to 50 degrees can pass the checks above; turned
        // clearly, it is another track, dropped as a wrong pick before lap 1
        // can end, so no lap has been counted on it.
        if (turnTest(mAlign.offset)) {
            mAlign.turned = true;
            abandonStored(tMs, ev);
            return;
        }
        endAlignment(tMs);
    }
}

void Tracker::endAlignment(uint32_t tMs) noexcept
{
    mAlign.aligning = false;
    mAlign.done     = true;
    mAlign.endM     = progressM();
    mAlign.endTMs   = tMs;
    // Counting mode's laps stay held until the visit's first fit confirms the
    // track (confirmStored).
    // Drift tracking takes over from the offset found, seeded with its normal
    // equations so it starts there and then moves at most driftPerLapM a lap.
    mDriftOn = true;
    mDA[0] = mAlignA[0];
    mDA[1] = mAlignA[1];
    mDA[2] = mAlignA[2];
    mDB[0] = mDA[0] * mDrift.x + mDA[1] * mDrift.y;
    mDB[1] = mDA[1] * mDrift.x + mDA[2] * mDrift.y;
    mDW = mDN = 0.0f;
}

void Tracker::abandonStored(uint32_t tMs, Events& ev) noexcept
{
    // A wrong pick: the run carries on as on a new track. Counting mode has
    // measured all along; calibration goes on from the points it has, and
    // calibrated mode takes over mid-run as usual. The display holds.
    mAlign.aligning  = false;
    mAlign.abandoned = true;
    mAlign.endM      = progressM();
    mAlign.endTMs    = tMs;
    mSupplied   = false;
    mPlaced     = false;
    mModel.clear();
    mLine       = false;
    mOff        = false;
    mLeft       = false;
    mOffRun     = 0;
    mOnRun      = 0;
    mHaveLateral = false;
    mOffLanesM   = 0.0f;
    mDriftOn    = false;
    mDrift      = Vec2();
    mDA[0] = mDA[1] = mDA[2] = 0.0f;
    mDB[0] = mDB[1] = 0.0f;
    mDW = mDN = 0.0f;
    mTau0Final  = false;
    mArmedStart = false;
    mFreezeStartP = false;
    mHaveStartP = false;
    mX          = 0.0f;
    mV          = 0.0f;
    mUmax       = 0.0f;
    mLapMetres  = 0.0f;
    mAltered    = false;
    mNeedAnchor = false;
    // Laps counting mode saw while the wrong track was in use, beyond those
    // already credited, are credited now at their own times (on the
    // wrong geometry lining up can outlast lap 1, which would otherwise be lost).
    for (uint8_t k = 0; k < mHeldCount; ++k) {
        const LapCounter::Event& c = mHeld[k];
        if (c.lapNumber > mLaps) {
            completeLap(c.type == LapCounter::EventType::Lap ? EventType::Lap : EventType::MissedLap, c.tMs,
                        c.lapLengthM, ev);
        }
    }
    mHeldCount  = 0;
    mWantSwitch = false;
    if (mCal.hasModel()) {
        mModel = mCal.model();
        mModel.setGeo(mCounter.frame());
        mWantSwitch = true;
    }
    setMode(Mode::Calibrating, tMs, ev);
}

void Tracker::reverseStored() noexcept
{
    // Same track, run the other way: lap 1 starts again from the start line
    // in the new direction (the display holds).
    mModel = mModel.reversed();
    ++mAlign.flips;
    mDirAcc     = 0.0f;
    mLine       = false;
    mX          = 0.0f;
    mV          = 0.0f;
    mUmax       = 0.0f;
    mLapMetres  = 0.0f;
    mArmedStart = false;
    mFreezeStartP = false;
    mTau0Final  = false;
    mHaveT0     = false;
    mAltered    = false;
    mHaveSafe   = false;
    mHaveZ      = false;
}

void Tracker::checkVisit() noexcept
{
    // The visit's own locked fit against the stored track, once the day's
    // offset is removed: the offset that best lines the stored track up with
    // all of this run's points.
    const TrackModel& v = mCal.model();
    const Calibrator::FitInfo& f = mCal.lastFit();
    Vec2  d     = mDrift;
    float A[3]  = {0.0f, 0.0f, 0.0f};
    float scale = 0.0f;
    if (!v.isValid() || !mStored.isValid() || !solveAlignment(mStored, d, A, scale)) {
        return;
    }
    TrackModel s = mStored;
    s.set(mStored.centre() + d, mStored.headingRad(), mStored.kerbRadiusM(), mStored.clockwise());

    float dh = std::fmod(std::fabs(v.headingRad() - s.headingRad()), 3.14159265f);
    if (dh > 0.5f * 3.14159265f) {
        dh = 3.14159265f - dh;
    }
    mVisit.done       = true;
    mVisit.headingDeg = dh * 57.2957795f;
    mVisit.radiusM    = std::fabs(v.kerbRadiusM() - s.kerbRadiusM());
    mVisit.centreM    = length(v.centre() - s.centre());
    const float own   = f.scaleM > 0.3f ? f.scaleM : 0.3f;
    mVisit.scaleRatio = mCal.scaleFor(s, f.runnerM) / own;

    const bool agree = mVisit.headingDeg <= mCfg.visitAgreeHeadingDeg && mVisit.radiusM <= mCfg.visitAgreeRM
        && mVisit.centreM <= mCfg.visitAgreeCentreM && mVisit.scaleRatio <= mCfg.visitAgreeScaleRatio;
    const bool differ = mVisit.headingDeg > mCfg.visitReplaceHeadingDeg || mVisit.radiusM > mCfg.visitReplaceRM
        || mVisit.centreM > mCfg.visitReplaceCentreM || mVisit.scaleRatio > mCfg.visitReplaceScaleRatio;
    mVisit.kind = agree ? StoreOutcome::Kind::Updated
                        : (differ ? StoreOutcome::Kind::Replaced : StoreOutcome::Kind::Unchanged);

    if (differ && mLine) {
        // The stored track no longer fits (resurfaced, or a poor first
        // calibration): the rest of this run uses the visit's own fit, which
        // sits where today's GPS is, so drift tracking starts again from zero.
        TrackModel m = v;
        m.setGeo(mCounter.frame());
        rebaseModel(m, Vec2());
        mDA[0] = mDA[1] = mDA[2] = 0.0f;
        mDB[0] = mDB[1] = 0.0f;
        mDW = mDN = 0.0f;
        mDriftOn = true;
    }
}

void Tracker::turnStep(uint32_t tMs, Events& ev) noexcept
{
    // The turn test again, on the points so far. Lining up can end
    // after 100 m, a straight and part of a bend, where a turn and an offset
    // look alike; half a lap pins the turn to a degree or two.
    if (turnTest(mDrift)) {
        dropTurned(tMs, ev);
    }
}

bool Tracker::turnTest(Vec2 offset) noexcept
{
    // One turn test on the points so far, from the offset in use. A
    // clear turn (kTurnLimitDeg, kTurnSigma standard errors) counts only on
    // kTurnMinPathM of points or more, and drops the track only when two
    // tests in a row find it, the same way round: on a bend's worth of points
    // at 5 m noise one test found a right track turned 30 degrees.
    mTurnPts = mCal.pointCount();
    float turn = 0.0f, se = 0.0f;
    if (!solveTurn(mModel, offset, turn, se)) {
        mTurnRun = 0;
        return false;
    }
    mAlign.turnDeg   = turn * 57.2957795f;
    mAlign.turnSeDeg = se * 57.2957795f;
    const float path = static_cast<float>(mTurnPts) * mCal.spacingM();
    const bool  clear = path >= kTurnMinPathM && std::fabs(mAlign.turnDeg) >= kTurnLimitDeg
        && std::fabs(mAlign.turnDeg) >= kTurnSigma * mAlign.turnSeDeg;
    if (!clear) {
        mTurnRun = 0;
        return false;
    }
    const int8_t way = turn > 0.0f ? 1 : -1;
    mTurnRun = (mTurnRun * way > 0) ? static_cast<int8_t>(mTurnRun + way) : way;
    return mTurnRun * way >= kTurnRuns;
}

void Tracker::firstFitCheck(uint32_t tMs, Events& ev) noexcept
{
    // The visit's first fit against the stored track, lined up with
    // all of this run's points as at the visit check (checkVisit). Turned by
    // kTurnLimitDeg or more, it is another track; otherwise the stored track
    // is confirmed. A first fit's heading is within 1.7 degrees (95th
    // percentile, docs/OVAL.md); R and the centre are left to the visit check
    // at lock, where a changed track is replaced, not dropped.
    const TrackModel& v = mCal.model();
    Vec2  d     = mDrift;
    float A[3]  = {0.0f, 0.0f, 0.0f};
    float scale = 0.0f;
    if (!v.isValid() || !mStored.isValid() || !solveAlignment(mStored, d, A, scale)) {
        return; // the next fit, then
    }
    float dh = std::fmod(std::fabs(v.headingRad() - mStored.headingRad()), 3.14159265f);
    if (dh > 0.5f * 3.14159265f) {
        dh = 3.14159265f - dh;
    }
    if (dh * 57.2957795f >= kTurnLimitDeg) {
        mAlign.turnDeg = dh * 57.2957795f;
        dropTurned(tMs, ev);
    } else {
        confirmStored(1);
    }
}

void Tracker::confirmStored(uint8_t by) noexcept
{
    mAlign.confirmed   = true;
    mAlign.confirmedBy = by;
    mHeldCount       = 0; // the right track: its own laps count
}

void Tracker::dropTurned(uint32_t tMs, Events& ev) noexcept
{
    // The stored track is turned from this one: another track
    // beside it. Laps already shown were counted on it; counting mode, which
    // has measured from Start all along, saw the runner pass the start line
    // too. Each lap shown is re-timed to counting mode's (LapTimeRefined, as
    // a refined lap time is: the lap list and the last split follow it, no
    // popup), its laps beyond those are credited at their own times
    // (abandonStored), and the run carries on as on a new track. A lap shown
    // that counting mode never saw cannot be taken back: it is counted.
    mAlign.turned = true;
    mAlign.dropM  = progressM();
    uint8_t matched = 0;
    double  fix     = 0.0; // credit corrected so far
    for (uint8_t k = 0; k < mHeldCount; ++k) {
        const LapCounter::Event& c = mHeld[k];
        if (c.lapNumber > mLaps || c.lapNumber > kMaxHeldLaps) {
            continue;
        }
        ++matched;
        ++mAlign.retimed;
        // The wrong track's line can credit a lap short or long (a spell
        // off it credits only the metres run on it): the lap is counting
        // mode's length, and the totals from it on move with it.
        fix += static_cast<double>(c.lapLengthM) - static_cast<double>(mShownLen[c.lapNumber - 1]);
        if (c.lapNumber == mLaps) {
            mLapStartT      = c.tMs;
            mLapStartActive = activeAt(c.tMs);
            mLastLapMs      = c.lapDurationMs;
        }
        if (ev.count < kMaxEvents) {
            Event& e        = ev.items[ev.count++];
            e               = Event();
            e.type          = EventType::LapTimeRefined;
            e.lapNumber     = c.lapNumber;
            e.tMs           = c.tMs;
            e.lapDurationMs = c.lapDurationMs;
            e.lapLengthM    = c.lapLengthM; // set: the lap is credited again, at totalM
            e.totalM        = static_cast<float>(mShownTotal[c.lapNumber - 1] + fix);
            e.mode          = mMode;
        }
    }
    mCreditedM += fix;
    mAlign.unmatched = static_cast<uint8_t>(mLaps > matched ? mLaps - matched : 0);
    mRefine[0] = Refine(); // measured on the wrong track
    mRefine[1] = Refine();
    abandonStored(tMs, ev);
}

StoreOutcome Tracker::storeOutcome() const noexcept
{
    StoreOutcome o;
    const bool locked = mCal.stage() == Calibrator::Stage::Locked && mCal.hasModel();
    if (locked) {
        o.model = mCal.model();
        o.model.setGeo(mCounter.frame());
        o.quality.scaleM = mCal.lastFit().scaleM;
        o.quality.shapeM = mCal.lastFit().shapeM;
    }
    if (!mStoredGiven) {
        o.kind = locked ? StoreOutcome::Kind::New : StoreOutcome::Kind::None;
    } else if (mAlign.abandoned) {
        // A wrong pick. Its own fit replaces the picked track if the two are
        // close and run the same way (then it was this track after all,
        // badly stored), else it is a new track. Turned beyond
        // visitReplaceHeadingDeg, it is another track beside the picked one,
        // a warm-up track beside the main one, and both are kept.
        float dh = 0.0f;
        if (locked && mStored.isValid()) {
            dh = std::fmod(std::fabs(o.model.headingRad() - mStored.headingRad()), 3.14159265f);
            if (dh > 0.5f * 3.14159265f) {
                dh = 3.14159265f - dh;
            }
        }
        if (!locked) {
            o.kind = StoreOutcome::Kind::None;
        } else if (mStored.isValid() && length(o.model.centre() - mStored.centre()) <= mCfg.storeReplaceNearM
                   && dh * 57.2957795f <= mCfg.visitReplaceHeadingDeg) {
            o.kind = StoreOutcome::Kind::Replaced;
        } else {
            o.kind = StoreOutcome::Kind::New;
        }
    } else if (locked && mVisit.done) {
        o.kind = mVisit.kind;
    } else {
        o.kind = mAlign.done ? StoreOutcome::Kind::Unchanged : StoreOutcome::Kind::None;
    }
    if (o.kind == StoreOutcome::Kind::None || o.kind == StoreOutcome::Kind::Unchanged) {
        o.model.clear();
        o.quality = FitQuality();
    }
    return o;
}

// ---------------------------------------------------------------------------
// Lap time refinement
// ---------------------------------------------------------------------------

void Tracker::scheduleRefine(bool calibrated, uint32_t tMs, float target, uint32_t startActiveMs) noexcept
{
    Refine& r = mRefine[0].active ? mRefine[1] : mRefine[0];
    r.active        = true;
    r.calibrated    = calibrated;
    r.lapNumber     = mLaps;
    r.tMs           = tMs;
    r.target        = target;
    r.startActiveMs = startActiveMs;
}

void Tracker::checkRefines(uint32_t nowMs, bool final, Events& ev) noexcept
{
    for (Refine& r : mRefine) {
        if (!r.active) {
            continue;
        }
        if (!final && msDiff(nowMs, r.tMs) < static_cast<int32_t>(mCfg.refineAfterMs)) {
            continue;
        }
        r.active = false;
        uint32_t t  = 0;
        bool     ok = false;
        if (r.calibrated) {
            ok = mTiming.fitTotalCrossing(r.tMs, r.target, mCfg.refineBeforeMs, mCfg.refineAfterMs, t);
        } else if (mCounter.hasGate()) {
            ok = mTiming.fitGateCrossing(r.tMs, mCounter.startPoint(), mCounter.gateDirection(),
                                         mCfg.refineBeforeMs, mCfg.refineAfterMs, t);
        }
        if (!ok) {
            continue;
        }
        // The lap now ends at t, and the next one starts there.
        const uint32_t endActive = activeAt(t);
        const uint32_t dur = endActive > r.startActiveMs ? endActive - r.startActiveMs : 0;
        if (r.lapNumber == mLaps && !mPressed) {
            mLapStartT      = t;
            mLapStartActive = endActive;
            mLastLapMs      = dur;
        }
        if (ev.count < kMaxEvents) {
            Event& e        = ev.items[ev.count++];
            e               = Event();
            e.type          = EventType::LapTimeRefined;
            e.lapNumber     = r.lapNumber;
            e.tMs           = t;
            e.lapDurationMs = dur;
            e.totalM        = static_cast<float>(mCreditedM);
            e.mode          = mMode;
        }
    }
}

bool Tracker::timeOfDistance(float totalM, uint32_t& tMs) const noexcept
{
    return mTiming.timeOfTotal(totalM, mCfg.refineBeforeMs, mCfg.refineAfterMs, tMs);
}

bool Tracker::lanePosition(double& lat, double& lon) const noexcept
{
    // The reading projected onto the lane line, as lineReading()
    // projects it, and put back in GPS terms.
    if (!mRunning || !mModel.isValid() || !mHaveLastP || !mCounter.frame().hasOrigin()) {
        return false;
    }
    const float off = lineOffset();
    const TrackModel::Projection pr = mModel.project(mLastP - mDrift, off);
    mCounter.frame().toLatLon(mModel.pointAt(pr.tau, off) + mDrift, lat, lon);
    return true;
}

// ---------------------------------------------------------------------------
// Derived values
// ---------------------------------------------------------------------------

Tracker::CalStage Tracker::calibrationStage() const noexcept
{
    if (mSupplied) {
        return CalStage::Supplied;
    }
    switch (mCal.stage()) {
    case Calibrator::Stage::Provisional:
    case Calibrator::Stage::Refined:  return CalStage::Provisional;
    case Calibrator::Stage::Locked:   return CalStage::Locked;
    case Calibrator::Stage::Rejected: return CalStage::Rejected;
    default:                          return CalStage::None;
    }
}

bool Tracker::lockedModel(TrackModel& out) const noexcept
{
    if (mSupplied || mCal.stage() != Calibrator::Stage::Locked || !mModel.isValid()) {
        return false;
    }
    out = mModel;
    return true;
}

uint32_t Tracker::activeAt(uint32_t tMs) const noexcept
{
    const int32_t since = msDiff(tMs, mStartT);
    if (since <= 0) {
        return 0;
    }
    // Only the pauses, and parts of them, before tMs.
    const int64_t paused = mPauses.pausedBefore(mStartT, tMs);
    const int64_t a = static_cast<int64_t>(since) - paused;
    return a > 0 ? static_cast<uint32_t>(a) : 0;
}

uint32_t Tracker::lapElapsedMs(uint32_t nowMs) const noexcept
{
    if (!mRunning) {
        return 0;
    }
    const uint32_t a = activeAt(nowMs);
    return a > mLapStartActive ? a - mLapStartActive : 0;
}

} // namespace Oval
