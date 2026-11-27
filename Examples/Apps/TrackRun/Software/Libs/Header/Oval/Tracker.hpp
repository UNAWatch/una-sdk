/*******************************************************************************
 * @file   Tracker.hpp
 * @brief  Oval: measuring on one track. It wraps counting mode (LapCounter),
 *         calibration (Calibrator, TrackModel) and calibrated mode, and moves
 *         between them on its own. The service uses Oval::Session, which
 *         owns a Tracker for each visit to a track and counts the road
 *         between (Session.hpp).
 *
 *         Modes:
 *           Calibrating  counting mode measures while calibration collects
 *                        points; no accepted fit yet.
 *           Calibrated   every reading is placed on the runner's lane line of
 *                        the fitted (or supplied) track.
 *           NotStandard  the trace is not a standard 400 m track: counting
 *                        mode measures, and calibration has stopped.
 *           Counting     counting mode measures for another reason: after
 *                        leaving the track while calibrated.
 *
 *         Calibrated mode: the position along the lane line is smoothed by
 *         an alpha-beta filter; distance is a high-water mark on it, so it
 *         never goes back. A lap ends when it passes the start line again.
 *         Off the track (beyond the lanes by offLanesM) nothing is
 *         credited; after offTrackMs it drops to counting mode, and rejoins
 *         without crediting the detour (a Session ends the track segment
 *         there instead, and credits the way off as road). Once
 *         locked or supplied, a slowly varying offset between GPS and the
 *         model is tracked.
 *
 *         Lap times are reported at once (two-reading interpolation) and
 *         refined a few seconds later by a line fit (LapTimeRefined).
 *
 *         Stored tracks: a supplied model is first lined up with the
 *         day's GPS (a free 2D offset over roughly the first bend), which
 *         also refines the start line and checks the running direction;
 *         then drift tracking takes over. Calibration runs in the background
 *         on every outdoor run, and storeOutcome() says at the end what the
 *         run means for the store. The lane is the runner's: setLane().
 *
 *         Same calls as LapCounter:
 *           pre-run screen:  prime(sample)
 *           Start:           start(tMs, lane, table [, &storedModel])
 *           each reading:    events = update(sample)
 *           pause menu:      pause(tMs), resume(tMs), setLane(lane)
 *           Stop:            events = finish()
 *
 *         No heap, no exceptions, no SDK headers. Not thread-safe.
 ******************************************************************************/

#ifndef OVAL_TRACKER_HPP
#define OVAL_TRACKER_HPP

#include "Oval/Calibrator.hpp"
#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/LapCounter.hpp"
#include "Oval/LapTiming.hpp"
#include "Oval/LocalFrame.hpp"
#include "Oval/PauseLog.hpp"
#include "Oval/Sample.hpp"
#include "Oval/TrackModel.hpp"
#include "Oval/TrackStore.hpp"

#include <cstdint>

namespace Oval {

class Tracker {
public:
    enum class Mode : uint8_t { Counting, Calibrating, Calibrated, NotStandard };

    /** How far calibration has got. */
    enum class CalStage : uint8_t {
        None,        ///< No fit yet
        Provisional, ///< A fit in use, still being refined
        Locked,      ///< Final fit: model() can be stored
        Supplied,    ///< A stored model was given at Start
        Rejected,    ///< Not a standard track
    };

    enum class EventType : uint8_t {
        Lap,                    ///< Lap ended on a crossing of the start line
        MissedLap,              ///< Lap credited without a seen crossing
        LapTimeRefined,         ///< A better end time for lap lapNumber
        CalibrationProvisional, ///< First fit accepted
        CalibrationLocked,      ///< Fit locked
        CalibrationRejected,    ///< Not a standard track
        ModeChanged,            ///< mode is the new mode
    };

    struct Event {
        EventType type          = EventType::Lap;
        uint16_t  lapNumber     = 0;    ///< Laps: 1-based
        uint32_t  tMs           = 0;    ///< Laps: when it ended (sensor clock). Others: the reading
        uint32_t  lapDurationMs = 0;    ///< Laps: active time of the lap
        float     lapLengthM    = 0.0f; ///< Laps: distance credited
        float     totalM        = 0.0f; ///< Credited distance once this lap is added
        Mode      mode          = Mode::Counting;
    };

