/*******************************************************************************
 * @file   Session.cpp
 * @brief  Oval: road and track in one run. See Session.hpp and docs/OVAL.md.
 ******************************************************************************/

#include "Oval/Session.hpp"

#include <cmath>
#include <limits>

namespace Oval {

namespace {

/** The Session's frame moves to the runner once they are this far from its
 *  origin, so the ring's decimetres (int16) always fit. */
constexpr float kRecentreM = 2500.0f;

/** Points looked at to confirm a join of a known track, and further back to
 *  find where the runner came onto it. */
constexpr uint16_t kJoinScan   = 48;
constexpr uint16_t kJoinExtend = 64;

/** The join point is where the runner came onto their own line: readings
 *  within kJoinSpread times the stretch's robust noise of its median
 *  offset, and at least kJoinLineM. Too wide and a slanting approach is
 *  taken as track, losing its GPS distance; too narrow and noise ends the
 *  run early, moving track to road where GPS distance over-reads. */
constexpr float kJoinLineM  = 6.0f;
constexpr float kJoinSpread = 2.5f;

/** The band is this much wider inside the kerb than outside the track:
 *  paths beside a track run outside it. */
constexpr float kJoinInnerM = 4.0f;

/** Where the runner came onto the track is looked for over running only:
 *  ring points further apart than this in time (a pause, a GPS outage) end
 *  the search. */
constexpr int32_t kJoinGapMs = 20000;

/** A lane set on the road starts the visit at the latest good reading only
 *  if it is this recent; else at the next good reading. */
constexpr int32_t kGoodFreshMs = 3000;

/** The entry at a join (entryAt). The readings looked at: up to
 *  kEntryBack ring points before the join point and kEntryFwd after it,
 *  leaving out readings before it more than kEntryFarM off the runner's
 *  line (the road before the last leg). */
constexpr uint8_t kEntryBack = 6;
constexpr uint8_t kEntryFwd  = 10;
constexpr uint8_t kEntryPts  = kEntryBack + kEntryFwd + 1;
constexpr float   kEntryFarM = 30.0f;
/** The station at the entry: a line through at most kEntryOn readings on the
 *  line from the entry on, and at least kEntryMinOn. */
constexpr uint8_t kEntryOn    = 8;
constexpr uint8_t kEntryMinOn = 4;
/** A slanting approach is taken only if it comes in at kEntrySlopeMps or more
 *  across the line, and fits kEntrySigma2 times the noise (as a variance)
 *  better than "on one line, at one speed, throughout". */
constexpr float kEntrySlopeMps = 0.5f;
constexpr float kEntrySigma2   = 16.0f;
/** The approach's stations are fitted over its last leg only: the readings
 *  from this long before the join point (about 1.5 s before the entry, as
 *  the join point usually comes 2 to 3 s before it). */
constexpr float kEntryLegS = 4.0f;
/** The entry is looked for from 3 s before the join point to kEntryLateS
 *  after it: in the sweeps the true entry came at most about 5.6 s after
 *  the join point (95th percentile, 12 m back and 22 m out). Further on, a
 *  slow drift of the runner's line against a stored track can pass for the
 *  end of an approach at 4 m noise. */
constexpr float kEntryLateS = 6.0f;
/** Readings in a row beyond kRecentreM before the frame moves. */
constexpr uint8_t kRecentreReadings = 3;

inline int16_t toDm(float m) noexcept
{
    const float d = std::round(m * 10.0f);
    return static_cast<int16_t>(d > 32767.0f ? 32767.0f : (d < -32767.0f ? -32767.0f : d));
}

/** A step of distance, cut to maxSpeed over the time it took (as counting
 *  mode does). */
inline float limitStep(float d, int32_t dtMs, float maxSpeedMps) noexcept
{
    if (!(d > 0.0f) || dtMs <= 0) {
        return 0.0f;
    }
    const float maxStep = maxSpeedMps * static_cast<float>(dtMs) * 0.001f;
    return d < maxStep ? d : maxStep;
}

/** Ground distance between two positions, metres (local frame round the
 *  first). */
inline float distM(double lat1, double lon1, double lat2, double lon2) noexcept
{
    LocalFrame f;
    f.setOrigin(lat1, lon1);
    return length(f.toLocal(lat2, lon2));
}

inline bool isLapType(Tracker::EventType t) noexcept
{
    return t == Tracker::EventType::Lap || t == Tracker::EventType::MissedLap
        || t == Tracker::EventType::LapTimeRefined;
}

inline uint8_t outcomeRank(StoreOutcome::Kind k) noexcept
{
    switch (k) {
    case StoreOutcome::Kind::New:
    case StoreOutcome::Kind::Updated:
    case StoreOutcome::Kind::Replaced:  return 2;
    case StoreOutcome::Kind::Unchanged: return 1;
    default:                            return 0;
    }
}

} // namespace

Session::Session(const Config& cfg) noexcept
    : mTracker(cfg)
{
    reset();
}

void Session::setConfig(const Config& cfg) noexcept
{
    mTracker.setConfig(cfg);
    reset();
}

void Session::reset() noexcept
{
    mTracker.reset();
    mTable = LaneTable();
    clearRun();
}

void Session::clearRun() noexcept
{
    mStore = nullptr;
    mFrame.clear();
    mOdo = Odometer();

    mRunning  = false;
    mHaveT    = false;
    mPaused   = false;
    mFinished = false;
    mStartT   = 0;
    mLastT    = 0;
    mLane     = kNoLane;
    mLastLane = 1;
    mSeg      = Segment::None;
    mHaveGood = false;
    mGood     = Sample();

    mRoadBaseM    = 0.0;
    mTrackBaseM   = 0.0;
    mRoadSegOdo   = 0.0f;
    mLapsBase     = 0;
    mShownM       = 0.0f;
    mLargestHoldM = 0.0f;
    mKmFromM      = 0.0f;
    mKmCount      = 0;

    mTrackNumber    = 0;
    mJoinKind       = Join::Start;
    mAutoJoins      = 0;
    mAbandonSeen    = false;
    mRestarted      = false;
    mJoinSeq        = 0;
    mJoinT          = 0;
    mHaveJoin       = false;
    mJoinLat        = 0.0;
    mJoinLon        = 0.0;
    mSegStartP      = Vec2();
    mRoadBaseBefore = 0.0;
    mRoadSegBefore  = 0.0f;
    mFarRun         = 0;
    mLastOnT       = 0;
    mAreaT          = 0;
    mAreaOdo        = 0.0f;
    mAreaTrackM     = 0.0f;

    mJoinModel.clear();
    mJoinHasModel = false;
    mJoinNumber   = 0;
    mEntry        = Entry();
    mHaveEntry    = false;

    mFarFixes     = 0;
    mActHead      = 0;
    mActCount     = 0;
    mActsPaused   = false;
    mPauseFromT   = 0;

    mStartSeq     = 0;
    mBlockOdo     = 0.0f;
    mAwayFirst    = false;
    mPendingLane  = kNoLane;

    for (Visited& v : mVisited) {
        v = Visited();
    }

    mRingSeq   = 0;
    mRingCount = 0;
    mRingT     = 0;
    mRingOdo   = 0.0f;
}

bool Session::isGood(const Sample& s) const noexcept
{
    return s.fix && std::isfinite(s.lat) && std::isfinite(s.lon) && !(s.hdop > cfg().maxHdop);
}

// ---------------------------------------------------------------------------
// Start, stop, pause, lane, Lap
// ---------------------------------------------------------------------------

bool Session::start(uint32_t tMs, uint8_t lane, const LaneTable& table, const TrackStore* store) noexcept
{
    if (!table.isValidLane(lane) && lane != kNoLane) {
        return false;
    }
    double     lat = 0.0, lon = 0.0;
    const bool primed = mTracker.primedPosition(lat, lon);
    const TrackRecord* rec = (lane != kNoLane && store != nullptr && primed) ? store->findNear(lat, lon) : nullptr;
    TrackModel stored;
    if (rec != nullptr) {
        stored = rec->model();
    }
    if (lane == kNoLane) {
        mTracker.reset(); // the road: no Tracker until a track is joined
    } else if (!mTracker.start(tMs, lane, table, rec != nullptr ? &stored : nullptr)) {
        return false;
    }
    mTable = table;
    clearRun();
    mStore   = store;
    mRunning = true;
    mStartT  = tMs;
    mLastT   = tMs;
    if (primed) {
        // The start point is the frame's origin; without primed fixes it is
        // the first good fix.
        mFrame.setOrigin(lat, lon);
    }
    mJoinT    = tMs;
    mLapEdgeT = tMs;
    mLastOnT  = tMs;
    if (lane == kNoLane) {
        mSeg = Segment::Road;
        return true;
    }
    // A lane: on a track from Start, a stored one if it is near.
    mSeg      = Segment::Track;
    mLane     = lane;
    mLastLane = lane;
    beginVisit(Join::Start, tMs, Vec2(), 0.0f);
    if (rec != nullptr) {
        mTrackNumber  = rec->number;
        mJoinModel    = stored;
        mJoinHasModel = true;
        mJoinNumber   = rec->number;
    }
    return true;
}

void Session::beginVisit(Join kind, uint32_t tMs, Vec2 p, float odo) noexcept
{
    mJoinKind    = kind;
    mJoinT       = tMs;
    mLapEdgeT    = tMs;
    mSegStartP   = p;
    mFarRun      = 0;
    mLastOnT     = tMs;
    mAreaT       = tMs;
    mAreaOdo     = odo;
    mAreaTrackM  = 0.0f;
    mAbandonSeen = false;
    mRestarted   = false;
    mAwayFirst   = false; // a lane given again ends any "No lane" block
    mPendingLane = kNoLane;
}

Session::Events Session::finish() noexcept
{
    Events ev;
    if (!mRunning || mFinished) {
        return ev;
    }
    mFinished = true;
    if (mSeg == Segment::Track && mTracker.isRunning()) {
        forward(mTracker.finish(), ev);
        endVisit();
    }
    updateShown();
    return ev;
}

void Session::addAct(uint32_t tMs, uint8_t kind, uint8_t lane, uint8_t from) noexcept
{
    if (mActCount == kActs) {
        // Full: the oldest Lap press gives way, so the pauses a replay keeps
        // out of its laps stay. With none, the oldest act goes, and the
        // pause state after it is kept for pausedAt().
        const uint8_t first = mActHead; // the oldest, in a full ring
        uint8_t       k     = 0;
        while (k < kActs && mActs[(first + k) % kActs].kind != kActLap) {
            ++k;
        }
        if (k == kActs) {
            k = 0;
            if (mActs[first].kind == kActPause || mActs[first].kind == kActResume) {
                mActsPaused = mActs[first].kind == kActPause;
            }
        }
        for (; k > 0; --k) { // close the gap from the older side
            mActs[(first + k) % kActs] = mActs[(first + k - 1u) % kActs];
        }
        mActCount = static_cast<uint8_t>(kActs - 1u); // the free slot is the head
    }
    mActs[mActHead] = Act{tMs, kind, lane, from};
    mActHead        = static_cast<uint8_t>((mActHead + 1u) % kActs);
    ++mActCount;
}

void Session::replayActs(uint32_t afterT, uint32_t toT, Events* ev) noexcept
{
    for (uint8_t k = 0; k < mActCount; ++k) {
        const Act& a = mActs[(mActHead + kActs - mActCount + k) % kActs];
        if (msDiff(a.tMs, afterT) <= 0 || msDiff(a.tMs, toT) > 0) {
            continue;
        }
        if (a.kind == kActPause) {
            mTracker.pause(a.tMs);
        } else if (a.kind == kActResume) {
            mTracker.resume(a.tMs);
        } else if (a.kind == kActLane) {
            mTracker.setLane(a.lane);
        } else {
            const Tracker::Events te = mTracker.restartLap(a.tMs);
            if (ev != nullptr) {
                forward(te, *ev);
            }
        }
    }
}

uint8_t Session::laneAt(uint32_t tMs) const noexcept
{
    for (uint8_t k = 0; k < mActCount; ++k) {
        const Act& a = mActs[(mActHead + kActs - mActCount + k) % kActs];
        if (a.kind == kActLane && msDiff(a.tMs, tMs) > 0) {
            return a.from;
        }
    }
    return mLane;
}

void Session::pause(uint32_t tMs) noexcept
{
    if (!mRunning || mPaused) {
        return;
    }
    mPaused     = true;
    mPauseFromT = tMs;
    addAct(tMs, kActPause);
    if (mTracker.isRunning()) {
        mTracker.pause(tMs);
    }
}

void Session::resume(uint32_t tMs) noexcept
{
    if (!mRunning || !mPaused) {
        return;
    }
    mPaused = false;
    addAct(tMs, kActResume);
    if (mTracker.isRunning()) {
        mTracker.resume(tMs);
    }
}

Session::Events Session::setLane(uint8_t lane, uint32_t tMs, bool* ok) noexcept
{
    Events ev;
    bool   done = false;
    if (mRunning && !mFinished) {
        // While paused, at the pause: nothing was run since.
        const uint32_t when = mPaused ? mPauseFromT : (msAfter(tMs, mLastT) ? tMs : mLastT);
        if (mSeg == Segment::Track && lane == kNoLane) {
            // "No lane" on a track: the road from here, or from where the
            // runner went off it if they already have. That track is not
            // joined again by itself until the runner has been away from
            // every known track (an infield session, a warm-down beside it).
            if (mJoinKind == Join::Runner && !msAfter(mLastT, mJoinT) && mTracker.lapsCompleted() == 0) {
                // The runner's lane, then "No lane" before the next reading
                // (a mis-tap put right): no visit at all (leaveTrack).
                leaveTrack(mJoinT, static_cast<float>(mOdo.m), mTracker.measuredDistanceM(), ev);
            } else if (mTracker.isRunning() && mTracker.isOffTrack()) {
                const uint32_t t = mTracker.offTrackFromTMs();
                leaveTrack(t, odoAtTime(t), mTracker.measuredDistanceM(), ev);
            } else {
                leaveTrack(when, static_cast<float>(mOdo.m), mTracker.measuredDistanceM(), ev);
            }
            mAwayFirst = true;
            done       = true;
        } else if (mSeg == Segment::Track && mTable.isValidLane(lane)) {
            addAct(when, kActLane, lane, mLane);
            mLane     = lane;
            mLastLane = lane;
            if (mTracker.isRunning()) {
                mTracker.setLane(lane);
            }
            if (mJoinKind == Join::Auto) {
                mJoinKind = Join::Runner; // the runner's track now: not undone
            }
            done = true;
        } else if (mSeg == Segment::Road && lane == kNoLane) {
            mPendingLane = kNoLane;
            done         = true;
        } else if (mSeg == Segment::Road && mTable.isValidLane(lane)) {
            if (mHaveGood && msDiff(when, mGood.tMs) <= kGoodFreshMs) {
                joinHere(lane, when, ev);
            } else {
                mPendingLane = lane; // joined at the next good reading
            }
            done = true;
        }
    }
    if (ok != nullptr) {
        *ok = done;
    }
    updateShown();
    return ev;
}

Session::Events Session::lapPress(uint32_t tMs) noexcept
{
    Events ev;
    if (!mRunning) {
        return ev;
    }
    if (mSeg == Segment::Track && mTracker.isRunning()) {
        addAct(tMs, kActLap);
        forward(mTracker.restartLap(tMs), ev);
        mLapEdgeT = tMs; // laps count again from here
    } else if (mSeg == Segment::Road) {
        // The next road split is a whole roadSplitM from here. A join timed
        // back before the press replays it as a Lap press on the track.
        addAct(tMs, kActLap);
        const float segM = static_cast<float>(mOdo.m) - mRoadSegOdo;
        mKmFromM = segM > 0.0f ? segM : 0.0f;
        mKmCount = 0;
    }
    updateShown();
    return ev;
}

// ---------------------------------------------------------------------------
// Getters for the service
// ---------------------------------------------------------------------------

bool Session::lanePosition(double& lat, double& lon) const noexcept
{
    return mSeg == Segment::Track && mTracker.isRunning() && mTracker.lanePosition(lat, lon);
}

bool Session::timeOfDistance(float totalM, uint32_t& tMs) const noexcept
{
    if (!mRunning || mSeg != Segment::Track || !mTracker.isRunning()) {
        return false;
    }
    // The Tracker's total is the run's, less the segments closed before it.
    return mTracker.timeOfDistance(totalM - segmentStartTotalM(), tMs);
}

uint32_t Session::earliestRetimeTMs() const noexcept
{
    uint32_t b = mLastT;
    if (!mRunning) {
        return b;
    }
    auto earlier = [&b](uint32_t t) {
        if (msAfter(b, t)) {
            b = t;
        }
    };
    // A lap refined, or credited after a gap or a pending pass, ends after the
    // lap before it; the refined end moves a few seconds at most.
    constexpr uint32_t kLapSlackMs = 15000;
    // Until the visit's first fit confirms the stored track, laps
    // shown on it can still be re-timed and credited again from counting
    // mode's (Tracker::dropTurned), back to the start of the visit.
    const bool lining = mTracker.isRunning() && mTracker.suppliedUnsettled();
    if (mSeg == Segment::Track && mTracker.isRunning()) {
        // On a track no join is looked for. What can still be timed back:
        // a wrong pick or an undone join while the model is being lined up
        // (to the start of the visit), an exit (to the last reading on the track,
        // or in counting mode the last reading near where the segment began),
        // and the Tracker's laps.
        if (lining) {
            earlier(mJoinT);
        }
        earlier(mLastOnT);
        if (mTracker.isOffTrack()) {
            earlier(mTracker.offTrackFromTMs());
        }
        if (mTracker.mode() != Tracker::Mode::Calibrated) {
            earlier(mAreaT);
        }
        earlier(mLapEdgeT - kLapSlackMs);
        return b;
    }
    // On the road: a join noticed, and the laps credited with it, start at a
    // ring point.
    if (mRingCount > 0) {
        earlier(timeAt(mRingSeq - mRingCount));
    } else {
        earlier(mStartT);
    }
    return b;
}

// ---------------------------------------------------------------------------
// Per reading
// ---------------------------------------------------------------------------

Session::Events Session::update(const Sample& s) noexcept
{
    Events ev;
    if (!mRunning || mFinished) {
        return ev;
    }
    if (mHaveT && msDiff(s.tMs, mLastT) <= 0) {
        return ev;
    }
    const uint32_t prevT   = mHaveT ? mLastT : mStartT;
    const float    prevOdo = static_cast<float>(mOdo.m);
    mHaveT = true;
    mLastT = s.tMs;

    bool good = isGood(s);
    bool wild = false; // a fix far from the frame, not used
    Vec2 p;
    if (good) {
        if (!mFrame.hasOrigin()) {
            mFrame.setOrigin(s.lat, s.lon);
        }
        p = mFrame.toLocal(s.lat, s.lon);
        if (length(p) > kRecentreM) {
            // Far from the frame's origin. A runner gets there over many
            // readings; one reading there, or two, is a fix gone wild, and is
            // not used (recentring on it would pull every ring point to the
            // edge of the frame).
            if (++mFarFixes < kRecentreReadings) {
                good = false;
                wild = true;
                p    = Vec2();
            } else {
                recentre(s.lat, s.lon);
                p = Vec2();
                mFarFixes = 0;
            }
        } else {
            mFarFixes = 0;
        }
    }
    if (good) {
        mGood     = s; // where a lane set on the road joins a track
        mHaveGood = true;
    }
    odometer(s, good, p);
    if (good && mPendingLane != kNoLane && mSeg == Segment::Road) {
        // A lane set with no fresh reading joins here, after this reading's
        // road distance (a GPS outage's way is road); while paused, at the
        // pause, as a lane set then would be.
        const uint8_t lane = mPendingLane;
        mPendingLane = kNoLane;
        roadKilometres(prevT, prevOdo, ev); // a kilometre this reading crossed
        joinHere(lane, mPaused ? mPauseFromT : s.tMs, ev);
    }
    const bool added = (good && !mPaused) ? addPoint(s, p) : false;

    switch (mSeg) {
    case Segment::None:
        break;
    case Segment::Track:
        if (wild) {
            // The Tracker gets it as a reading with no fix: its time and the
            // kernel's distance still count, its position does not.
            Sample noFix = s;
            noFix.fix    = false;
            trackStep(noFix, good, p, added, ev);
        } else {
            trackStep(s, good, p, added, ev);
        }
        break;
    case Segment::Road:
        roadStep(s, added, ev);
        break;
    }
    if (mSeg == Segment::Road) {
        roadKilometres(prevT, prevOdo, ev);
    }
    updateShown();
    return ev;
}

void Session::addEvent(Events& ev, EventType type, uint32_t tMs) noexcept
{
    if (ev.count >= kMaxEvents) {
        return;
    }
    Event& e      = ev.items[ev.count++];
    e             = Event();
    e.type        = type;
    e.tMs         = tMs;
    e.segment     = mSeg;
    e.trackNumber = mTrackNumber;
    e.totalM      = roadDistanceM() + static_cast<float>(mTrackBaseM)
        + (mSeg == Segment::Track && mTracker.isRunning() ? mTracker.creditedDistanceM() : 0.0f);
}

void Session::segmentChanged(Events& ev, Segment from, Segment to, uint32_t tMs) noexcept
{
    addEvent(ev, EventType::SegmentChanged, tMs);
    if (ev.count > 0 && ev.items[ev.count - 1].type == EventType::SegmentChanged) {
        ev.items[ev.count - 1].from    = from;
        ev.items[ev.count - 1].segment = to;
    }
}

void Session::forward(const Tracker::Events& tev, Events& ev) noexcept
{
    for (uint8_t k = 0; k < tev.count && ev.count < kMaxEvents; ++k) {
        const Tracker::Event& t = tev.items[k];
        Event& e      = ev.items[ev.count++];
        e             = Event();
        e.type        = isLapType(t.type) ? EventType::TrackLap : EventType::TrackEvent;
        e.tMs         = t.tMs;
        e.segment     = mSeg;
        e.trackNumber = mTrackNumber;
        e.runLap      = t.lapNumber > 0 ? static_cast<uint16_t>(mLapsBase + t.lapNumber) : 0;
        e.totalM      = roadDistanceM() + static_cast<float>(mTrackBaseM) + t.totalM;
        e.track       = t;
        if (isLapType(t.type) && msAfter(t.tMs, mLapEdgeT)) {
            mLapEdgeT = t.tMs; // bookkeeping for earliestRetimeTMs() only
        }
    }
}

void Session::updateShown() noexcept
{
    const float cand = roadDistanceM() + trackDistanceM();
    if (cand > mShownM) {
        mShownM = cand;
    } else if (mShownM - cand > mLargestHoldM) {
        mLargestHoldM = mShownM - cand;
    }
}

// ---------------------------------------------------------------------------
// Odometer, frame and ring
// ---------------------------------------------------------------------------

void Session::odometer(const Sample& s, bool good, Vec2 p) noexcept
{
    // As counting mode reads the kernel's distance (LapCounter::updateOdometer).
    float delta = 0.0f;
    bool  have  = false;
    if (std::isfinite(s.gpsDistM)) {
        if (mOdo.haveGps) {
            const float d = s.gpsDistM - mOdo.lastGps;
            if (d != 0.0f) {
                delta        = limitStep(d, msDiff(s.tMs, mOdo.lastGpsT), cfg().maxSpeedMps);
                mOdo.lastGpsT = s.tMs;
            }
            have = true;
        } else {
            mOdo.lastGpsT = s.tMs;
        }
        mOdo.haveGps = true;
        mOdo.lastGps = s.gpsDistM;
    } else {
        mOdo.haveGps = false;
    }
    if (good) {
        if (!have && mOdo.haveStep) {
            delta = limitStep(length(p - mOdo.stepP), msDiff(s.tMs, mOdo.stepT), cfg().maxSpeedMps);
            have  = true;
        }
        mOdo.haveStep = true;
        mOdo.stepP    = p;
        mOdo.stepT    = s.tMs;
    }
    if (have && !mPaused) {
        mOdo.m += static_cast<double>(delta);
    }
}

void Session::recentre(double lat, double lon) noexcept
{
    // The new origin in the old frame; everything kept moves by it.
    const Vec2 d = mFrame.toLocal(lat, lon);
    mFrame.setOrigin(lat, lon);
    const float dx = std::round(d.x * 10.0f);
    const float dy = std::round(d.y * 10.0f);
    for (uint16_t i = 0; i < kRingPoints; ++i) {
        mRing[i].x = toDm(0.1f * (static_cast<float>(mRing[i].x) - dx));
        mRing[i].y = toDm(0.1f * (static_cast<float>(mRing[i].y) - dy));
    }
    mSegStartP  = mSegStartP - d;
    mEntry.p    = mEntry.p - d;
    mOdo.stepP  = mOdo.stepP - d;
}

bool Session::addPoint(const Sample& s, Vec2 p) noexcept
{
    const float odo = static_cast<float>(mOdo.m);
    if (mRingCount > 0 && odo - mRingOdo < cfg().ringSpacingM) {
        return false;
    }
    RingPoint& r = mRing[mRingSeq % kRingPoints];
    r.x   = toDm(p.x);
    r.y   = toDm(p.y);
    r.tMs = s.tMs;
    if (s.hdop >= 0.0f) {
        const float h = std::round(s.hdop * 20.0f);
        r.hdop20 = static_cast<uint8_t>(h < 1.0f ? 1.0f : (h > 255.0f ? 255.0f : h));
    } else {
        r.hdop20 = 0;
    }
    if (mRingCount == 0) {
        r.dOdo10 = 0;
        mRingOdo = odo;
    } else {
        // Rounded with the error carried on, so the sum never drifts.
        float d = std::round((odo - mRingOdo) * 10.0f);
        d = d < 0.0f ? 0.0f : (d > 65535.0f ? 65535.0f : d);
        r.dOdo10 = static_cast<uint16_t>(d);
        mRingOdo += 0.1f * d;
    }
    mRingT = s.tMs;
    ++mRingSeq;
    if (mRingCount < kRingPoints) {
        ++mRingCount;
    }
    return true;
}

bool Session::inRing(uint32_t seq) const noexcept
{
    return seq < mRingSeq && mRingSeq - seq <= mRingCount;
}

Vec2 Session::pointAt(uint32_t seq) const noexcept
{
    const RingPoint& r = mRing[seq % kRingPoints];
    return {0.1f * static_cast<float>(r.x), 0.1f * static_cast<float>(r.y)};
}

uint32_t Session::timeAt(uint32_t seq) const noexcept
{
    return mRing[seq % kRingPoints].tMs;
}

float Session::odoAt(uint32_t seq) const noexcept
{
    float o = mRingOdo;
    for (uint32_t q = mRingSeq - 1u; q > seq; --q) {
        o -= 0.1f * static_cast<float>(mRing[q % kRingPoints].dOdo10);
    }
    return o;
}

float Session::odoAtTime(uint32_t tMs) const noexcept
{
    if (mRingCount == 0) {
        return static_cast<float>(mOdo.m);
    }
    if (msDiff(tMs, mRingT) >= 0) {
        // Between the newest point and now.
        const int32_t span = msDiff(mLastT, mRingT);
        const float   now  = static_cast<float>(mOdo.m);
        if (span <= 0) {
            return now;
        }
        const float f = static_cast<float>(msDiff(tMs, mRingT)) / static_cast<float>(span);
        return mRingOdo + (now - mRingOdo) * (f > 1.0f ? 1.0f : f);
    }
    float    oNewer = mRingOdo;
    uint32_t tNewer = mRingT;
    const uint32_t oldest = mRingSeq - mRingCount;
    for (uint32_t q = mRingSeq - 1u; q > oldest; --q) {
        const float    oOlder = oNewer - 0.1f * static_cast<float>(mRing[q % kRingPoints].dOdo10);
        const uint32_t tOlder = timeAt(q - 1u);
        if (msDiff(tMs, tOlder) >= 0) {
            const int32_t span = msDiff(tNewer, tOlder);
            const float   f    = span > 0 ? static_cast<float>(msDiff(tMs, tOlder)) / static_cast<float>(span) : 1.0f;
            return oOlder + (oNewer - oOlder) * f;
        }
        oNewer = oOlder;
        tNewer = tOlder;
    }
    return oNewer;
}

uint32_t Session::timeAtOdo(float odo, uint32_t prevT, float prevOdo) const noexcept
{
    const float now = static_cast<float>(mOdo.m);
    if (odo >= prevOdo && now > prevOdo) {
        const float    f  = (odo - prevOdo) / (now - prevOdo);
        const int32_t  dt = msDiff(mLastT, prevT);
        return prevT + static_cast<uint32_t>(static_cast<float>(dt > 0 ? dt : 0) * (f > 1.0f ? 1.0f : f) + 0.5f);
    }
    if (mRingCount == 0) {
        return mLastT;
    }
    float    oNewer = mRingOdo;
    uint32_t tNewer = mRingT;
    const uint32_t oldest = mRingSeq - mRingCount;
    for (uint32_t q = mRingSeq - 1u; q > oldest; --q) {
        const float    oOlder = oNewer - 0.1f * static_cast<float>(mRing[q % kRingPoints].dOdo10);
        const uint32_t tOlder = timeAt(q - 1u);
        if (odo >= oOlder) {
            const float   span = oNewer - oOlder;
            const float   f    = span > 0.0f ? (odo - oOlder) / span : 1.0f;
            const int32_t dt   = msDiff(tNewer, tOlder);
            return tOlder + static_cast<uint32_t>(static_cast<float>(dt > 0 ? dt : 0) * f + 0.5f);
        }
        oNewer = oOlder;
        tNewer = tOlder;
    }
    return tNewer;
}

Sample Session::sampleAt(uint32_t seq, float odo, float gpsNow) const noexcept
{
    // A reading as it came, from the ring: the kernel's distance is put back
    // on today's total, so the readings after the replay carry on from it.
    Sample s;
    s.tMs = timeAt(seq);
    mFrame.toLatLon(pointAt(seq), s.lat, s.lon);
    s.fix = true;
    const uint8_t h = mRing[seq % kRingPoints].hdop20;
    s.hdop     = h > 0 ? 0.05f * static_cast<float>(h) : -1.0f;
    s.gpsDistM = std::isfinite(gpsNow) ? gpsNow - (static_cast<float>(mOdo.m) - odo) : gpsNow;
    return s;
}

void Session::replay(uint32_t seq, uint8_t lane, const TrackModel* model, const Sample* live, bool addedLive,
                     Events* ev, const Entry* entry) noexcept
{
    // A fresh Tracker, started at ring point seq as if Start had been pressed
    // there, then the readings since. With an entry, started there
    // instead, on a reading made at the entry, then ring point seq on.
    const float gpsNow = (live != nullptr && std::isfinite(live->gpsDistM))
        ? live->gpsDistM
        : std::numeric_limits<float>::quiet_NaN();
    mTracker.reset();
    mAbandonSeen = false;
    float  odo = odoAt(seq);
    Sample s0  = sampleAt(seq, odo, gpsNow);
    if (entry != nullptr) {
        s0.tMs = entry->tMs;
        mFrame.toLatLon(entry->p, s0.lat, s0.lon);
        s0.hdop     = entry->hdop20 > 0 ? 0.05f * static_cast<float>(entry->hdop20) : -1.0f;
        s0.gpsDistM = std::isfinite(gpsNow) ? gpsNow - (static_cast<float>(mOdo.m) - entry->odo) : gpsNow;
    }
    mTracker.prime(s0);
    mTracker.start(s0.tMs, lane, mTable, model);
    if (pausedAt(s0.tMs)) {
        mTracker.pause(s0.tMs);
    }
    if (model == &mJoinModel && mJoinNumber == 0) {
        // A track visited earlier in the run, not stored: its own fit,
        // already lined up with today's GPS. Nothing to confirm (the turn
        // checks are for a stored pick).
        mTracker.confirmOwnFit();
    }
    if (entry != nullptr) {
        // As at a ring point, the start reading is also the first one.
        const Tracker::Events te = mTracker.update(s0);
        if (ev != nullptr) {
            forward(te, *ev);
        }
    }
    // The runner's pauses, resumes and Lap presses since the Tracker's start
    // go in at their own times, between the readings either side.
    uint32_t lastT = s0.tMs;
    const uint32_t newest = mRingSeq - 1u;
    for (uint32_t q = seq;; ++q) {
        if (q != seq) {
            odo += 0.1f * static_cast<float>(mRing[q % kRingPoints].dOdo10);
        }
        const Sample sq = sampleAt(q, odo, gpsNow);
        replayActs(lastT, sq.tMs, ev);
        lastT = sq.tMs;
        const Tracker::Events te = mTracker.update(sq);
        if (ev != nullptr) {
            forward(te, *ev);
        }
        if (q == newest) {
            break;
        }
    }
    if (live != nullptr && !addedLive) {
        replayActs(lastT, live->tMs, ev);
        lastT = live->tMs;
        const Tracker::Events te = mTracker.update(*live);
        if (ev != nullptr) {
            forward(te, *ev);
        }
    }
    replayActs(lastT, mLastT, ev);
    if (mPaused) {
        // From where the pause began, not the last reading (if the acts
        // above have not already paused it).
        mTracker.pause(msDiff(mPauseFromT, s0.tMs) > 0 ? mPauseFromT : mLastT);
    }
}

bool Session::entryAt(uint32_t join, uint32_t lowest, const TrackModel& m, float c, Entry& out) const noexcept
{
    // The join point is the earliest reading on the runner's line;
    // on a slanting approach that is a reading or more before they reached
    // it, and the station of any one reading carries its noise along the
    // track. So, in the track's terms (station s along the kerb line and the
    // offset l from the median line c, model m placed in the Session's
    // frame), with the readings either side of the join point, a hinge is
    // fitted (below): the approach and the run along the line meeting at
    // the time te, at the station se. An approach that does not slant
    // clearly keeps the join point's time, with its station from the
    // readings either side. The entry is the point at se on the runner's
    // line, at te. Docs: OVAL.md.
    const uint32_t newest = mRingSeq - 1u;
    if (!inRing(join) || join < lowest || join >= newest) {
        return false;
    }
    uint32_t q0 = join > lowest + kEntryBack ? join - kEntryBack : lowest;
    for (uint32_t k = join; k > q0; --k) {
        if (msDiff(timeAt(k), timeAt(k - 1u)) > kJoinGapMs) {
            q0 = k; // not across a pause or a GPS outage
            break;
        }
    }
    const uint32_t q1   = newest - join > kEntryFwd ? join + kEntryFwd : newest;
    const uint32_t tJ   = timeAt(join);
    const float    lapK = m.lapLengthM(0.0f);
    float    tS[kEntryPts], sM[kEntryPts], lM[kEntryPts], wM[kEntryPts];
    uint8_t  n = 0, at = 0;
    float    s0 = 0.0f, sPrev = 0.0f, sRun = 0.0f;
    for (uint32_t q = q0; q <= q1 && n < kEntryPts; ++q) {
        const TrackModel::Projection pr = m.project(pointAt(q), 0.0f);
        const float l = pr.lateralM - c;
        if (q < join && std::fabs(l) > kEntryFarM) {
            continue; // the road before the last leg
        }
        if (n == 0) {
            s0 = pr.s;
        } else {
            float ds = pr.s - sPrev;
            ds -= std::floor(ds / lapK + 0.5f) * lapK;
            sRun += ds;
        }
        sPrev = pr.s;
        if (q == join) {
            at = n;
        }
        tS[n] = 0.001f * static_cast<float>(msDiff(timeAt(q), tJ));
        sM[n] = sRun;
        lM[n] = l;
        // Weighted by 1 / HDOP squared: a reading's noise grows with it.
        const uint8_t h20 = mRing[q % kRingPoints].hdop20;
        const float   h   = h20 > 0 ? 0.05f * static_cast<float>(h20) : 1.0f;
        wM[n] = 1.0f / (h > 0.5f ? h * h : 0.25f);
        ++n;
    }
    if (n < at + kEntryMinOn + 1u) {
        return false;
    }
    // The hinge, at a given te, by least squares on both coordinates:
    //  - offset l = c' + a x, x = max(0, te - t) (the runner's line c' is
    //    fitted too: against a stored track it moves by a few metres round
    //    the lap with the day's offset);
    //  - station s = se + u y + w z, y = min(0, t - te), z = max(0, t - te):
    //    the approach and the run along the line meet at se, so the
    //    approach's own stations place the entry, not only the time.
    // The station fit takes only the last leg: readings from kEntryLegS before
    // the join point (a road can turn just before it, where the station
    // stops changing).
    // gain is how much better that fits than "on one line, at one speed,
    // throughout". Every 0.5 s, then every 0.05 s round the best.
    const float tLeg = tS[at] - kEntryLegS;
    float nf = 0.0f, sl = 0.0f, sll = 0.0f;
    float nfS = 0.0f, ss = 0.0f, sss = 0.0f, stt0 = 0.0f, st0 = 0.0f, sts0 = 0.0f;
    for (uint8_t i = 0; i < n; ++i) {
        const float w = wM[i];
        nf += w;
        sl += w * lM[i];
        sll += w * lM[i] * lM[i];
        if (tS[i] >= tLeg) {
            nfS += w;
            ss += w * sM[i];
            sss += w * sM[i] * sM[i];
            st0 += w * tS[i];
            stt0 += w * tS[i] * tS[i];
            sts0 += w * tS[i] * sM[i];
        }
    }
    // The null fit: l constant, s a straight line in t.
    const float det0 = nfS * stt0 - st0 * st0;
    if (!(det0 > 0.0f)) {
        return false;
    }
    const float b0   = (nfS * sts0 - st0 * ss) / det0;
    const float a0   = (ss - b0 * st0) / nfS;
    const float rss0 = (sll - sl * sl / nf) + (sss - a0 * ss - b0 * sts0);
    struct Hinge {
        float rss = 0.0f, slope = 0.0f, line = 0.0f, se = 0.0f;
        bool  ok  = false;
    };
    auto fit = [&](float te) {
        Hinge h;
        float sx = 0.0f, sxx = 0.0f, sxl = 0.0f;
        float py = 0.0f, pyy = 0.0f, pys = 0.0f, qz = 0.0f, qzz = 0.0f, qzs = 0.0f;
        for (uint8_t i = 0; i < n; ++i) {
            const float d = tS[i] - te;
            const float w = wM[i];
            const bool  s = tS[i] >= tLeg;
            if (d < 0.0f) {
                sx += -w * d;
                sxx += w * d * d;
                sxl += -w * d * lM[i];
                if (s) {
                    py += w * d;
                    pyy += w * d * d;
                    pys += w * d * sM[i];
                }
            } else if (s) {
                qz += w * d;
                qzz += w * d * d;
                qzs += w * d * sM[i];
            }
        }
        const float det = nf * sxx - sx * sx;
        if (!(det > 0.0f) || !(pyy > 0.0f) || !(qzz > 0.0f)) {
            return h;
        }
        h.slope = (nf * sxl - sx * sl) / det;
        h.line  = (sl - h.slope * sx) / nf;
        const float rl = sll - h.line * sl - h.slope * sxl;
        // y z = 0 always: the normal equations reduce to one for se.
        const float dd = nfS - py * py / pyy - qz * qz / qzz;
        if (!(dd > 0.0f)) {
            return h;
        }
        h.se = (ss - py * pys / pyy - qz * qzs / qzz) / dd;
        const float u  = (pys - py * h.se) / pyy;
        const float w  = (qzs - qz * h.se) / qzz;
        const float rs = sss - h.se * ss - u * pys - w * qzs;
        h.rss = rl + rs;
        h.ok  = true;
        return h;
    };
    const float tLo = tS[at] - 3.0f > tS[0] ? tS[at] - 3.0f : tS[0];
    const float tHi = tS[n - kEntryMinOn] < tS[at] + kEntryLateS ? tS[n - kEntryMinOn] : tS[at] + kEntryLateS;
    float te = tS[at], bestRss = rss0;
    bool  have = false;
    for (float t = tLo; t <= tHi; t += 0.5f) {
        const Hinge h = fit(t);
        if (h.ok && h.rss < bestRss) {
            bestRss = h.rss;
            te      = t;
            have    = true;
        }
    }
    const float tc = te;
    for (float t = tc - 0.5f; have && t <= tc + 0.5f; t += 0.05f) {
        if (t < tLo || t > tHi) {
            continue;
        }
        const Hinge h = fit(t);
        if (h.ok && h.rss < bestRss) {
            bestRss = h.rss;
            te      = t;
        }
    }
    const Hinge h     = have ? fit(te) : Hinge();
    const float best  = rss0 - bestRss;
    const float slope = h.slope;
    float       line  = h.line;
    // The noise about the hinge (two coordinates, six parameters), at HDOP 1.
    const float var  = n > 3 ? bestRss / (2.0f * static_cast<float>(n) - 6.0f) : 0.0f;
    const bool  slant = have && h.ok && std::fabs(slope) >= kEntrySlopeMps
        && best >= kEntrySigma2 * (var > 1.0f ? var : 1.0f);
    if (!slant) {
        te   = tS[at]; // no clear slant: the join point's own time, on the line of the readings after it
        line = 0.0f;
        uint8_t no = 0;
        for (uint8_t i = at; i < n; ++i) {
            line += lM[i];
            ++no;
        }
        line = no > 0 ? line / static_cast<float>(no) : 0.0f;
    }
    // The station at te: where the two legs meet; with no clear slant, a line
    // through the stations of the readings within kEntryLegS either side of
    // the join point (already on the line there). Either way the runner must
    // be moving along the line after te.
    auto lineFit = [&](float from, float to, uint8_t most, float& v, float& alpha) {
        float   st = 0.0f, ssum = 0.0f, stt = 0.0f, sts = 0.0f;
        uint8_t k = 0;
        for (uint8_t i = 0; i < n && k < most; ++i) {
            if (tS[i] < from || tS[i] > to) {
                continue;
            }
            const float d = tS[i] - te;
            st += d;
            ssum += sM[i];
            stt += d * d;
            sts += d * sM[i];
            ++k;
        }
        const float kf  = static_cast<float>(k);
        const float den = kf * stt - st * st;
        if (k < kEntryMinOn || !(den > 0.0f)) {
            return false;
        }
        v     = (kf * sts - st * ssum) / den;
        alpha = (ssum - v * st) / kf;
        return true;
    };
    float v = 0.0f, alpha = 0.0f;
    if (!lineFit(te, tS[n - 1], kEntryOn, v, alpha) || std::fabs(v) < cfg().stillMps) {
        return false; // not moving along the line
    }
    if (slant) {
        alpha = h.se;
    } else {
        float v2 = 0.0f;
        lineFit(te - kEntryLegS, te + kEntryLegS, kEntryPts, v2, alpha);
    }
    float se = s0 + alpha;
    se -= std::floor(se / lapK) * lapK;
    out.p   = m.pointAt(m.tauOf(se, 0.0f), 0.0f, c + line);
    out.tMs = tJ + static_cast<uint32_t>(static_cast<int32_t>(std::lround(1000.0f * te)));
    // The first ring point after it.
    uint32_t q = q0;
    while (q < newest && msDiff(timeAt(q), out.tMs) <= 0) {
        ++q;
    }
    if (msDiff(timeAt(q), out.tMs) <= 0 || msDiff(out.tMs, timeAt(lowest)) < 0) {
        return false;
    }
    out.next   = q;
    out.odo    = odoAtTime(out.tMs);
    out.hdop20 = mRing[q % kRingPoints].hdop20;
    return true;
}

bool Session::restartOther(const Sample& s, bool added, Events& ev) noexcept
{
    // The stored track picked at the start of the visit (Start, or a lane
    // set on the road) was dropped before any lap counted. If another stored
    // track lies within storeNearM of where the visit began, the visit
    // starts again on it: a fresh Tracker, primed with counting mode's start
    // point (where the runner was) and started at the visit's start, then
    // the readings and the runner's acts since replayed from the ring, with
    // a pause in force at the start kept. No lap had counted; one the replay
    // credits is raised, and the display holds. Lining up then confirms the
    // track or drops it in turn; only one such restart is made a visit.
    mRestarted = true;
    if (mStore == nullptr || mRingCount == 0 || !mTracker.counter().frame().hasOrigin()) {
        return false;
    }
    double lat = 0.0, lon = 0.0;
    mFrame.toLatLon(mSegStartP, lat, lon);
    const TrackRecord* rec = mStore->findNear(lat, lon, nullptr, mTrackNumber);
    if (rec == nullptr) {
        return false;
    }
    Sample s0;
    s0.tMs = mJoinT;
    mTracker.counter().frame().toLatLon(mTracker.counter().startPoint(), s0.lat, s0.lon);
    s0.fix  = true;
    s0.hdop = -1.0f;
    s0.gpsDistM = std::numeric_limits<float>::quiet_NaN();
    s0.altM     = std::numeric_limits<float>::quiet_NaN();
    const TrackModel model = rec->model();
    const float gpsNow = std::isfinite(s.gpsDistM) ? s.gpsDistM : std::numeric_limits<float>::quiet_NaN();
    mTracker.reset();
    mAbandonSeen = false;
    mTracker.prime(s0);
    // On the lane at the join: the lane changes since go in at their times.
    if (!mTracker.start(mJoinT, laneAt(mJoinT), mTable, &model)) {
        return false;
    }
    if (pausedAt(mJoinT)) {
        mTracker.pause(mJoinT);
    }
    mTrackNumber  = rec->number;
    mJoinModel    = model;
    mJoinHasModel = true;
    mJoinNumber   = rec->number;
    const uint32_t oldest = mRingSeq - mRingCount;
    float          odo    = odoAt(oldest);
    uint32_t       lastT  = mJoinT;
    for (uint32_t q = oldest; q != mRingSeq; ++q) {
        if (q != oldest) {
            odo += 0.1f * static_cast<float>(mRing[q % kRingPoints].dOdo10);
        }
        if (msAfter(timeAt(q), mJoinT)) {
            replayActs(lastT, timeAt(q), &ev);
            lastT = timeAt(q);
            forward(mTracker.update(sampleAt(q, odo, gpsNow)), ev);
        }
    }
    if (!added) {
        replayActs(lastT, s.tMs, &ev);
        lastT = s.tMs;
        forward(mTracker.update(s), ev);
    }
    replayActs(lastT, mLastT, &ev);
    if (mPaused) {
        mTracker.pause(msDiff(mPauseFromT, mJoinT) > 0 ? mPauseFromT : mLastT);
    }
    return true;
}

bool Session::pausedAt(uint32_t tMs) const noexcept
{
    // The runner's latest pause or resume at or before tMs, from the acts
    // (or as it was after the oldest one given up).
    bool paused = mActsPaused;
    for (uint8_t k = 0; k < mActCount; ++k) {
        const Act& a = mActs[(mActHead + kActs - mActCount + k) % kActs];
        if (msDiff(a.tMs, tMs) > 0) {
            continue;
        }
        if (a.kind == kActPause) {
            paused = true;
        } else if (a.kind == kActResume) {
            paused = false;
        }
    }
    return paused;
}

// ---------------------------------------------------------------------------
// Segments
// ---------------------------------------------------------------------------

float Session::roadDistanceM() const noexcept
{
    float r = static_cast<float>(mRoadBaseM);
    if (mSeg == Segment::Road) {
        const float d = static_cast<float>(mOdo.m) - mRoadSegOdo;
        r += d > 0.0f ? d : 0.0f;
    }
    return r;
}

float Session::roadSplitShare() const noexcept
{
    const float split = cfg().roadSplitM;
    if (mSeg != Segment::Road || !(split > 0.0f)) {
        return 0.0f;
    }
    const float into = static_cast<float>(mOdo.m) - mRoadSegOdo - mKmFromM - static_cast<float>(mKmCount) * split;
    if (!(into > 0.0f)) {
        return 0.0f;
    }
    return into >= split ? 1.0f : into / split;
}

float Session::trackDistanceM() const noexcept
{
    float t = static_cast<float>(mTrackBaseM);
    if (mSeg == Segment::Track && mTracker.isRunning()) {
        t += mTracker.totalDistanceM();
    }
    return t;
}

uint16_t Session::segmentLaps() const noexcept
{
    return (mSeg == Segment::Track && mTracker.isRunning()) ? mTracker.lapsCompleted() : 0;
}

bool Session::holdElevation() const noexcept
{
    return mSeg == Segment::Track && mTracker.isRunning() && mTracker.hasModel();
}

bool Session::lanePositions() const noexcept
{
    return mSeg == Segment::Track && mTracker.isRunning() && mTracker.mode() == Tracker::Mode::Calibrated
        && !mTracker.isOffTrack();
}

bool Session::joinPoint(double& lat, double& lon) const noexcept
{
    if (!mHaveJoin) {
        return false;
    }
    lat = mJoinLat;
    lon = mJoinLon;
    return true;
}

bool Session::startTracker(uint32_t tMs, uint8_t lane, const TrackModel* model) noexcept
{
    mTracker.reset();
    mAbandonSeen = false;
    if (mHaveGood) {
        mTracker.prime(mGood); // the runner is here: counting mode's start point
    }
    if (!mTracker.start(tMs, lane, mTable, model)) {
        return false;
    }
    if (model != nullptr && mJoinNumber == 0) {
        mTracker.confirmOwnFit(); // a track visited earlier in the run, not stored
    }
    if (mPaused) {
        mTracker.pause(tMs);
    }
    return true;
}

void Session::joinHere(uint8_t lane, uint32_t tMs, Events& ev) noexcept
{
    // A lane set on the road: a track from here. The known track nearest,
    // if there is one within storeNearM (lined up with today's GPS as at
    // Start, and dropped for another or for a track that is not stored if
    // it is not this one), else a track that is not stored, learnt from here.
    TrackModel m;
    uint8_t    number = 0, knownLane = 0;
    int8_t     visited = -1;
    const bool known = mHaveGood && nearestKnown(mGood.lat, mGood.lon, cfg().storeNearM, m, number, visited, knownLane);
    const Vec2  p   = (mHaveGood && mFrame.hasOrigin()) ? mFrame.toLocal(mGood.lat, mGood.lon) : Vec2();
    const float odo = static_cast<float>(mOdo.m);
    const float seg = odo - mRoadSegOdo;
    keepBeforeJoin();
    mRoadBaseBefore = mRoadBaseM;
    mRoadSegBefore  = mRoadSegOdo;
    mRoadBaseM += static_cast<double>(seg > 0.0f ? seg : 0.0f);
    mSeg          = Segment::Track;
    mLane         = lane;
    mLastLane     = lane;
    mTrackNumber  = known ? number : 0;
    mJoinModel    = m;
    mJoinHasModel = known;
    mJoinNumber   = known ? number : 0;
    mHaveJoin     = mHaveGood;
    if (mHaveGood) {
        mJoinLat = mGood.lat;
        mJoinLon = mGood.lon;
    }
    beginVisit(Join::Runner, tMs, p, odo);
    segmentChanged(ev, Segment::Road, Segment::Track, tMs);
    if (!startTracker(tMs, lane, known ? &mJoinModel : nullptr) && known) {
        mTrackNumber  = 0;
        mJoinHasModel = false;
        mJoinNumber   = 0;
        startTracker(tMs, lane, nullptr);
    }
}

void Session::roadKilometres(uint32_t prevT, float prevOdo, Events& ev) noexcept
{
    const float split = cfg().roadSplitM;
    if (!(split > 0.0f)) {
        return;
    }
    const float segM = static_cast<float>(mOdo.m) - mRoadSegOdo;
    for (int guard = 0; guard < 8; ++guard) {
        const float target = mKmFromM + static_cast<float>(mKmCount + 1) * split;
        if (segM < target) {
            break;
        }
        const uint32_t t = timeAtOdo(mRoadSegOdo + target, prevT, prevOdo);
        ++mKmCount;
        addEvent(ev, EventType::RoadKilometre, t);
        if (ev.count > 0 && ev.items[ev.count - 1].type == EventType::RoadKilometre) {
            ev.items[ev.count - 1].kilometre = mKmCount;
        }
    }
}

void Session::trackStep(const Sample& s, bool good, Vec2 p, bool added, Events& ev) noexcept
{
    Tracker&      T = mTracker;
    const Config& c = cfg();
    forward(T.update(s), ev);

    if (T.alignment().abandoned && !mAbandonSeen) {
        mAbandonSeen = true;
        const bool turned = T.alignment().turned;
        if (mJoinKind == Join::Auto) {
            if (turned) {
                // A track turned from the stored one: the runner is on a
                // track all the same, another one. The segment stands; it is
                // measured as a track that is not stored from the join.
                mTrackNumber = 0;
                mJoinNumber  = 0;
                return;
            }
            undoJoin(ev); // not that track after all
            return;
        }
        // The runner's lane says a track, but not the stored one picked.
        // Another stored track near where the visit began may be (a warm-up
        // track beside the main one, both stored). Else counting mode has
        // measured from the start of the visit: a track that is not stored.
        if (!mRestarted && T.lapsCompleted() == 0 && restartOther(s, added, ev)) {
            return;
        }
        mTrackNumber = 0;
        mJoinNumber  = 0;
        return;
    }
    if (mPaused) {
        // No exit while paused: a runner who walks off (to a tap, a bag)
        // is judged afresh from the resume, where the rules below start
        // counting again.
        mFarRun = 0;
        return;
    }

    if (T.mode() == Tracker::Mode::Calibrated || T.hasLeft()) {
        // The Tracker's off-track check decides; sooner if clearly far away.
        if (good && T.hasLateralOffset()) {
            const float lat   = T.offLanesM(); // outside the lanes, whatever the lane set
            const float widen = hdopWiden(s.hdop, c);
            if (lat <= c.offLanesM * widen) {
                mLastOnT = s.tMs; // where the runner was last on the track
            }
            if (lat > c.exitFarM * widen) {
                if (mFarRun < 255) {
                    ++mFarRun;
                }
            } else {
                mFarRun = 0;
            }
        }
        const bool far = mFarRun >= (c.offTrackReadings > 0 ? c.offTrackReadings : 1);
        if ((far || T.hasLeft()) && T.alignment().aligning && mJoinKind != Join::Auto
            && !msAfter(mLastOnT, mJoinT)) {
            // Off a stored track still being lined up, and never on it since
            // the runner's lane: a track, but not that one. A wrong pick, as
            // lining up would find (another stored track near, else a track
            // that is not stored), from the next reading.
            forward(T.dropStoredPick(s.tMs), ev);
            mFarRun = 0;
            return;
        }
        if (T.hasLeft() || far) {
            // Left where the Tracker's check says, or (while a stored track is
            // still being lined up, when that check waits) at the last
            // reading on the track.
            const uint32_t t = T.isOffTrack() ? T.offTrackFromTMs() : mLastOnT;
            leaveTrack(t, odoAtTime(t), T.measuredDistanceM(), ev);
        }
        return;
    }
    // No fitted track (counting mode): the track lies round where the
    // segment began.
    if (good) {
        const float d = length(p - mSegStartP);
        if (d <= c.exitAwayM) {
            mAreaT      = s.tMs;
            mAreaOdo    = static_cast<float>(mOdo.m);
            mAreaTrackM = T.measuredDistanceM();
        } else if (d > c.exitCountingM) {
            leaveTrack(mAreaT, mAreaOdo, mAreaTrackM, ev);
        }
    }
}

void Session::leaveTrack(uint32_t exitT, float exitOdo, float trackM, Events& ev) noexcept
{
    if (mJoinKind != Join::Start && exitT == mJoinT && mTracker.lapsCompleted() == 0) {
        // Left the instant it was joined (as noticed, or a lane and "No
        // lane" before the next reading): no visit at all. Run::Splits reads
        // a change back at the join's time as an undone join, so the road
        // carries on as one (its kilometres numbered on).
        undoJoin(ev);
        return;
    }
    // The track part keeps what was measured up to where the runner left;
    // the road starts there.
    forward(mTracker.finish(), ev);
    endVisit();
    mTrackBaseM += static_cast<double>(trackM > 0.0f ? trackM : 0.0f);
    mLapsBase    = static_cast<uint16_t>(mLapsBase + mTracker.lapsCompleted());
    mRoadSegOdo  = exitOdo < static_cast<float>(mOdo.m) ? exitOdo : static_cast<float>(mOdo.m);
    mKmFromM     = 0.0f;
    mKmCount     = 0;
    mSeg         = Segment::Road;
    mLane        = kNoLane;
    segmentChanged(ev, Segment::Track, Segment::Road, exitT);
    mTracker.reset();
    mFarRun      = 0;
    mLastOnT     = mJoinT;
    mStartSeq    = mRingSeq; // watch for tracks from here on
}

void Session::keepBeforeJoin() noexcept
{
    mBeforeJoin.trackNumber = mTrackNumber;
    mBeforeJoin.lane     = mLane;
    mBeforeJoin.lastLane = mLastLane;
    mBeforeJoin.haveJoin = mHaveJoin;
    mBeforeJoin.joinT    = mJoinT;
    mBeforeJoin.lapEdgeT = mLapEdgeT;
    mBeforeJoin.joinLat  = mJoinLat;
    mBeforeJoin.joinLon  = mJoinLon;
}

void Session::restoreBeforeJoin() noexcept
{
    // The road's lane and the last join stand again: a stored track's lane
    // must not stay in force for a track the run never joined.
    mTrackNumber = mBeforeJoin.trackNumber;
    mLane     = mBeforeJoin.lane;
    mLastLane = mBeforeJoin.lastLane;
    mHaveJoin = mBeforeJoin.haveJoin;
    mJoinT    = mBeforeJoin.joinT;
    mLapEdgeT = mBeforeJoin.lapEdgeT;
    mJoinLat  = mBeforeJoin.joinLat;
    mJoinLon  = mBeforeJoin.joinLon;
}

void Session::undoJoin(Events& ev) noexcept
{
    // The road carries on as if the join had not happened.
    mSeg        = Segment::Road;
    mRoadBaseM  = mRoadBaseBefore;
    mRoadSegOdo = mRoadSegBefore;
    mTracker.reset();
    segmentChanged(ev, Segment::Track, Segment::Road, mJoinT);
    restoreBeforeJoin();
    mBlockOdo    = static_cast<float>(mOdo.m) + cfg().joinRetryM;
}

void Session::endVisit() noexcept
{
    // What this visit means for the store, merged with earlier visits to the
    // same track in the run.
    const Tracker& T = mTracker;
    if (!T.isRunning()) {
        return;
    }
    StoreOutcome o      = T.storeOutcome();
    const bool   locked = T.calibrator().stage() == Calibrator::Stage::Locked && T.calibrator().hasModel();
    uint8_t      number = mTrackNumber;
    if (mTrackNumber == 0) {
        // Not in the store: a locked fit is a new track, whatever model the
        // visit was given (one from earlier in the run).
        o = StoreOutcome();
        if (locked) {
            o.kind  = StoreOutcome::Kind::New;
            o.model = T.calibrator().model();
            o.model.setGeo(T.counter().frame());
            o.quality.scaleM = T.calibrator().lastFit().scaleM;
            o.quality.shapeM = T.calibrator().lastFit().shapeM;
            // Unless a stored track is this one after all (within
            // storeReplaceNearM, with the visit check's "clearly differs"
            // heading and R not reached): a run that dropped a neighbour
            // picked at Start, once laps were shown, is blended into the
            // stored track it was really on rather than saved twice.
            const TrackRecord* rec = mStore != nullptr ? mStore->findNear(o.model.centreLat(), o.model.centreLon()) : nullptr;
            if (rec != nullptr) {
                const TrackModel s = rec->model();
                float dh = std::fmod(std::fabs(o.model.headingRad() - s.headingRad()), 3.14159265f);
                if (dh > 0.5f * 3.14159265f) {
                    dh = 3.14159265f - dh;
                }
                if (distM(rec->lat, rec->lon, o.model.centreLat(), o.model.centreLon()) <= cfg().storeReplaceNearM
                    && dh * 57.2957795f <= cfg().visitReplaceHeadingDeg
                    && std::fabs(o.model.kerbRadiusM() - s.kerbRadiusM()) <= cfg().visitReplaceRM) {
                    o.kind = StoreOutcome::Kind::Updated;
                    number = rec->number;
                }
            }
        }
    }
    o.lane = T.lane();

    // The model for a rejoin: the one in use, lined up with today's GPS.
    TrackModel m;
    if (T.hasModel()) {
        const TrackModel& tm = T.model();
        m.set(tm.centre() + T.driftOffset(), tm.headingRad(), tm.kerbRadiusM(), tm.clockwise());
        m.setAltitudeM(tm.altitudeM());
        m.setGeo(T.counter().frame());
    }

    int idx = -1;
    for (uint8_t i = 0; i < kMaxVisited && idx < 0; ++i) {
        const Visited& v = mVisited[i];
        if (!v.used) {
            continue;
        }
        if (number != 0 ? v.number == number
                              : (v.number == 0 && m.hasGeo() && v.model.hasGeo()
                                 && distM(v.model.centreLat(), v.model.centreLon(), m.centreLat(), m.centreLon())
                                        <= cfg().storeReplaceNearM)) {
            idx = i;
        }
    }
    for (uint8_t i = 0; i < kMaxVisited && idx < 0; ++i) {
        if (!mVisited[i].used) {
            idx = i;
        }
    }
    if (idx < 0) {
        // All in use: the one with the fewest laps goes, if this has more.
        uint8_t least = 0;
        for (uint8_t i = 1; i < kMaxVisited; ++i) {
            if (mVisited[i].laps < mVisited[least].laps) {
                least = i;
            }
        }
        if (mVisited[least].laps >= T.lapsCompleted()) {
            return;
        }
        mVisited[least] = Visited();
        idx = least;
    }
    Visited& v = mVisited[idx];
    if (!v.used) {
        v.used   = true;
        v.number = number;
    }
    v.laps = static_cast<uint16_t>(v.laps + T.lapsCompleted());
    v.lane = T.lane();
    if (m.isValid()) {
        v.model = m;
    }
    if (outcomeRank(o.kind) >= outcomeRank(v.outcome.kind)) {
        v.outcome = o;
    }
    v.outcome.lane = T.lane();
}

uint8_t Session::visitedCount() const noexcept
{
    uint8_t n = 0;
    for (const Visited& v : mVisited) {
        n = static_cast<uint8_t>(n + (v.used ? 1 : 0));
    }
    return n;
}

StoreOutcome Session::storeOutcome(uint8_t i, uint8_t& number) const noexcept
{
    number = 0;
    for (const Visited& v : mVisited) {
        if (!v.used) {
            continue;
        }
        if (i == 0) {
            number = v.number;
            return v.outcome;
        }
        --i;
    }
    return StoreOutcome();
}

StoreOutcome Session::storeOutcome(uint8_t& number) const noexcept
{
    number = 0;
    const Visited* best = nullptr;
    for (const Visited& v : mVisited) {
        if (v.used && (best == nullptr || v.laps > best->laps)) {
            best = &v;
        }
    }
    if (best == nullptr) {
        return StoreOutcome();
    }
    number = best->number;
    return best->outcome;
}

// ---------------------------------------------------------------------------
// On the road: known tracks
// ---------------------------------------------------------------------------

void Session::roadStep(const Sample& s, bool added, Events& ev) noexcept
{
    if (!added || mPaused) {
        return;
    }
    if (mAwayFirst) {
        // "No lane" was set on a track: no join until the runner is away
        // from every known track, and none reaching back before that.
        TrackModel m;
        uint8_t    number = 0, lane = 0;
        int8_t     visited = -1;
        if (nearestKnown(s.lat, s.lon, cfg().storeNearM, m, number, visited, lane)) {
            return;
        }
        mAwayFirst = false;
        mStartSeq  = mRingSeq;
    }
    nearStep(s, added, ev);
}

bool Session::nearestKnown(double lat, double lon, float maxM, TrackModel& model, uint8_t& number,
                           int8_t& visited, uint8_t& lane) const noexcept
{
    // Tracks visited earlier in the run first (lined up with today's GPS),
    // then the store.
    bool  found = false;
    float best  = maxM;
    for (uint8_t i = 0; i < kMaxVisited; ++i) {
        const Visited& v = mVisited[i];
        if (!v.used || !v.model.isValid() || !v.model.hasGeo()) {
            continue;
        }
        const float d = distM(lat, lon, v.model.centreLat(), v.model.centreLon());
        if (d <= best) {
            best    = d;
            found   = true;
            model   = v.model;
            number  = v.number;
            visited = static_cast<int8_t>(i);
            lane    = v.lane;
        }
    }
    if (mStore == nullptr) {
        return found;
    }
    const TrackRecord* recs[TrackStore::kMaxTracks];
    const uint8_t n = mStore->list(recs);
    for (uint8_t k = 0; k < n; ++k) {
        const TrackRecord* r = recs[k];
        bool seen = false;
        for (const Visited& v : mVisited) {
            seen = seen || (v.used && v.number == r->number && v.model.isValid());
        }
        if (seen) {
            continue; // its lined-up model from earlier in the run is better
        }
        const float d = distM(lat, lon, r->lat, r->lon);
        if (d < best) {
            best    = d;
            found   = true;
            model   = r->model();
            number  = r->number;
            visited = -1;
            lane    = r->lane;
        }
    }
    return found;
}

void Session::nearStep(const Sample& s, bool added, Events& ev) noexcept
{
    // On each new point near a known track: has the runner been on it, and
    // moving along it, for joinConfirmM of path?
    const Config& c = cfg();
    if (static_cast<float>(mOdo.m) < mBlockOdo) {
        return;
    }
    TrackModel m;
    uint8_t    number = 0, lane = 0;
    int8_t     visited = -1;
    if (!nearestKnown(s.lat, s.lon, c.storeNearM, m, number, visited, lane) || !m.placeIn(mFrame)) {
        return;
    }
    // Inside the kerb the band is wider: paths beside a track run outside it,
    // and a day's offset inward should not hold up a join in lanes 1 and 2.
    const float    lo     = -c.joinMarginM - kJoinInnerM;
    const float    hi     = mTable.lineOffsetM(mTable.laneCount()) + 1.02f + c.joinMarginM; // outer edge
    const float    lapK   = m.lapLengthM(0.0f);
    const uint32_t oldest = mRingSeq - mRingCount;
    const uint32_t lowest = mStartSeq > oldest ? mStartSeq : oldest;
    const uint32_t newest = mRingSeq - 1u;
    if (newest <= lowest) {
        return;
    }
    float    lats[kJoinScan];
    uint16_t n = 0, nIn = 0;
    float    path = 0.0f, along = 0.0f;
    Vec2     pNew = pointAt(newest);
    float    sNew = m.project(pNew, 0.0f).s;
    uint32_t q    = newest;
    for (;;) {
        const Vec2 pq = pointAt(q);
        const TrackModel::Projection pr = m.project(pq, 0.0f);
        lats[n++] = pr.lateralM;
        nIn = static_cast<uint16_t>(nIn + ((pr.lateralM >= lo && pr.lateralM <= hi) ? 1 : 0));
        if (q != newest) {
            path += length(pNew - pq);
            float ds = sNew - pr.s;
            ds -= std::floor(ds / lapK + 0.5f) * lapK;
            along += ds;
        }
        pNew = pq;
        sNew = pr.s;
        if (path >= c.joinConfirmM || n >= kJoinScan || q <= lowest) {
            break;
        }
        --q;
    }
    if (path < c.joinConfirmM) {
        return;
    }
    // The median offset in the band, most readings in it, and moving along.
    for (uint16_t i = 1; i < n; ++i) {
        const float v = lats[i];
        uint16_t    j = i;
        for (; j > 0 && lats[j - 1] > v; --j) {
            lats[j] = lats[j - 1];
        }
        lats[j] = v;
    }
    const float med = lats[n / 2];
    const int32_t dtMs = msDiff(timeAt(newest), timeAt(q));
    if (med < lo || med > hi || 4u * nIn < 3u * n || std::fabs(along) < c.joinAlongFrac * path || dtMs <= 0
        || path / (0.001f * static_cast<float>(dtMs)) < c.stillMps) {
        return;
    }
    // Where they came onto it: back from the newest point while on the
    // runner's own line (the median offset of the stretch), within the
    // spread the readings showed there; three stray readings in a row, or a
    // gap in them, end it. The band only says whether the runner is on the
    // track: a runner's line near its edge (lane 8 with a day's offset
    // outward) still reaches back to the entry. A slanting approach is road:
    // only its run along the track would count on the track.
    for (uint16_t i = 0; i < n; ++i) {
        lats[i] = std::fabs(lats[i] - med);
    }
    for (uint16_t i = 1; i < n; ++i) {
        const float v = lats[i];
        uint16_t    j = i;
        for (; j > 0 && lats[j - 1] > v; --j) {
            lats[j] = lats[j - 1];
        }
        lats[j] = v;
    }
    const float spread = 1.4826f * lats[n / 2]; // robust noise of the stretch
    const float width  = std::fmax(kJoinLineM, kJoinSpread * spread);
    const float onHi   = med + width;
    const float onLo   = med - width;
    uint32_t    join = newest;
    uint8_t     outs = 0;
    for (uint32_t k = newest; k > lowest && newest - k < static_cast<uint32_t>(kJoinScan + kJoinExtend);) {
        if (msDiff(timeAt(k), timeAt(k - 1u)) > kJoinGapMs) {
            break;
        }
        --k;
        const float lat = m.project(pointAt(k), 0.0f).lateralM;
        if (lat >= onLo && lat <= onHi) {
            join = k;
            outs = 0;
        } else if (++outs >= 3) {
            break;
        }
    }
    // When and where they reached their line, from the readings
    // either side of the join point.
    mHaveEntry = entryAt(join, lowest, m, med, mEntry);
    if (along < 0.0f) {
        m = m.reversed(); // run the other way round from the way it was stored
    }
    joinKnown(join, m, number, lane, s, added, ev);
}

void Session::joinKnown(uint32_t seq, const TrackModel& model, uint8_t number, uint8_t lane, const Sample& s,
                        bool added, Events& ev) noexcept
{
    // With an entry (set by the caller), the segment starts there.
    const uint8_t count0 = ev.count;
    const Entry*  e      = joinEntry();
    const float   odoJ   = e != nullptr ? e->odo : odoAt(seq);
    const Vec2    pJ     = e != nullptr ? e->p : pointAt(seq);
    keepBeforeJoin();
    mRoadBaseBefore = mRoadBaseM;
    mRoadSegBefore  = mRoadSegOdo;
    mRoadBaseM     += static_cast<double>(odoJ - mRoadSegOdo);
    mJoinSeq        = e != nullptr ? e->next : seq;
    mJoinT          = e != nullptr ? e->tMs : timeAt(seq);
    mLapEdgeT       = mJoinT;
    mHaveJoin       = true;
    mFrame.toLatLon(pJ, mJoinLat, mJoinLon);
    mJoinModel      = model;
    mJoinHasModel   = true;
    mJoinNumber     = number;
    // The lane last run there; for a stored track with none recorded, the
    // last lane run in this run.
    const uint8_t useLane = mTable.isValidLane(lane) ? lane : mLastLane;

    mTrackNumber = number;
    mSeg         = Segment::Track;
    mLane        = useLane;
    mLastLane    = useLane;
    beginVisit(Join::Auto, mJoinT, pJ, odoJ);
    segmentChanged(ev, Segment::Road, Segment::Track, mJoinT);
    addEvent(ev, EventType::TrackJoined, s.tMs);
    if (ev.count > 0 && ev.items[ev.count - 1].type == EventType::TrackJoined) {
        ev.items[ev.count - 1].lane = useLane;
    }
    replay(mJoinSeq, useLane, &mJoinModel, &s, added, &ev, e);
    if (mTracker.alignment().abandoned) {
        cancelJoin(ev, count0);
    } else {
        ++mAutoJoins;
    }
}

void Session::cancelJoin(Events& ev, uint8_t count0) noexcept
{
    // Lining the track up with today's GPS, over the readings replayed, found
    // it is not this one (a wrong pick, or a path beside it) before anything
    // was shown: the join never happened. The road carries on.
    ev.count     = count0;
    mSeg         = Segment::Road;
    mRoadBaseM   = mRoadBaseBefore;
    mRoadSegOdo  = mRoadSegBefore;
    restoreBeforeJoin();
    mTracker.reset();
    mBlockOdo    = static_cast<float>(mOdo.m) + cfg().joinRetryM;
}

} // namespace Oval
