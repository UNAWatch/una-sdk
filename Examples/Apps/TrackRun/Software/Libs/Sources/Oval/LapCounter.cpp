/*******************************************************************************
 * @file   LapCounter.cpp
 * @brief  Oval: counting mode. See LapCounter.hpp and docs/OVAL.md.
 ******************************************************************************/

#include "Oval/LapCounter.hpp"

#include <cmath>

namespace Oval {

namespace {

/** A rolling start. The primed fixes (all kept, over up to twice the
 *  prime window) move at least kRollMinMps and kRollSigma standard errors,
 *  and the first reading after Start lies at least kRollLeadFrac of the way
 *  ahead of the primed mean that steady running would have put it. A walker
 *  under kRollMinMps keeps the primed mean, a few metres behind the press. */
constexpr float kRollMinMps   = 1.5f;
constexpr float kRollSigma    = 3.0f;
constexpr float kRollLeadFrac = 0.5f;

/** The first three readings move "as the first readings do" when
 *  their velocities differ by at most this fraction of the readings' speed
 *  (a stale start whose readings stand, then run, are not on one line). */
constexpr float kRollAgree    = 0.5f;

/** The first readings show a runner moving when the GPS distance
 *  over them gives at least this speed, even if their line is unclear
 *  (motionOf). */
constexpr float kMoveMps      = 2.5f;

/** The line through the first readings has standard errors from
 *  their scatter about it, never under kPressScatterFloor (as a variance) of
 *  what their HDOP says. */
constexpr float kPressScatterFloor = 0.25f;

/** The first readings' line shows a runner moving when its speed is
 *  kChoiceSigma of its standard error (and kRollMinMps or more). */
constexpr float kChoiceSigma = 2.0f;

/** A reading this many standard errors (from the others' scatter)
 *  off the line through the others is left out of the press estimate. */
constexpr float   kPressOutlier = 3.5f;
constexpr uint8_t kNoSkip       = 255;

/** The two halves of the first readings, carried back to the press,
 *  agree within this (splitPress, judgeFromFirst). */
constexpr float kPressAgreeM = 6.0f;

/** A line through the first readings faster than kSpeedSlack times
 *  the GPS distance's speed over them, plus kSpeedPadMps, is noise. */
constexpr float kSpeedSlack  = 1.5f;
constexpr float kSpeedPadMps = 1.0f;

/** A start judged stale from the first readings must be this many
 *  times the GPS noise at Start out, as well as staleStartM (staleBarM). */
constexpr float kStaleNoise  = 6.0f;

/** A start point moved across onto the runner's line (sideways):
 *  at least kSideFrac of gateDirDistM and kSideNoise times the GPS noise at
 *  Start to one side, on a line kSideSigma standard errors clear. */
constexpr float kSideFrac    = 0.5f;
constexpr float kSideNoise   = 3.0f;
constexpr float kSideSigma   = 4.0f;

inline float clampf(float v, float lo, float hi) noexcept
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float minf(float a, float b) noexcept
{
    return a < b ? a : b;
}

inline float maxf(float a, float b) noexcept
{
    return a > b ? a : b;
}

/** t0 + f x (t1 - t0) on the wrapping clock, for t1 after t0 and f in [0, 1]. */
inline uint32_t lerpTime(uint32_t t0, uint32_t t1, float f) noexcept
{
    const int32_t dt = msDiff(t1, t0);
    if (dt <= 0) {
        return t0;
    }
    return t0 + static_cast<uint32_t>(static_cast<float>(dt) * clampf(f, 0.0f, 1.0f) + 0.5f);
}

} // namespace

LapCounter::LapCounter(const Config& cfg) noexcept
    : mCfg(cfg)
{
    reset();
}

void LapCounter::setConfig(const Config& cfg) noexcept
{
    mCfg = cfg;
    reset();
}

void LapCounter::reset() noexcept
{
    clearRun();
    mTable      = LaneTable();
    mLane       = 1;
    mLaneLenM   = mTable.lapLengthM(1);
    mPrimeHead  = 0;
    mPrimeCount = 0;
}

void LapCounter::clearRun() noexcept
{
    mRunning = false;
    mHaveT   = false;
    mStartT  = 0;
    mLastT   = 0;
    mState   = GateState::Idle;

    mPaused        = false;
    mHadPause      = false;
    mPauseStartT   = 0;
    mPausedTotalMs = 0;
    mLastPauseT    = 0;
    mLastPauseMs   = 0;
    mResumeT       = 0;

    mFrame.clear();
    mPrev         = Reading();
    mCentre       = Vec2();
    mStartChecked = false;
    mNearSum      = Vec2();
    mNearFirst    = Vec2();
    mNearCount    = 0;

    mPrimedN      = 0;
    mMoveCand     = false;
    mPrimedV      = Vec2();
    mLagS         = 0.0f;
    mPrimedSigma  = 0.0f;
    mRolling      = false;
    mStartPending = false;
    mForcePress   = false;
    mStaleMean    = false;
    mCurW         = 1.0f;
    mFirstCount   = 0;
    mJudgeAt      = kJudgeAt;
    mPendFrom     = 0;

    mGateSet      = false;
    mDir          = Vec2();
    mDirRun       = 0;
    mDirSum       = Vec2();
    mCandSet      = false;
    mCandAxis     = Vec2();
    mHaveBaseline = false;
    mEstFloor     = 0.0f;

    mOdoM         = 0.0f;
    mHaveGpsDist  = false;
    mLastGpsDistM = 0.0f;
    mLastGpsDistT = 0;
    mHaveStepPos  = false;
    mStepPos      = Vec2();
    mStepPosT     = 0;

    mLapStart   = LapPoint();
    mSegOdoM    = 0.0f;
    mSegFrac    = 0.0f;
    mSegFracCap = 0.0f;
    mSegMetres  = 0.0f;
    mRatio      = 1.0f;
    mOneLap     = LapPoint();
    mPending    = LapPoint();
    mGapInLap   = false;

    mCreditedM  = 0.0;
    mLaps       = 0;
    mMissedLaps = 0;
    mLastLapMs  = 0;
}

bool LapCounter::isGood(const Sample& s) const noexcept
{
    // Negative HDOP means unknown, which is accepted.
    return s.fix && std::isfinite(s.lat) && std::isfinite(s.lon)
        && !(s.hdop > mCfg.maxHdop);
}

// ---------------------------------------------------------------------------
// Pre-run and start
// ---------------------------------------------------------------------------

void LapCounter::prime(const Sample& s) noexcept
{
    if (mRunning || !isGood(s)) {
        return;
    }
    // Weighted by 1 / the estimated error squared.
    float err = hdopErrorM(s.hdop, mCfg);
    if (err < 1.0f) {
        err = 1.0f;
    }
    mPrimes[mPrimeHead] = {s.tMs, s.lat, s.lon, 1.0f / (err * err)};
    mPrimeHead = static_cast<uint8_t>((mPrimeHead + 1) % kPrimeSlots);
    if (mPrimeCount < kPrimeSlots) {
        ++mPrimeCount;
    }
}

bool LapCounter::start(uint32_t tMs, uint8_t lane, const LaneTable& table) noexcept
{
    if (!table.isValidLane(lane)) {
        return false;
    }
    clearRun();
    mTable    = table;
    mLane     = lane;
    mLaneLenM = table.lapLengthM(lane);
    mRunning  = true;
    mStartT   = tMs;
    mLastT    = tMs;
    mLapStart.valid = true;
    mLapStart.tMs   = tMs;

    double lat = 0.0, lon = 0.0;
    const bool primed = primedMean(tMs, lat, lon);
    if (primed) {
        mFrame.setOrigin(lat, lon);
        primedMotion(tMs);
    }
    mPrimeCount = 0;
    mPrimeHead  = 0;

    if (primed) {
        mState = GateState::NeedGate;
    } else {
        mState = GateState::NeedFix;
    }
    return true;
}

bool LapCounter::primedPosition(double& lat, double& lon) const noexcept
{
    if (mRunning || mPrimeCount == 0) {
        return false;
    }
    const uint8_t latest = static_cast<uint8_t>((mPrimeHead + kPrimeSlots - 1) % kPrimeSlots);
    return primedMean(mPrimes[latest].tMs, lat, lon);
}

bool LapCounter::primedMean(uint32_t tMs, double& lat, double& lon) const noexcept
{
    // Weighted mean of the recent primed fixes, relative to the first one so
    // the sums stay small. Longitude differences wrap at 180 degrees.
    double sumW = 0.0, sumLat = 0.0, sumLon = 0.0, lat0 = 0.0, lon0 = 0.0;
    for (uint8_t i = 0; i < mPrimeCount; ++i) {
        const Prime& p = mPrimes[i];
        const int32_t age = msDiff(tMs, p.tMs);
        if (age < 0 || static_cast<uint32_t>(age) > mCfg.primeWindowMs) {
            continue;
        }
        if (sumW == 0.0) {
            lat0 = p.lat;
            lon0 = p.lon;
        }
        double dLon = p.lon - lon0;
        if (dLon > 180.0) {
            dLon -= 360.0;
        } else if (dLon < -180.0) {
            dLon += 360.0;
        }
        sumW   += p.weight;
        sumLat += p.weight * (p.lat - lat0);
        sumLon += p.weight * dLon;
    }
    if (!(sumW > 0.0)) {
        return false;
    }
    lon = lon0 + sumLon / sumW;
    if (lon > 180.0) {
        lon -= 360.0;
    } else if (lon < -180.0) {
        lon += 360.0;
    }
    lat = lat0 + sumLat / sumW;
    return true;
}

void LapCounter::primedMotion(uint32_t tMs) noexcept
{
    // How the primed fixes moved: a weighted line of position
    // against time through every fix kept from up to twice the prime window,
    // in the new local frame, times in seconds from the press: its velocity
    // and whether that is clear of its standard error (mMoveCand).
    // Also how many fixes the start point averaged, and how long before the
    // press their weighted mean time was.
    double W = 0.0, St = 0.0, Wn = 0.0, Stn = 0.0;
    Vec2   Sp;
    uint8_t n = 0;
    for (uint8_t i = 0; i < mPrimeCount; ++i) {
        const Prime&  f   = mPrimes[i];
        const int32_t age = msDiff(tMs, f.tMs);
        if (age < 0 || static_cast<uint32_t>(age) > 2u * mCfg.primeWindowMs) {
            continue;
        }
        const double t = -0.001 * static_cast<double>(age);
        W  += f.weight;
        St += f.weight * t;
        Sp  = Sp + mFrame.toLocal(f.lat, f.lon) * f.weight;
        ++n;
        if (static_cast<uint32_t>(age) <= mCfg.primeWindowMs) {
            Wn  += f.weight;
            Stn += f.weight * t;
            ++mPrimedN;
        }
    }
    if (n < 3 || !(W > 0.0)) {
        return;
    }
    const double tBar = St / W;
    const Vec2   pBar = Sp * static_cast<float>(1.0 / W);
    double Stt = 0.0;
    Vec2   Stp;
    for (uint8_t i = 0; i < mPrimeCount; ++i) {
        const Prime&  f   = mPrimes[i];
        const int32_t age = msDiff(tMs, f.tMs);
        if (age < 0 || static_cast<uint32_t>(age) > 2u * mCfg.primeWindowMs) {
            continue;
        }
        const double dt = -0.001 * static_cast<double>(age) - tBar;
        Stt += f.weight * dt * dt;
        Stp  = Stp + (mFrame.toLocal(f.lat, f.lon) - pBar) * static_cast<float>(f.weight * dt);
    }
    if (!(Stt > 1e-9)) {
        return;
    }
    mPrimedV     = Stp * static_cast<float>(1.0 / Stt);
    mLagS        = Wn > 0.0 ? static_cast<float>(-Stn / Wn) : 0.0f;
    const float speed = length(mPrimedV);
    const float se    = static_cast<float>(1.0 / std::sqrt(Stt));
    mMoveCand = speed >= kRollMinMps && speed >= kRollSigma * se;
    // The GPS noise at Start, per axis: the fixes' scatter about their line.
    // A stale start is judged against it (judgeFromFirst): at 5 m noise two
    // readings can sit 15 m out together; at 2 to 3 m they cannot.
    double r2 = 0.0;
    for (uint8_t i = 0; i < mPrimeCount; ++i) {
        const Prime&  p   = mPrimes[i];
        const int32_t age = msDiff(tMs, p.tMs);
        if (age < 0 || static_cast<uint32_t>(age) > 2u * mCfg.primeWindowMs) {
            continue;
        }
        const double dt = -0.001 * static_cast<double>(age) - tBar;
        const Vec2   e  = mFrame.toLocal(p.lat, p.lon) - (pBar + mPrimedV * static_cast<float>(dt));
        r2 += static_cast<double>(dot(e, e));
    }
    mPrimedSigma = n > 2 ? static_cast<float>(std::sqrt(r2 / (2.0 * (n - 2)))) : 0.0f;
}

LapCounter::Events LapCounter::finish() noexcept
{
    Events ev;
    if (mRunning && mState == GateState::Passed) {
        completeLap(EventType::MissedLap, mPending, ev);
        mState = GateState::Armed;
    }
    return ev;
}

// ---------------------------------------------------------------------------
// Pause, resume, lane
// ---------------------------------------------------------------------------

void LapCounter::pause(uint32_t tMs) noexcept
{
    if (!mRunning || mPaused) {
        return;
    }
    mPaused      = true;
    mPauseStartT = tMs;
}

void LapCounter::resume(uint32_t tMs) noexcept
{
    if (!mRunning || !mPaused) {
        return;
    }
    int32_t len = msDiff(tMs, mPauseStartT);
    if (len < 0) {
        len = 0;
    }
    mPausedTotalMs += static_cast<uint32_t>(len);
    mLastPauseT  = mPauseStartT;
    mLastPauseMs = static_cast<uint32_t>(len);
    mResumeT     = tMs;
    mHadPause    = true;
    mPaused      = false;
}

bool LapCounter::setLane(uint8_t lane) noexcept
{
    if (!mTable.isValidLane(lane)) {
        return false;
    }
    if (mRunning && mHaveBaseline) {
        // Close the segment: what is run so far stays credited at the old
        // lane's length, the rest of the lap is at the new lane's length.
        // What was run of the lap so far: not the display's floor (lap 1
        // shown before an excursion reset it), which was not run.
        const float f  = fracAt(mOdoM);
        const float fc = minf(f, mCfg.partLapCap);
        mSegMetres  = fc > mSegFracCap ? mSegMetres + (fc - mSegFracCap) * mLaneLenM : mSegMetres;
        mSegFrac    = f;
        mSegFracCap = fc;
        mSegOdoM    = mOdoM;
    }
    mLane     = lane;
    mLaneLenM = mTable.lapLengthM(lane);
    return true;
}

LapCounter::Events LapCounter::restartLap(uint32_t tMs) noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    if (mState == GateState::Passed) {
        completeLap(EventType::MissedLap, mPending, ev);
    }
    // The part-lap run so far stays credited, as distance.
    mCreditedM += static_cast<double>(estimateAt(mOdoM));