    /** How lining a stored track up with the day's GPS went. */
    struct Alignment {
        bool     supplied   = false; ///< A stored model was given at Start
        bool     aligning   = false; ///< Still lining up
        bool     done       = false; ///< Lined up; drift tracking has taken over
        bool     abandoned  = false; ///< Not this track (a wrong pick): calibrating from scratch
        uint8_t  flips      = 0;     ///< Times the running direction was reversed
        float    endM       = 0.0f;  ///< Progress along the track when it ended
        uint32_t endTMs     = 0;     ///< and when
        float    t0MoveM    = 0.0f;  ///< How far the start line moved along the track meanwhile
        float    scaleM     = 0.0f;  ///< Robust residual scale of the points against the aligned model
        Vec2     offset;             ///< The offset found
        // A stored track turned from this one is another track
        float    turnDeg    = 0.0f;  ///< The turn that lines the stored track up best, latest
        float    turnSeDeg  = 0.0f;  ///< and its standard error
        bool     turned     = false; ///< Dropped as turned: another track beside this one
        bool     confirmed  = false; ///< The visit's first fit agreed with it (or no fit could)
        uint8_t  confirmedBy = 0;    ///< 1 the first fit, 2 counting mode's laps with no fit, 3 a rejected fit,
                                     ///< 4 the run's own fit (confirmOwnFit)
        float    dropM      = 0.0f;  ///< Progress along the track when dropped as turned
        uint8_t  retimed    = 0;     ///< Laps already shown, re-timed to counting mode's on the drop
        uint8_t  unmatched  = 0;     ///< Laps shown that counting mode never saw (not taken back)
    };

    /** The stored track in use is still to be confirmed by the visit's
     *  first fit: counting mode's laps are kept meanwhile, so a turned
     *  neighbour found late can still be corrected. */
    bool storedUnconfirmed() const noexcept
    {
        return mAlign.supplied && !mAlign.abandoned && !mAlign.confirmed;
    }

    /** The supplied model is still being lined up, or is a stored track still
     *  to be confirmed: counting mode's laps are kept, and the Session's
     *  settle bound stays at the start. */
    bool suppliedUnsettled() const noexcept
    {
        return storedUnconfirmed() || (mAlign.supplied && !mAlign.done && !mAlign.abandoned);
    }

    /** The model supplied at start() is the run's own fit (a track not
     *  stored, visited earlier in the run), not a stored track: it cannot be
     *  a turned neighbour, so it needs no confirming. Call after start(). */
    void confirmOwnFit() noexcept
    {
        if (mAlign.supplied && !mAlign.abandoned) {
            mAlign.confirmed   = true;
            mAlign.confirmedBy = 4;
        }
    }

    /** The visit's locked fit against the stored track. */
    struct VisitCheck {
        bool               done       = false;
        float              headingDeg = 0.0f; ///< Heading difference
        float              radiusM    = 0.0f; ///< R difference
        float              centreM    = 0.0f; ///< Centre difference, the day's offset removed
        float              scaleRatio = 0.0f; ///< Stored track's scale on the visit's points / the visit's own
        StoreOutcome::Kind kind       = StoreOutcome::Kind::None; ///< Updated, Replaced or Unchanged
    };

    static constexpr uint8_t kMaxEvents = 8;

    struct Events {
        uint8_t count = 0;
        Event   items[kMaxEvents];
    };

    explicit Tracker(const Config& cfg = Config()) noexcept;

    void          setConfig(const Config& cfg) noexcept;
    const Config& config() const noexcept { return mCfg; }
    void          reset() noexcept;

    /** Road splits' length (a mile on an imperial
     *  watch), without a reset. Only the Session reads it. */
    void setRoadSplitM(float m) noexcept { mCfg.roadSplitM = m; }

    void prime(const Sample& s) noexcept { mCounter.prime(s); }

    /** Pre-run: the averaged primed position, to look up a stored track
     *  (TrackStore::findNear). False with no primed fixes. */
    bool primedPosition(double& lat, double& lon) const noexcept { return mCounter.primedPosition(lat, lon); }

