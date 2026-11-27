/*******************************************************************************
 * @file   LapCounter.hpp
 * @brief  Oval: counting mode.
 *
 *         A virtual gate sits at the start point. Each forward crossing adds
 *         one lap of the runner's lane. Between crossings the part-lap is
 *         estimated from GPS distance, scaled by how far off GPS was on the
 *         last lap and capped just below a full lap. Whole laps are exact;
 *         only the part-lap is approximate, and that error is wiped out at
 *         every crossing.
 *
 *         A crossing only counts once the runner has been round: the gate
 *         line must first be crossed on the far side of the track, heading
 *         back. Standing on the line, jogging back and forth over it, or
 *         resting there between reps never counts a lap.
 *
 *         Lap times come from the readings' own timestamps, with the crossing
 *         interpolated between the two readings either side of the gate.
 *
 *         docs/OVAL.md describes the design, the guards and every parameter.
 *
 *         Usage, once a second:
 *           pre-run screen:  prime(sample)
 *           Start:           start(tMs, lane, table)
 *           each reading:    events = update(sample)
 *           pause menu:      pause(tMs), resume(tMs), setLane(lane)
 *           Stop:            events = finish()
 *
 *         No heap, no exceptions, no SDK headers. Not thread-safe.
 ******************************************************************************/

#ifndef OVAL_LAP_COUNTER_HPP
#define OVAL_LAP_COUNTER_HPP

#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/LocalFrame.hpp"
#include "Oval/Sample.hpp"

#include <cstdint>

namespace Oval {

class LapCounter {
public:
    /** Where the gate logic is. Exposed for logging and tests. */
    enum class GateState : uint8_t {
        Idle,        ///< Not started
        NeedFix,     ///< Started, waiting for a first good fix as the start point
        NeedGate,    ///< Start point known, gate not final yet
        Out,         ///< Just past the gate: waiting for the far side
        Armed,       ///< Crossed back over the gate line on the far side
        Passed,      ///< Passed the gate without a countable crossing: a lap is pending
    };

    enum class EventType : uint8_t {
        Lap,        ///< Lap completed on a real crossing
        MissedLap,  ///< Lap credited without a countable crossing
    };

    struct Event {
        EventType type          = EventType::Lap;
        uint16_t  lapNumber     = 0;    ///< 1-based
        uint32_t  tMs           = 0;    ///< When the lap ended, sensor clock (interpolated)
        uint32_t  lapDurationMs = 0;    ///< Active time of the lap (pauses excluded)
        float     lapLengthM    = 0.0f; ///< Distance credited for this lap
        float     totalM        = 0.0f; ///< Credited distance once this lap is added
    };

    static constexpr uint8_t kMaxEvents = 4;

    /** The events raised by one update() or finish(), oldest first. */
    struct Events {
        uint8_t count = 0;
        Event   items[kMaxEvents];
    };

    static constexpr uint8_t kPrimeSlots = 8;

    explicit LapCounter(const Config& cfg = Config()) noexcept;

    /** Replaces the configuration and resets everything. */
    void          setConfig(const Config& cfg) noexcept;
    const Config& config() const noexcept { return mCfg; }

    /** Back to Idle, primed fixes dropped. The configuration stays. */
    void reset() noexcept;

    /** Pre-run: keeps the last few good fixes so Start can use an averaged
     *  start point, weighted by 1 / hdopErrorM() squared. Ignored once
     *  started. */
    void prime(const Sample& s) noexcept;

    /** Pre-run: the averaged primed fixes from the last primeWindowMs before
     *  the latest one, as start() would average them (for recognising a
     *  stored track on the pre-run screen). False with none. */
    bool primedPosition(double& lat, double& lon) const noexcept;

    /** Start of the run. The averaged primed fixes from the last
     *  primeWindowMs become the start point (the gate centre and the local
     *  frame origin); with none, the first good fix after Start is used. If
     *  the first readings after Start sit well away from the primed point
     *  (a fix still settling), their mean replaces it. Returns false, and
     *  does not start, for an invalid lane. */
    bool start(uint32_t tMs, uint8_t lane, const LaneTable& table) noexcept;