    LapPoint at;
    at.valid    = true;
    at.tMs      = msDiff(tMs, mLastT) > 0 || !mHaveT ? tMs : mLastT;
    at.activeMs = activeAt(at.tMs);
    at.odoM     = mOdoM;
    startLap(at);

    // The gate starts again round where the runner is, as at Start.
    if (mPrev.valid) {
        mCentre = mPrev.p;
    }
    mStartChecked = true;
    mStartPending = false;
    mForcePress   = false;
    mStaleMean    = false;
    mGateSet      = false;
    mDir          = Vec2();
    mDirRun       = 0;
    mDirSum       = Vec2();
    mCandSet      = false;
    mCandAxis     = Vec2();
    mHaveBaseline = false;
    mState        = mFrame.hasOrigin() ? GateState::NeedGate : GateState::NeedFix;
    return ev;
}

// ---------------------------------------------------------------------------
// Per reading
// ---------------------------------------------------------------------------

LapCounter::Events LapCounter::update(const Sample& s) noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    if (mHaveT && msDiff(s.tMs, mLastT) <= 0) {
        return ev;
    }
    const uint32_t prevT   = mHaveT ? mLastT : mStartT;
    const float    prevOdo = mOdoM;
    mHaveT = true;
    mLastT = s.tMs;

    const bool good = isGood(s);
    if (good && !mFrame.hasOrigin()) {
        mFrame.setOrigin(s.lat, s.lon);
        mState = GateState::NeedGate;
    }
    const Vec2 p = good ? mFrame.toLocal(s.lat, s.lon) : Vec2();

    updateOdometer(s, good, p);

    if (good) {
        Reading cur;
        cur.valid  = true;
        cur.paused = mPaused;
        cur.tMs    = s.tMs;
        cur.p      = p;
        cur.odoM   = mOdoM;
        if (mPrev.valid && msDiff(cur.tMs, mPrev.tMs) > static_cast<int32_t>(mCfg.maxCrossGapMs)) {
            mGapInLap = true;
        }
        if (!mGateSet) {
            float err = hdopErrorM(s.hdop, mCfg);
            if (err < 1.0f) {
                err = 1.0f;
            }
            mCurW = 1.0f / (err * err);
            updateGateDirection(cur);
        } else if (mPrev.valid) {
            checkCrossings(mPrev, cur, ev);
        }
        mPrev = cur;
    }

    if (mGateSet && !mPaused) {
        trackOneLapPoint(prevT, prevOdo);
        checkMissedLaps(prevT, prevOdo, ev);
    }
    return ev;
}