    /** Start of the run. With a supplied model (a stored track) the
     *  run is calibrated from the first reading and laps count from where
     *  Start was pressed; it must have its latitude and longitude. It is
     *  lined up with the day's GPS over roughly the first bend. */
    bool start(uint32_t tMs, uint8_t lane, const LaneTable& table,
               const TrackModel* supplied = nullptr) noexcept;

    Events update(const Sample& s) noexcept;
    Events finish() noexcept;

    /** The stored track supplied is a wrong pick, as the caller judged from
     *  where the runner is: dropped as lining up drops one (alignment()
     *  says abandoned), if it is still being lined up. */
    Events dropStoredPick(uint32_t tMs) noexcept;

    void pause(uint32_t tMs) noexcept;
    void resume(uint32_t tMs) noexcept;

    /** The runner's lane change, from this point. */
    bool setLane(uint8_t lane) noexcept;

    /** A Lap press: the lap starts again from where the runner
     *  is, so the next lap ends a full lap from here (off the track with
     *  the line kept, the line stays and only the credited distance is
     *  settled). What was run of the lap so far stays credited as distance,
     *  with no lap. Laps completed carry on. Returns a lap still waiting to
     *  be credited, if any. */
    Events restartLap(uint32_t tMs) noexcept;

    /** What the run means for the stored tracks, for TrackStore::apply()
     *  with the number of the track supplied at Start. Call after finish(). */
    StoreOutcome storeOutcome() const noexcept;

    // State ------------------------------------------------------------------

    bool     isRunning() const noexcept { return mRunning; }
    bool     isPaused() const noexcept { return mPaused; }
    Mode     mode() const noexcept { return mMode; }
    CalStage calibrationStage() const noexcept;
    uint8_t  calibrationLaps() const noexcept { return mCal.lapsCovered(); }

    /** The model in use (provisional, locked or supplied). */
    bool              hasModel() const noexcept { return mModel.isValid(); }
    const TrackModel& model() const noexcept { return mModel; }

    /** The locked model, with its latitude, longitude and altitude, for
     *  the track store. False until locked. */
    bool lockedModel(TrackModel& out) const noexcept;

    /** Lateral offset of the latest reading from the lane line, positive
     *  outward (calibrated mode only). */
    bool  hasLateralOffset() const noexcept { return mHaveLateral; }
    float lateralOffsetM() const noexcept { return mLateralM; }

    /** How far the latest reading is outside the track's lanes, from lane
     *  1's line to the last lane's (0 within them), drift included
     *  (calibrated mode only): what the off-track check (offLanesM) and the
     *  Session's exit check measure. A runner on another lane than the one
     *  set is on the track. */
    float offLanesM() const noexcept { return mOffLanesM; }

    /** The latest good reading placed on the runner's lane line of the model
     *  in use (drift included), as latitude and longitude: the FIT record
     *  positions on a track. False without a model. */
    bool lanePosition(double& lat, double& lon) const noexcept;

    /** The tracked offset between GPS and the model (drift tracking). */
    Vec2 driftOffset() const noexcept { return mDrift; }

    /** Off the track (for Session): offTrackReadings readings in a
     *  row beyond the lanes by offLanesM, progress frozen. offTrackFromTMs()
     *  is when the runner was last on the track (the last reading within a
     *  third of offTrackM of their line that still moved along it, if it came
     *  within kSafeMaxAgeMs of going off; else the first reading off).
     *  hasLeft(): off for offTrackMs, counting mode has taken over. */
    bool     isOffTrack() const noexcept { return mOff; }
    uint32_t offTrackFromTMs() const noexcept { return mHaveSafe ? mSafeT : mOffFirstT; }
    bool     hasLeft() const noexcept { return mLeft; }

    /** The distance measured so far, without the display's hold: credited
     *  laps plus the measured part-lap (calibrated progress on the line, or
     *  counting mode's estimate). Off the track it stays where the runner
     *  left. */
    float measuredDistanceM() const noexcept;

    /** Lining up a stored track, and the visit check. */
    const Alignment&  alignment() const noexcept { return mAlign; }
    const VisitCheck& visitCheck() const noexcept { return mVisit; }

    // Distances and laps (as LapCounter) -------------------------------------