    /** One reading. Readings that are not newer than the last are ignored. */
    Events update(const Sample& s) noexcept;

    /** End of the run (Stop): a pass of the gate still waiting to be
     *  credited (state Passed) is credited now. Counting stays running, so
     *  the getters still answer; call reset() or start() for a new run. */
    Events finish() noexcept;

    /** Pause and resume. tMs is on the sample clock; the latest reading's tMs
     *  is fine if the caller has nothing better. Readings keep arriving while
     *  paused: no laps count, and GPS distance covered while paused counts
     *  towards neither the guards nor the part-lap estimate. */
    void pause(uint32_t tMs) noexcept;
    void resume(uint32_t tMs) noexcept;

    /** Lane change, applying from this point. The current lap is credited as
     *  (progress fraction at the change x old lap length) + (the rest x new
     *  lap length). Returns false for an invalid lane. */
    bool setLane(uint8_t lane) noexcept;

    /** A Lap press: the lap starts again from where the runner
     *  is. The part-lap run so far is credited as distance (no lap), a pass
     *  still waiting is credited first, and the gate re-forms round the
     *  latest good position as at Start. Laps completed carry on. */
    Events restartLap(uint32_t tMs) noexcept;

    // Getters --------------------------------------------------------------

    bool      isRunning() const noexcept { return mRunning; }
    bool      isPaused() const noexcept { return mPaused; }
    GateState gateState() const noexcept { return mState; }
    uint8_t   lane() const noexcept { return mLane; }
    float     laneLengthM() const noexcept { return mLaneLenM; }

    /** Credited laps plus the part-lap estimate. Never decreases. */
    float totalDistanceM() const noexcept;

    /** Sum of the credited laps only. */
    float creditedDistanceM() const noexcept { return static_cast<float>(mCreditedM); }

    /** The part-lap estimate, metres into the current lap. */
    float lapProgressM() const noexcept;

    /** How far into the current lap the scaled GPS distance says the runner
     *  is, as a fraction of the lap, without the part-lap cap: over 1 when a
     *  pass of the line has not been seen. */
    float lapFraction() const noexcept { return fracAt(mOdoM); }

    /** Laps completed, including credited missed laps. */
    uint16_t lapsCompleted() const noexcept { return mLaps; }
    uint16_t missedLapsCredited() const noexcept { return mMissedLaps; }

    /** Active time into the current lap, at the latest reading or at nowMs. */
    uint32_t lapElapsedMs() const noexcept { return lapElapsedMs(mLastT); }
    uint32_t lapElapsedMs(uint32_t nowMs) const noexcept;

    /** Active duration of the last completed lap, 0 before the first. */
    uint32_t lastLapDurationMs() const noexcept { return mLastLapMs; }

    /** True metres per GPS metre, from the last lap that ended on a real
     *  crossing (1.0 until then). */
    float gpsRatio() const noexcept { return mRatio; }

    /** The local frame (origin = the start point as first set), the start
     *  point in it, and the gate, for logging. */
    const LocalFrame& frame() const noexcept { return mFrame; }
    Vec2              startPoint() const noexcept { return mCentre; }
    bool              hasGate() const noexcept { return mGateSet; }

    /** The runner was moving when Start was pressed (the primed
     *  fixes moved steadily), so the start point is where they were at the
     *  press, not the primed mean. */
    bool rollingStart() const noexcept { return mRolling; }
    /** The start point is still being judged from the first readings
     *  after Start (the check for a stale start ended before it had three
     *  readings near each other), and may still move. */
    bool startPending() const noexcept { return mStartPending; }
    Vec2              gateDirection() const noexcept { return mDir; }

private:
    struct Prime {
        uint32_t tMs;
        double   lat;
        double   lon;
        float    weight;
    };

    /** A good reading in the local frame, with what the gate needs. */
    struct Reading {
        bool     valid  = false;
        bool     paused = false;
        uint32_t tMs    = 0;
        Vec2     p;
        float    odoM   = 0.0f;
    };