namespace {

/** A step of distance, cut to maxSpeed over the time it took. */
inline float limitStep(float d, int32_t dtMs, float maxSpeedMps) noexcept
{
    if (!(d > 0.0f) || dtMs <= 0) {
        return 0.0f;
    }
    const float maxStep = maxSpeedMps * static_cast<float>(dtMs) * 0.001f;
    return d < maxStep ? d : maxStep;
}

} // namespace

void LapCounter::updateOdometer(const Sample& s, bool good, Vec2 p) noexcept
{
    float delta = 0.0f;
    bool  have  = false;

    // The kernel's distance is a running total: use its differences. It stays
    // flat through a dropout and then jumps, so the speed limit runs from the
    // last time it moved. A NaN drops the reference, so a total that grew
    // meanwhile is not counted twice when it comes back.
    if (std::isfinite(s.gpsDistM)) {
        if (mHaveGpsDist) {
            const float d = s.gpsDistM - mLastGpsDistM;
            if (d != 0.0f) {
                delta = limitStep(d, msDiff(s.tMs, mLastGpsDistT), mCfg.maxSpeedMps);
                mLastGpsDistT = s.tMs;
            }
            have = true;
        } else {
            mLastGpsDistT = s.tMs;
        }
        mHaveGpsDist  = true;
        mLastGpsDistM = s.gpsDistM;
    } else {
        mHaveGpsDist = false;
    }

    // Fallback: the step between good positions in the local frame.
    if (good) {
        if (!have && mHaveStepPos) {
            delta = limitStep(length(p - mStepPos), msDiff(s.tMs, mStepPosT), mCfg.maxSpeedMps);
            have  = true;
        }
        mHaveStepPos = true;
        mStepPos     = p;
        mStepPosT    = s.tMs;
    }

    if (have && !mPaused) {
        mOdoM += delta;
    }
}