    /** Credited laps plus progress into the current lap. Never decreases. */
    float    totalDistanceM() const noexcept { return mShownM; }
    float    creditedDistanceM() const noexcept { return static_cast<float>(mCreditedM); }
    float    lapProgressM() const noexcept { return mShownM - static_cast<float>(mCreditedM); }
    uint16_t lapsCompleted() const noexcept { return mLaps; }
    uint16_t missedLapsCredited() const noexcept { return mMissed; }
    uint32_t lapElapsedMs() const noexcept { return lapElapsedMs(mLastT); }
    uint32_t lapElapsedMs(uint32_t nowMs) const noexcept;
    uint32_t lastLapDurationMs() const noexcept { return mLastLapMs; }
    uint8_t  lane() const noexcept { return mLane; }
    float    laneLengthM() const noexcept { return mTable.lapLengthM(mLane); }

    /** When the total reached totalM, on the sensor clock, from the last few
     *  seconds of readings (a line fit in calibrated mode). For rep endings
     *  mid-lap. False if not reached yet or too long ago. */
    bool timeOfDistance(float totalM, uint32_t& tMs) const noexcept;

    /** The largest amount the shown total has had to hold above the measured
     *  one (after switching to calibrated mode, rejoining or a refit). */
    float largestHoldM() const noexcept { return mLargestHoldM; }

    const LapCounter& counter() const noexcept { return mCounter; }
    const Calibrator& calibrator() const noexcept { return mCal; }

    /** How full the fixed buffers are now (for the buffer check). */
    uint8_t heldLapCount() const noexcept { return mHeldCount; }
    uint8_t laneMarkCount() const noexcept { return mMarkCount; }
    uint8_t lapTimingCount() const noexcept { return mTiming.size(); }

private:
    /** A lane change made before calibrated mode took over, for re-measuring
     *  the lap on the model at the switch. */
    struct LaneMark {
        Vec2    p;          ///< Where the runner was
        uint8_t laneBefore = 1;
    };
    static constexpr uint8_t kMaxLaneMarks = 3;

    /** A lap whose time will be refined once the readings after it are in. */
    struct Refine {
        bool     active     = false;
        bool     calibrated = false;
        uint16_t lapNumber  = 0;
        uint32_t tMs        = 0;    ///< Provisional end
        float    target     = 0.0f; ///< Calibrated: the raw total at the end
        uint32_t startActiveMs = 0; ///< Active time at the start of that lap
    };

    void clearRun() noexcept;
    bool isGood(const Sample& s) const noexcept;
    void addEvent(Events& ev, EventType type, uint32_t tMs) noexcept;
    void setMode(Mode m, uint32_t tMs, Events& ev) noexcept;
    uint32_t activeAt(uint32_t tMs) const noexcept;

    void completeLap(EventType type, uint32_t endMs, float lengthM, Events& ev) noexcept;
    void onCounterEvents(const LapCounter::Events& cev, uint32_t nowMs, Events& ev) noexcept;
    void onCalibration(Calibrator::Change c, uint32_t tMs, Events& ev) noexcept;

    // Calibrated mode
    float lineOffset() const noexcept { return mTable.lineOffsetM(mLane); }
    float lapLen() const noexcept { return mModel.lapLengthM(lineOffset()); }
    float startS() const noexcept { return mModel.sOf(mTau0, lineOffset()); }
    float relOf(float tau) const noexcept;   ///< Metres from the start line, [0, lap)
    float remeasureLap(float tauNow) const noexcept;
    float nearest(float rel, float ref) const noexcept; ///< rel plus whole laps, nearest ref
    void  beginLine(float rel, uint32_t tMs) noexcept;
    float recentSpeed(uint32_t tMs) const noexcept;
    void  anchor(float rel, uint32_t tMs, float allowM, Events& ev, bool afterPause = false) noexcept;
    void  rebaseModel(const TrackModel& next, Vec2 nextDrift) noexcept;
    void  lineReading(Vec2 p, uint32_t tMs, Events& ev) noexcept;
    float advance(float z, float pred, uint32_t tMs, bool gap, Events& ev) noexcept;
    void  trackDrift(float tau, float lateral, float progressM) noexcept;