    /** A point in the run where a lap could end. */
    struct LapPoint {
        bool     valid    = false;
        uint32_t tMs      = 0;
        uint32_t activeMs = 0;
        float    odoM     = 0.0f;
    };

    void clearRun() noexcept;
    bool isGood(const Sample& s) const noexcept;
    bool primedMean(uint32_t tMs, double& lat, double& lon) const noexcept;
    void updateOdometer(const Sample& s, bool good, Vec2 p) noexcept;
    void checkStartPoint(Vec2 p) noexcept;
    void primedMotion(uint32_t tMs) noexcept;
    bool rollingAt(const Reading& r) noexcept;
    struct PressFit {
        Vec2  at;           ///< Where the runner was at the press
        Vec2  mean;         ///< The readings' weighted mean
        Vec2  v;            ///< Their velocity, m/s
        float speed = 0.0f; ///< m/s
        float sdAt  = 0.0f; ///< Standard error of at, per axis, m
        float sdV   = 0.0f; ///< Standard error of the velocity, per axis, m/s
        float gpsMps = -1.0f; ///< The GPS distance's speed over the readings (negative: none)
        bool  wild  = false; ///< Faster than the GPS distance allows: the line is noise
    };
    bool pressFromFirst(uint8_t n, PressFit& f) const noexcept;
    bool pressLine(uint8_t n, uint8_t skip, PressFit& f, float* worstR2, uint8_t* worstAt) const noexcept;
    void judgeFromFirst() noexcept;
    bool sideways(const PressFit& f, Vec2 a, Vec2 b, float& side) const noexcept;
    bool firstDone() const noexcept;
    float staleBarM() const noexcept;
    enum class Motion : uint8_t { Standing, Moving, Unsure };
    static Motion motionOf(const PressFit& f) noexcept;
    static Vec2   carryV(const PressFit& f) noexcept;
    bool  splitPress(Vec2 v, Vec2& a, Vec2& b) const noexcept;
    void updateGateDirection(const Reading& r) noexcept;
    void gateStep(const Reading& r) noexcept;
    void checkCrossings(const Reading& prev, const Reading& cur, Events& ev) noexcept;
    void onFarBackward(Events& ev) noexcept;
    void onForwardPass(const Reading& prev, const Reading& cur, float f, Vec2 x, Events& ev) noexcept;
    void checkMissedLaps(uint32_t prevT, float prevOdo, Events& ev) noexcept;
    void trackOneLapPoint(uint32_t prevT, float prevOdo) noexcept;

    float    fracAt(float odoM) const noexcept;
    float    estimateAt(float odoM) const noexcept;
    float    creditLengthM() const noexcept;
    uint32_t activeAt(uint32_t tMs) const noexcept;

    /** learnRatio: a Lap teaches the GPS ratio, unless its GPS distance is
     *  only what a hidden lap credited before it left over. */
    void completeLap(EventType type, const LapPoint& at, Events& ev, bool learnRatio = true) noexcept;
    void startLap(const LapPoint& at) noexcept;

    Config    mCfg;
    LaneTable mTable;

    // Priming ring buffer
    Prime   mPrimes[kPrimeSlots] = {};
    uint8_t mPrimeHead           = 0;
    uint8_t mPrimeCount          = 0;

    // Run and clock
    bool      mRunning   = false;
    bool      mHaveT     = false;
    uint32_t  mStartT    = 0;
    uint32_t  mLastT     = 0;
    GateState mState     = GateState::Idle;
    uint8_t   mLane      = 1;
    float     mLaneLenM  = 400.0f;

    // Pause bookkeeping (all on the sample clock)
    bool     mPaused         = false;
    bool     mHadPause       = false;
    uint32_t mPauseStartT    = 0;
    uint32_t mPausedTotalMs  = 0; ///< Completed pauses only
    uint32_t mLastPauseT     = 0; ///< Start of the last completed pause
    uint32_t mLastPauseMs    = 0; ///< Its length
    uint32_t mResumeT        = 0;