void LapCounter::checkStartPoint(Vec2 p) noexcept
{
    // While the runner is still where the readings after Start began, keep a
    // running mean of those readings. Well away from the primed start point,
    // the primed fixes were stale (a fix still settling): the start point
    // moves there. Once the runner leaves, or after enough readings, the
    // check is over. A runner jogging off slowly stays within reach of the
    // mean, which follows them and can drag the start point 15 m along.
    // So the check also ends once the mean has moved half of
    // gateResetM from where the first three readings put it: a settling fix
    // that has settled stays put.
    if (mNearCount == 0) {
        mNearSum   = p;
        mNearCount = 1;
        return;
    }
    const Vec2 mean = mNearSum * (1.0f / static_cast<float>(mNearCount));
    if (length(p - mean) > mCfg.gateResetM) {
        // Away before three readings near each other could say whether the
        // primed fixes were stale (a runner setting off at once, or
        // two noisy readings). With a real pre-run, the first readings judge
        // it instead (judgeFromFirst).
        if (mNearCount < 3 && mPrimedN >= 3) {
            mStartPending = true;
        } else {
            mStartChecked = true;
        }
        return;
    }
    mNearSum = mNearSum + p;
    ++mNearCount;
    const Vec2 m = mNearSum * (1.0f / static_cast<float>(mNearCount));
    if (mNearCount == 3) {
        mNearFirst = m;
    } else if (mNearCount > 3 && length(m - mNearFirst) > 0.5f * mCfg.gateResetM) {
        mStartChecked = true;
        return;
    }
    // Three readings already on the move put their mean about 1.5 s
    // of running past the press (7 m at 5 m/s), and a runner heading towards
    // a stale primed point keeps the mean near it. When the three seem to
    // move, the first readings judge it instead (judgeFromFirst): where a
    // stale start point moves to, and whether it was stale at all. Three are
    // too few to judge either at 4 to 5 m noise.
    PressFit   f;
    const bool fit3   = mNearCount == 3 && mPrimedN >= 3 && pressFromFirst(3, f);
    const bool moving = fit3 && motionOf(f) == Motion::Moving;
    if (mNearCount >= 3 && length(m - mCentre) > mCfg.staleStartM) {
        if (moving) {
            mStartPending = true;
            mStaleMean    = true;
            return;
        }
        mCentre       = m;
        mStartChecked = true;
    } else if (moving && length(f.at - mCentre) > staleBarM()) {
        mStartPending = true; // perhaps stale: the first readings judge it
    } else if (mNearCount >= 10) {
        mStartChecked = true;
    }
}

bool LapCounter::rollingAt(const Reading& r) noexcept
{
    // The first reading after Start, for a runner whose primed fixes moved.
    // Carried back to the press at their speed, it should lie
    // ahead of the primed mean by about what steady running gives. Then the
    // runner was under way at the press: the start point is where they were
    // then, at once the first reading carried back, and from the fourth
    // reading the press from the first four (judgeFromFirst). Both are in the
    // run's own GPS, so a jump at Start while rolling makes no difference;
    // the primed fixes only said that the runner was moving.
    const float speed = length(mPrimedV);
    const float dt    = 0.001f * static_cast<float>(msDiff(r.tMs, mStartT) > 0 ? msDiff(r.tMs, mStartT) : 0);
    const Vec2  q1    = r.p - mPrimedV * dt;
    const float lead  = dot(q1 - mCentre, mPrimedV * (1.0f / speed));
    if (lead < kRollLeadFrac * speed * mLagS) {
        return false; // not clearly moving: as a standing start
    }
    mCentre       = q1;
    mRolling      = true;
    mForcePress   = true;
    mStartPending = true;
    mPendFrom     = 0;
    return true;
}