    // Return visits and the visit check
    float progressM() const noexcept { return static_cast<float>(mCreditedM) + mLapMetres; }
    Vec2  startPointNow() const noexcept { return mHaveStartP ? mStartP : mCounter.startPoint(); }
    Vec2  shrinkOffset(Vec2 d, const float A[3], float scale, bool* unsure = nullptr) const noexcept;
    void  alignStep(uint32_t tMs, Events& ev) noexcept;
    bool  solveAlignment(const TrackModel& m, Vec2& d, float A[3], float& scale) const noexcept;
    bool  solveTurn(const TrackModel& m, Vec2 d, float& turnRad, float& seRad) const noexcept;
    float alignScaleWiden() const noexcept;
    void  endAlignment(uint32_t tMs) noexcept;
    void  abandonStored(uint32_t tMs, Events& ev) noexcept;
    void  reverseStored() noexcept;
    void  checkVisit() noexcept;
    void  turnStep(uint32_t tMs, Events& ev) noexcept;
    bool  turnTest(Vec2 offset) noexcept;
    void  firstFitCheck(uint32_t tMs, Events& ev) noexcept;
    void  confirmStored(uint8_t by) noexcept;
    void  dropTurned(uint32_t tMs, Events& ev) noexcept;

    void checkRefines(uint32_t nowMs, bool final, Events& ev) noexcept;
    void scheduleRefine(bool calibrated, uint32_t tMs, float target, uint32_t startActiveMs) noexcept;

    Config     mCfg;
    LaneTable  mTable;
    LapCounter mCounter;
    Calibrator mCal;
    TrackModel mModel;
    LapTiming  mTiming;

    // Run and clock
    bool     mRunning = false;
    bool     mHaveT   = false;
    bool     mSupplied = false;
    uint32_t mStartT  = 0;
    uint32_t mLastT   = 0;
    uint8_t  mLane    = 1;
    Mode     mMode    = Mode::Counting;

    // Pause bookkeeping, as LapCounter
    bool     mPaused        = false;
    bool     mHadPause      = false;
    PauseLog mPauses;                ///< Every recent pause, for times before more than one
    uint32_t mPauseStartT   = 0;
    uint32_t mPausedTotalMs = 0;
    uint32_t mLastPauseT    = 0;
    uint32_t mLastPauseMs   = 0;
    uint32_t mResumeT       = 0;

    // Credited laps
    double   mCreditedM     = 0.0;
    uint16_t mLaps          = 0;
    uint16_t mMissed        = 0;
    uint32_t mLapStartT     = 0;  ///< End of the last lap (refined when it is)
    uint32_t mLapStartActive = 0;
    uint32_t mLastLapMs     = 0;
    bool     mLaneChanged   = false; ///< A lane change in the current lap
    LaneMark mMarks[kMaxLaneMarks];
    uint8_t  mMarkCount     = 0;     ///< Lane changes this lap before the switch (beyond max: unknown)
    /** Counting mode's laps while a stored track is lined up: kept
     *  in case the track is dropped as a wrong pick, which on the wrong
     *  geometry can come after lap 1 has ended. */
    static constexpr uint8_t kMaxHeldLaps = 4;
    LapCounter::Event mHeld[kMaxHeldLaps];
    float    mShownLen[kMaxHeldLaps]   = {}; ///< Laps shown while the stored track was unconfirmed: length
    double   mShownTotal[kMaxHeldLaps] = {}; ///< and credited total
    uint8_t  mHeldCount     = 0;
    bool     mHaveLastP     = false;
    Vec2     mLastP;                 ///< Latest good position

    // Shown total
    float mShownM       = 0.0f;
    float mLargestHoldM = 0.0f;