    // Positions. The frame origin is where the start point was first put.
    LocalFrame mFrame;
    Reading    mPrev;
    Vec2       mCentre;              ///< The start point (moved by checkStartPoint() if stale)
    bool       mStartChecked = false;
    Vec2       mNearSum;             ///< Readings near where the run began, for checkStartPoint()
    Vec2       mNearFirst;           ///< The mean of the first three
    uint8_t    mNearCount    = 0;

    // How the run started. The primed fixes' motion (a line fit,
    // in the local frame) at Start; the first good readings after it.
    // Judged at kJudgeAt readings; at kFirstReadings when those leave it
    // borderline.
    static constexpr uint8_t kJudgeAt       = 4;
    static constexpr uint8_t kFirstReadings = 6;
    uint8_t  mPrimedN      = 0;     ///< Primed fixes the start point averaged
    bool     mMoveCand     = false; ///< The primed fixes moved steadily
    Vec2     mPrimedV;              ///< Their velocity, m/s
    float    mLagS         = 0.0f;  ///< The press less their weighted mean time, s
    float    mPrimedSigma  = 0.0f;  ///< Their scatter about their line, per axis, m
    bool     mRolling      = false;
    bool     mStartPending = false; ///< Judging from the first readings
    bool     mForcePress   = false; ///< Take their press estimate whatever it is
    bool     mStaleMean    = false; ///< Stale for sure; the first readings say where to
    float    mCurW         = 1.0f;  ///< The reading being handled: 1 / its error squared
    uint8_t  mFirstCount   = 0;
    uint8_t  mJudgeAt      = kJudgeAt; ///< Readings to judge the start point at
    uint8_t  mPendFrom     = 0;     ///< The first reading the gate has not seen yet
    Reading  mFirst[kFirstReadings];
    float    mFirstW[kFirstReadings] = {};

    // Gate. The line comes from the excursion's first readings at
    // gateDirDistM; it becomes final, facing the right way, at the first
    // crossing of that line on the far side of the track.
    bool    mGateSet      = false;
    Vec2    mDir;
    uint8_t mDirRun       = 0;     ///< Consecutive readings at gateDirDistM or more
    Vec2    mDirSum;
    bool    mCandSet      = false;
    Vec2    mCandAxis;
    bool    mHaveBaseline = false; ///< Lap progress can be measured
    float   mEstFloor     = 0.0f;  ///< Lap 1 shown so far, kept when an excursion resets

    // GPS odometer: kernel distance deltas, or summed steps as a fallback.
    // Accumulates only while active.
    float    mOdoM         = 0.0f;
    bool     mHaveGpsDist  = false;
    float    mLastGpsDistM = 0.0f;
    uint32_t mLastGpsDistT = 0;   ///< When the kernel total last moved
    bool     mHaveStepPos  = false;
    Vec2     mStepPos;
    uint32_t mStepPosT     = 0;

    // Current lap. Progress is a fraction of a lap, piecewise across lane
    // changes: a segment starts at the lap start or at the last lane change.
    LapPoint mLapStart;
    float    mSegOdoM    = 0.0f;
    float    mSegFrac    = 0.0f; ///< Uncapped fraction at the segment start
    float    mSegFracCap = 0.0f; ///< The same, capped at partLapCap
    float    mSegMetres  = 0.0f; ///< Metres credited for the lap up to the segment start
    float    mRatio      = 1.0f;
    LapPoint mOneLap;            ///< Where the fraction first reached 1.0 this lap
    LapPoint mPending;           ///< A pass that could not be counted
    bool     mGapInLap   = false; ///< A gap in good readings longer than maxCrossGapMs this lap

    // Totals
    double   mCreditedM  = 0.0;  ///< double: exact to 0.01 m over a long run
    uint16_t mLaps       = 0;
    uint16_t mMissedLaps = 0;
    uint32_t mLastLapMs  = 0;
};

} // namespace Oval

#endif // OVAL_LAP_COUNTER_HPP