bool LapCounter::pressFromFirst(uint8_t n, PressFit& f) const noexcept
{
    // The first n readings after Start, on a weighted straight line against
    // time, at the press: where the runner was then, whether standing or
    // already moving, in the GPS of the run itself. Also the line's speed,
    // and the standard errors of both, from how far the readings scatter
    // about the line (never under half of what their HDOP says): noisy
    // readings of a runner standing still can put a line 15 m out at a
    // "speed" of 6 m/s.
    // With five readings or more, one reading far off the line through the
    // others (beyond kPressOutlier times their scatter) is left out: a single
    // stray first reading moved the estimate 10 m.
    if (n > mFirstCount) {
        n = mFirstCount;
    }
    if (n < 2) {
        return false;
    }
    float worst = 0.0f;
    uint8_t worstAt = 0;
    if (!pressLine(n, kNoSkip, f, &worst, &worstAt)) {
        return false;
    }
    if (n >= 5) {
        PressFit g;
        float    rest = 0.0f;
        if (pressLine(n, worstAt, g, &rest, nullptr)
            && worst > kPressOutlier * kPressOutlier * (rest > kPressScatterFloor ? rest : kPressScatterFloor)) {
            f = g;
        }
    }
    // The GPS distance's speed over the same readings. A line faster than it
    // allows is noise, not running (three readings at 4 m noise gave 11 to
    // 14 m/s for runners at 4 to 6.6 m/s).
    const int32_t span = msDiff(mFirst[n - 1].tMs, mFirst[0].tMs);
    const float   odo  = mFirst[n - 1].odoM - mFirst[0].odoM;
    if (span > 0 && odo > 0.0f) {
        f.gpsMps = odo / (0.001f * static_cast<float>(span));
        f.wild   = f.speed > kSpeedSlack * f.gpsMps + kSpeedPadMps;
    }
    return true;
}

LapCounter::Motion LapCounter::motionOf(const PressFit& f) noexcept
{
    // What the first readings show. Moving if the line is clear of
    // its own noise (kRollMinMps, kChoiceSigma standard errors) or the GPS
    // distance says the runner covers kMoveMps or more, unless the line is
    // faster than the GPS distance allows (then it is noise, and nothing is
    // judged from it); otherwise standing.
    const bool line = f.speed >= kRollMinMps && f.speed >= kChoiceSigma * f.sdV;
    const bool gps  = f.gpsMps >= kMoveMps;
    if (!line && !gps) {
        return Motion::Standing;
    }
    return f.wild ? Motion::Unsure : Motion::Moving;
}

Vec2 LapCounter::carryV(const PressFit& f) noexcept
{
    // The velocity to carry readings back to the press with: the line's
    // direction at the GPS distance's speed, when there is one.
    if (!(f.speed > 0.0f)) {
        return Vec2();
    }
    return f.gpsMps > 0.0f ? f.v * (f.gpsMps / f.speed) : f.v;
}

bool LapCounter::pressLine(uint8_t n, uint8_t skip, PressFit& f, float* worstR2, uint8_t* worstAt) const noexcept
{
    // The weighted line through the first n readings bar `skip`. worstR2 gets
    // the largest weighted squared residual (or, with worstAt null, the mean
    // weighted squared residual per axis), worstAt that reading.
    float   W = 0.0f, St = 0.0f;
    Vec2    Sp;
    uint8_t m = 0;
    for (uint8_t i = 0; i < n; ++i) {
        if (i == skip) {
            continue;
        }
        const float t = 0.001f * static_cast<float>(msDiff(mFirst[i].tMs, mStartT));
        W  += mFirstW[i];
        St += mFirstW[i] * t;
        Sp  = Sp + mFirst[i].p * mFirstW[i];
        ++m;
    }
    if (m < 2) {
        return false;
    }
    const float tBar = St / W;
    const Vec2  pBar = Sp * (1.0f / W);
    float Stt = 0.0f;
    Vec2  Stp;
    for (uint8_t i = 0; i < n; ++i) {
        if (i == skip) {
            continue;
        }
        const float dt = 0.001f * static_cast<float>(msDiff(mFirst[i].tMs, mStartT)) - tBar;
        Stt += mFirstW[i] * dt * dt;
        Stp  = Stp + (mFirst[i].p - pBar) * (mFirstW[i] * dt);
    }
    if (!(Stt > 1e-6f)) {
        return false;
    }
    const Vec2 v = Stp * (1.0f / Stt);
    f.mean  = pBar;
    f.at    = pBar - v * tBar;
    f.v     = v;
    f.speed = length(v);
    // The scatter about the line, against what the HDOP weights expect (1).
    float c2 = 1.0f, r2 = 0.0f, big = 0.0f;
    for (uint8_t i = 0; i < n; ++i) {
        if (i == skip) {
            continue;
        }
        const float dt = 0.001f * static_cast<float>(msDiff(mFirst[i].tMs, mStartT)) - tBar;
        const Vec2  e  = mFirst[i].p - (pBar + v * dt);
        const float ri = mFirstW[i] * dot(e, e);
        r2 += ri;
        if (ri > big) {
            big = ri;
            if (worstAt != nullptr) {
                *worstAt = i;
            }
        }
    }
    if (m > 2) {
        c2 = r2 / (2.0f * static_cast<float>(m - 2));
        c2 = c2 > kPressScatterFloor ? c2 : kPressScatterFloor;
    }
    if (worstR2 != nullptr) {
        *worstR2 = worstAt != nullptr ? big : (m > 2 ? r2 / (2.0f * static_cast<float>(m - 2)) : 0.0f);
    }
    f.sdAt = std::sqrt(c2 * (1.0f / W + tBar * tBar / Stt));
    f.sdV  = std::sqrt(c2 / Stt);
    return true;
}

bool LapCounter::splitPress(Vec2 v, Vec2& a, Vec2& b) const noexcept
{
    // Where the first readings put the runner at the press, from
    // each half of them separately, each reading carried back to the press
    // at velocity v (carryV: a stray reading tilts the line and inflates its
    // speed, not the GPS distance; none for a runner standing). A jump moves
    // both halves alike; a stray reading moves one.
    const uint8_t n = mFirstCount;
    if (n < 4) {
        return false;
    }
    const uint8_t h = static_cast<uint8_t>(n / 2);
    Vec2 sa, sb;
    for (uint8_t i = 0; i < n; ++i) {
        const float t = 0.001f * static_cast<float>(msDiff(mFirst[i].tMs, mStartT));
        const Vec2  q = mFirst[i].p - v * t;
        if (i < h) {
            sa = sa + q;
        } else {
            sb = sb + q;
        }
    }
    a = sa * (1.0f / static_cast<float>(h));
    b = sb * (1.0f / static_cast<float>(n - h));
    return true;
}