    // Calibrated mode: positions are metres along the current lane's line
    // from the start line, unwrapped, so the current lap is 0 to lapLen().
    bool     mPlaced     = false; ///< A supplied model has been placed in this run's frame
    bool     mWantSwitch = false; ///< A fit is waiting for the safe moment to switch
    uint32_t mSwitchT    = 0;     ///< When calibrated mode last took over
    bool     mLine       = false; ///< Calibrated measuring is set up
    float    mTau0       = 0.0f;  ///< Start line station
    bool     mTau0Final  = false;
    bool     mArmedStart = false; ///< Lap 1 progress counts (the runner is under way)
    bool     mFreezeStartP = false; ///< The start point is to be frozen once counting mode has decided it
    float    mX          = 0.0f;  ///< Smoothed position
    float    mV          = 0.0f;  ///< Smoothed speed, m/s
    uint32_t mFilterT    = 0;
    float    mUmax       = 0.0f;  ///< High-water mark of mX
    float    mLapMetres  = 0.0f;  ///< Credited progress into the lap
    bool     mAltered    = false; ///< Lap not a plain lap: credit the metres, not the lane length
    uint8_t  mOutliers   = 0;
    bool     mNeedAnchor = false; ///< After a resume: restart without credit
    float    mLastCounterTotal = 0.0f;
    bool     mRawValid   = false; ///< This reading has a raw calibrated total
    float    mRawTotal   = 0.0f;

    // Off track
    bool     mOff        = false; ///< Off the track: nothing credited
    bool     mLeft       = false; ///< Off long enough to drop to counting mode
    uint8_t  mOffRun     = 0;     ///< Consecutive off-track readings
    uint8_t  mOnRun      = 0;     ///< Consecutive on-track readings while off
    uint32_t mOffSinceT  = 0;     ///< When offTrackReadings in a row confirmed it
    uint32_t mOffFirstT  = 0;     ///< The first of those readings
    float    mOffV       = 0.0f;  ///< Speed along the line when the runner went off
    float    mRejoinRel  = 0.0f;  ///< Position at the first on-track reading of a rejoin
    uint32_t mRejoinT    = 0;     ///< and its time
    float    mLastZ      = 0.0f;  ///< Raw position of the last reading used on the line
    bool     mHaveZ      = false;
    float    mSafeUmax   = 0.0f;  ///< Progress at the last reading close to the line, kept
    bool     mHaveSafe   = false; ///< to undo what the filter's momentum credits as the
                                  ///< runner turns off the track
    uint32_t mSafeT      = 0;     ///< and its time

    bool  mHaveLateral = false;
    float mLateralM    = 0.0f;
    float mOffLanesM   = 0.0f;
    /** The last reading on the runner's line (mSafeT) is where they left
     *  only if it came this soon before the reading that confirms them off
     *  (the offTrackReadings-th in a row). */
    static constexpr uint32_t kSafeMaxAgeMs = 20000;

    float mHdop = -1.0f; ///< HDOP of the reading being handled

    // Drift tracking: a translation between GPS and the model
    bool  mDriftOn = false;
    Vec2  mDrift;
    float mDA[3] = {0.0f, 0.0f, 0.0f}; ///< Normal equations: xx, xy, yy
    float mDB[2] = {0.0f, 0.0f};
    float mDW    = 0.0f; ///< The same decay over the readings' HDOP weights,
    float mDN    = 0.0f; ///< and over 1s: their ratio is the mean weight

    // Lap time refinement
    Refine mRefine[2];
    bool   mPressed = false; ///< A Lap press since the last lap: its start stays put

    // Return visits
    TrackModel mStored;          ///< The stored track as supplied, placed in this run's frame
    bool       mStoredGiven = false;
    Alignment  mAlign;
    float      mAlignA[3]  = {0.0f, 0.0f, 0.0f}; ///< Sum of n n^T of the last alignment
    uint16_t   mAlignPts   = 0;  ///< Calibrator points at the last alignment
    uint16_t   mTurnPts    = 0;  ///< Calibrator points at the last turn test
    int8_t     mTurnRun    = 0;  ///< Clear turn tests in a row, signed by the way round
    bool       mHaveStartP = false; ///< The start point as it was when the runner got under way
    Vec2       mStartP;
    bool       mHaveDirP   = false; ///< Running direction: smoothed position, and the way
    Vec2       mDirP;               ///< moved along the stored direction while lining up
    float      mDirAcc     = 0.0f;
    bool       mHaveT0     = false;
    float      mT0FirstS   = 0.0f; ///< Start line when first placed, metres along the lane line
    VisitCheck mVisit;
};

} // namespace Oval

#endif // OVAL_TRACKER_HPP