float LapCounter::staleBarM() const noexcept
{
    // How far the first readings must put the runner at the press
    // from the primed point to call it stale when the three-reading check could not
    // judge it: staleStartM, or kStaleNoise times the GPS noise at Start if
    // more.
    const float n = kStaleNoise * mPrimedSigma;
    return n > mCfg.staleStartM ? n : mCfg.staleStartM;
}

bool LapCounter::sideways(const PressFit& f, Vec2 a, Vec2 b, float& side) const noexcept
{
    // The gate axis forms from readings gateDirDistM out, so a start
    // point well to one side of the runner's line turns the gate (a 20 m jump
    // read as 12 m along it turned the gate 50 degrees: seed set 3 of
    // StalePrimedStartPointIsReplaced). Across the line the press estimate
    // does not depend on when the runner set off. Both halves to the same
    // side by kSideFrac of gateDirDistM (30 degrees of turn) and kSideNoise
    // times the GPS noise at Start, agreeing within kPressAgreeM, on a line
    // whose direction is kSideSigma standard errors clear: noise alone, at 3
    // to 5 m, put right start points 10 to 16 m to the side of lines that
    // were clear but tilted, or unclear (Sweep and TrackerSweep).
    if (!(f.speed > 0.0f) || f.speed < kSideSigma * f.sdV) {
        return false;
    }
    const Vec2  u   = f.v * (1.0f / f.speed);
    const Vec2  n   = {-u.y, u.x};
    const float sa  = dot(a - mCentre, n);
    const float sb  = dot(b - mCentre, n);
    const float bar = maxf(kSideFrac * mCfg.gateDirDistM, kSideNoise * mPrimedSigma);
    if (!((sa > bar && sb > bar) || (sa < -bar && sb < -bar)) || std::fabs(sa - sb) > kPressAgreeM) {
        return false;
    }
    side = 0.5f * (sa + sb);
    return true;
}

bool LapCounter::firstDone() const noexcept
{
    return mFirstCount >= mJudgeAt;
}

void LapCounter::judgeFromFirst() noexcept
{
    // The start point is judged by where the first readings (four,
    // or six when four leave it borderline) put the runner at the press, each
    // half on its own (splitPress). Both halves far from the primed point,
    // and close to each other: the primed fixes were stale (a fix still
    // settling, the GPS track jumping 20 to 45 m), and the start point moves
    // there. Otherwise it stays: one stray reading at 4 to 5 m
    // noise can put a line's press 17 m away. The gate then sees the
    // readings it has not seen yet, exactly as it would have.
    // When three readings near each other were already well away (stale for
    // sure) but their motion was unclear, the first readings say where to:
    // the press if they clearly move along one line, else the mean of the
    // three (a runner who stood, then ran, is not on one line). A rolling
    // start takes the press whatever it is.
    PressFit f;
    if (pressFromFirst(kFirstReadings, f)) {
        const Motion mo = motionOf(f);
        Vec2         a, b;
        PressFit     f3;
        if (mStaleMean) {
            // On one line only if the first three move as the four do.
            const bool three = pressLine(3, kNoSkip, f3, nullptr, nullptr);
            const bool line  = mo == Motion::Moving && (!three || length(f.v - f3.v) <= kRollAgree * f.speed);
            if (line && splitPress(carryV(f), a, b)) {
                mCentre = (a + b) * 0.5f;
            } else if (three) {
                mCentre = f3.mean;
            }
        } else if (mForcePress) {
            // Rolling: the primed fixes' velocity, unless the readings show
            // the runner's own.
            // Halves that disagree are a bend the line cannot follow: the
            // first half, carried back least, is then the nearer.
            if (splitPress(mo == Motion::Moving ? carryV(f) : mPrimedV, a, b)) {
                mCentre = length(a - b) <= kPressAgreeM ? (a + b) * 0.5f : a;
            }
        } else if (mo != Motion::Unsure && splitPress(mo == Motion::Moving ? carryV(f) : Vec2(), a, b)) {
            const bool stale = length(a - mCentre) > staleBarM() && length(b - mCentre) > staleBarM()
                               && length(a - b) <= kPressAgreeM;
            float side = 0.0f;
            if (stale) {
                mCentre = (a + b) * 0.5f;
            } else if (mo == Motion::Moving && mJudgeAt == kFirstReadings && sideways(f, a, b, side)) {
                // Off to one side of the runner's line: the start point moves
                // across onto it, not along it (sideways).
                const Vec2 u = f.v * (1.0f / f.speed);
                mCentre      = mCentre + Vec2{-u.y, u.x} * side;
            } else if (mo == Motion::Moving && mJudgeAt < kFirstReadings
                       && (length(a - mCentre) > mCfg.gateResetM || length(b - mCentre) > mCfg.gateResetM)) {
                // Not stale, but a half lies beyond gateResetM: one stray
                // reading among four can hide a jump (seed set 3 of
                // StalePrimedStartPointIsReplaced: a 20 m jump read as 12 m,
                // the gate turned 50 degrees and three laps were missed).
                // Judged again at kFirstReadings, where a stray is left out
                // (pressFromFirst).
                mJudgeAt = kFirstReadings;
                return;
            }
        }
    }
    mStartPending = false;
    mForcePress   = false;
    mStaleMean    = false;
    mStartChecked = true;
    const Reading keep = mPrev;
    for (uint8_t i = mPendFrom; i < mFirstCount; ++i) {
        mPrev = i > 0 ? mFirst[i - 1] : Reading();
        gateStep(mFirst[i]);
    }
    mPrev = keep;
}

void LapCounter::updateGateDirection(const Reading& r) noexcept
{
    if (r.paused) {
        return;
    }
    if (!mStartChecked) {
        if (mFirstCount < kFirstReadings) {
            mFirst[mFirstCount]  = r;
            mFirstW[mFirstCount] = mCurW;
            ++mFirstCount;
        }
        if (!mStartPending && mFirstCount == 1 && mMoveCand && rollingAt(r)) {
            return;
        }
        if (!mStartPending) {
            checkStartPoint(r.p);
            if (mStartPending) {
                mPendFrom = static_cast<uint8_t>(mFirstCount - 1);
            }
        }
        if (mStartPending) {
            if (firstDone()) {
                judgeFromFirst();
            }
            return;
        }
        if (!mStartChecked) {
            return;
        }
    }
    gateStep(r);
}

void LapCounter::gateStep(const Reading& r) noexcept
{
    const Vec2  q = r.p - mCentre;
    const float d = length(q);

    // Back near the start point: this excursion is over. What it showed of
    // lap 1 so far is kept as a floor, so the total never goes back.
    if (d <= mCfg.gateResetM) {
        if (mHaveBaseline) {
            mEstFloor     = estimateAt(mOdoM);
            mHaveBaseline = false;
        }
        mDirRun  = 0;
        mDirSum  = Vec2();
        mCandSet = false;
        return;
    }

    if (!mCandSet) {
        if (d < mCfg.gateDirDistM) {
            mDirRun = 0;
            mDirSum = Vec2();
            return;
        }
        ++mDirRun;
        mDirSum = mDirSum + q;
        const uint8_t need = mCfg.gateDirConfirm > 0 ? mCfg.gateDirConfirm : 1;
        const Vec2    m    = mDirSum * (1.0f / static_cast<float>(mDirRun));
        const float   lm   = length(m);
        if (mDirRun < need || !(lm > 0.0f)) {
            return;
        }
        // The gate line: through the start point, across this direction.
        // Progress into lap 1 restarts here from the straight-line distance,
        // so GPS distance from standing or jogging about at the line before
        // setting off does not count.
        mCandAxis      = m * (1.0f / lm);
        mCandSet       = true;
        mHaveBaseline  = true;
        mLapStart.odoM = r.odoM - d / mRatio;
        mSegOdoM       = mLapStart.odoM;
        mSegFrac       = 0.0f;
        mSegFracCap    = 0.0f;
        mSegMetres     = 0.0f;
        return;
    }

    // The gate becomes final at the first crossing of its line on the far
    // side of the track. Going round, the runner crosses it there heading
    // back, which also fixes which way the gate faces. Jogging back and forth
    // along the track near the start, or a run-up from behind the line,
    // never crosses it far out.
    if (!mPrev.valid) {
        return;
    }
    const float sp = dot(mPrev.p - mCentre, mCandAxis);
    const float sc = dot(q, mCandAxis);
    if ((sp > 0.0f) == (sc > 0.0f)) {
        return;
    }
    const float f = sp / (sp - sc);
    const Vec2  x = mPrev.p + (r.p - mPrev.p) * f;
    if (length(x - mCentre) < mCfg.armFarM) {
        return;
    }
    mDir     = (sp > 0.0f) ? mCandAxis : mCandAxis * -1.0f;
    mGateSet = true;
    mState   = GateState::Armed;
}

void LapCounter::checkCrossings(const Reading& prev, const Reading& cur, Events& ev) noexcept
{
    const float sp = dot(prev.p - mCentre, mDir);
    const float sc = dot(cur.p - mCentre, mDir);

    if (sp > 0.0f && sc <= 0.0f) {
        // Backward over the gate line. Far from the start point it is the
        // far side of the track: the runner is on the way round.
        const float f = sp / (sp - sc);
        const Vec2  x = prev.p + (cur.p - prev.p) * f;
        if (length(x - mCentre) >= mCfg.armFarM) {
            onFarBackward(ev);
        }
    } else if (sp < 0.0f && sc >= 0.0f) {
        const float f = -sp / (sc - sp);
        const Vec2  x = prev.p + (cur.p - prev.p) * f;
        if (length(x - mCentre) <= mCfg.passRadiusM) {
            onForwardPass(prev, cur, f, x, ev);
        }
    }
}

void LapCounter::onFarBackward(Events& ev) noexcept
{
    if (mState == GateState::Out) {
        mState = GateState::Armed;
    } else if (mState == GateState::Passed && !mPaused) {
        // Round to the far side again since an uncounted pass: that pass was
        // a lap.
        completeLap(EventType::MissedLap, mPending, ev);
        mState = GateState::Armed;
    }
}

void LapCounter::onForwardPass(const Reading& prev, const Reading& cur, float f, Vec2 x,
                               Events& ev) noexcept
{
    if (mState != GateState::Armed && mState != GateState::Passed) {
        return;
    }
    LapPoint at;
    at.valid    = true;
    at.tMs      = lerpTime(prev.tMs, cur.tMs, f);
    at.odoM     = prev.odoM + (cur.odoM - prev.odoM) * f;
    at.activeMs = activeAt(at.tMs);

    // Guards: enough of a lap by scaled GPS distance, and enough time.
    const float    frac  = fracAt(at.odoM);
    const uint32_t since = at.activeMs > mLapStart.activeMs ? at.activeMs - mLapStart.activeMs : 0;
    if (frac < mCfg.minLapFrac || since < mCfg.minLapMs) {
        return;
    }

    // Counted now if the crossing point is close to the start point, the two
    // readings are close in time, and the run was not paused at the moment.
    // Otherwise the pass is held and credited a little later.
    const bool inPause = mPaused || (mHadPause && msDiff(at.tMs, mResumeT) < 0);
    const bool precise = !inPause
        && length(x - mCentre) <= mCfg.gateRadiusM
        && msDiff(cur.tMs, prev.tMs) <= static_cast<int32_t>(mCfg.maxCrossGapMs);

    // Already holding a pass: only a countable crossing soon after it (the
    // same passage of the line, not a lap later) replaces it, being the
    // better timed of the two.
    if (mState == GateState::Passed
        && (!precise || frac - fracAt(mPending.odoM) >= mCfg.minLapFrac)) {
        return;
    }

    if (precise) {
        // A lap 1.5 or more long by distance, with a gap in the readings:
        // the gap hid a pass. Credit that lap where the distance said one
        // lap, then count this crossing as the end of the next.
        const bool hidden = frac >= mCfg.missedLapFrac && mGapInLap && mOneLap.valid;
        if (hidden) {
            completeLap(EventType::MissedLap, mOneLap, ev);
        }
        completeLap(EventType::Lap, at, ev, !hidden);
        mState = GateState::Out;
    } else if (mState == GateState::Armed) {
        mPending = at;
        mState   = GateState::Passed;
    }
}

void LapCounter::trackOneLapPoint(uint32_t prevT, float prevOdo) noexcept
{
    if (mOneLap.valid) {
        return;
    }
    const float fNow = fracAt(mOdoM);
    if (fNow < 1.0f) {
        return;
    }
    const float fPrev = fracAt(prevOdo);
    const float k = (fNow > fPrev) ? clampf((1.0f - fPrev) / (fNow - fPrev), 0.0f, 1.0f) : 1.0f;
    mOneLap.valid    = true;
    mOneLap.tMs      = lerpTime(prevT, mLastT, k);
    mOneLap.odoM     = prevOdo + (mOdoM - prevOdo) * k;
    mOneLap.activeMs = activeAt(mOneLap.tMs);
}

void LapCounter::checkMissedLaps(uint32_t prevT, float prevOdo, Events& ev) noexcept
{
    while (ev.count < kMaxEvents) {
        const float frac = fracAt(mOdoM);
        // With good readings every few seconds a pass of the gate is always
        // seen, so a lap is credited by distance alone only after a gap, and
        // never for GPS distance piling up while standing about.
        const bool gap = mGapInLap || !mPrev.valid
            || msDiff(mLastT, mPrev.tMs) > static_cast<int32_t>(mCfg.maxCrossGapMs);
        if (mState == GateState::Passed && frac >= mCfg.missedLapFrac) {
            completeLap(EventType::MissedLap, mPending, ev);
        } else if (mState != GateState::Passed && frac >= mCfg.missedLapNoPassFrac && mOneLap.valid && gap) {
            completeLap(EventType::MissedLap, mOneLap, ev);
        } else {
            break;
        }
        // Keep the gate armed for the next real crossing.
        mState = GateState::Armed;
        trackOneLapPoint(prevT, prevOdo);
    }
}

// ---------------------------------------------------------------------------
// Laps
// ---------------------------------------------------------------------------

void LapCounter::completeLap(EventType type, const LapPoint& at, Events& ev, bool learnRatio) noexcept
{
    const float len = creditLengthM();

    // Only a lap ending on a real crossing teaches the GPS ratio.
    if (type == EventType::Lap && learnRatio) {
        const float gpsLapM = at.odoM - mLapStart.odoM;
        if (gpsLapM > 1.0f) {
            mRatio = clampf(len / gpsLapM, mCfg.ratioMin, mCfg.ratioMax);
        }
    }

    mCreditedM += static_cast<double>(len);
    ++mLaps;
    if (type == EventType::MissedLap) {
        ++mMissedLaps;
    }
    mLastLapMs = at.activeMs > mLapStart.activeMs ? at.activeMs - mLapStart.activeMs : 0;

    if (ev.count < kMaxEvents) {
        Event& e        = ev.items[ev.count++];
        e.type          = type;
        e.lapNumber     = mLaps;
        e.tMs           = at.tMs;
        e.lapDurationMs = mLastLapMs;
        e.lapLengthM    = len;
        e.totalM        = static_cast<float>(mCreditedM);
    }
    startLap(at);
}

void LapCounter::startLap(const LapPoint& at) noexcept
{
    mLapStart   = at;
    mSegOdoM    = at.odoM;
    mSegFrac    = 0.0f;
    mSegFracCap = 0.0f;
    mSegMetres  = 0.0f;
    mOneLap     = LapPoint();
    mPending    = LapPoint();
    mGapInLap   = false;
    mEstFloor   = 0.0f;
}

// ---------------------------------------------------------------------------
// Derived values
// ---------------------------------------------------------------------------

float LapCounter::fracAt(float odoM) const noexcept
{
    if (!mHaveBaseline || !(mLaneLenM > 0.0f)) {
        return 0.0f;
    }
    return mSegFrac + (odoM - mSegOdoM) * mRatio / mLaneLenM;
}

float LapCounter::estimateAt(float odoM) const noexcept
{
    if (!mHaveBaseline) {
        return mEstFloor;
    }
    const float fc = minf(fracAt(odoM), mCfg.partLapCap);
    const float e  = fc > mSegFracCap ? mSegMetres + (fc - mSegFracCap) * mLaneLenM : mSegMetres;
    return e > mEstFloor ? e : mEstFloor;
}

float LapCounter::creditLengthM() const noexcept
{
    return mSegMetres + (1.0f - mSegFracCap) * mLaneLenM;
}

uint32_t LapCounter::activeAt(uint32_t tMs) const noexcept
{
    const int32_t since = msDiff(tMs, mStartT);
    if (since <= 0) {
        return 0;
    }
    int64_t paused = mPausedTotalMs;
    if (mPaused) {
        const int32_t in = msDiff(tMs, mPauseStartT);
        if (in > 0) {
            paused += in;
        }
    } else if (mHadPause && msDiff(tMs, mResumeT) < 0) {
        // tMs is before the last resume: only the part of that pause before
        // tMs counts.
        paused -= mLastPauseMs;
        const int32_t into = msDiff(tMs, mLastPauseT);
        if (into > 0) {
            paused += (static_cast<uint32_t>(into) < mLastPauseMs) ? into : static_cast<int32_t>(mLastPauseMs);
        }
    }
    const int64_t a = static_cast<int64_t>(since) - paused;
    return a > 0 ? static_cast<uint32_t>(a) : 0;
}

float LapCounter::totalDistanceM() const noexcept
{
    return static_cast<float>(mCreditedM + static_cast<double>(estimateAt(mOdoM)));
}

float LapCounter::lapProgressM() const noexcept
{
    return estimateAt(mOdoM);
}

uint32_t LapCounter::lapElapsedMs(uint32_t nowMs) const noexcept
{
    if (!mRunning) {
        return 0;
    }
    const uint32_t a = activeAt(nowMs);
    return a > mLapStart.activeMs ? a - mLapStart.activeMs : 0;
}

} // namespace Oval
