/*******************************************************************************
 * @file   Session.hpp
 * @brief  Oval: road and track in one run. The one class the service uses.
 *
 *         A run is a sequence of segments, each Road or Track:
 *           Road   normal GPS distance (the kernel's GPS_DISTANCE deltas).
 *           Track  what Oval::Tracker measures: laps counted from where the
 *                  runner joined the track, calibrated on the lane line.
 *         The total is road plus track, never decreases and never counts a
 *         stretch twice.
 *
 *         The runner sets the lane: a lane number, or "No lane" (kNoLane).
 *         No lane is the road. A lane is a track: given at Start, or set on
 *         the road (setLane()), the run is on a track from there, the known
 *         track there if there is one, else a track that is not stored,
 *         learnt from there. Laps count from where the runner was.
 *
 *         Two things the Session notices by itself. On the road, a known
 *         track (a stored one, or one visited earlier in the run): near it
 *         and running along it for a short stretch joins it where the runner
 *         came onto it, on the lane last run there (TrackJoined). And on a
 *         track, leaving it (the Tracker's off-track check, confirmed): the
 *         road from where the runner left, with no lane. It never takes a
 *         loop that is not known for a track, and never changes the lane.
 *
 *         A join it notices is replayed: a fresh Tracker started at the join
 *         point, then the readings since from a ring of recent points, so
 *         laps count from the join and every counting, calibration and
 *         stored-track rule applies unchanged.
 *
 *         Calls:
 *           pre-run screen:  prime(sample), primedPosition()
 *           Start:           start(tMs, lane, table, &store)
 *           each reading:    events = update(sample)
 *           pause menu:      pause(tMs), resume(tMs)
 *           the lane:        events = setLane(lane, tMs)
 *           Lap button:      events = lapPress(tMs)
 *           Stop:            events = finish(); storeOutcome(...)
 *
 *         No heap, no exceptions, no SDK headers. Not thread-safe.
 ******************************************************************************/

#ifndef OVAL_SESSION_HPP
#define OVAL_SESSION_HPP

#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/LocalFrame.hpp"
#include "Oval/PauseLog.hpp"
#include "Oval/Sample.hpp"
#include "Oval/TrackModel.hpp"
#include "Oval/TrackStore.hpp"
#include "Oval/Tracker.hpp"

#include <cstdint>

namespace Oval {

class Session {
public:
    enum class Segment : uint8_t {
        None,  ///< Before Start
        Road,  ///< No lane: GPS distance
        Track, ///< On a track: the Tracker measures
    };

    /** The lane "No lane": the road. */
    static constexpr uint8_t kNoLane = 0;

    enum class EventType : uint8_t {
        TrackLap,       ///< track: a Tracker Lap, MissedLap or LapTimeRefined
        TrackEvent,     ///< track: another Tracker event (calibration, mode)
        TrackJoined,    ///< A known track noticed on the road and joined: trackNumber (0: not stored), lane
        SegmentChanged, ///< from to segment, at tMs (which can be in the past)
        RoadKilometre,  ///< kilometre: another whole roadSplitM in the road segment, at tMs
    };

    struct Event {
        EventType      type        = EventType::TrackLap;
        Segment        segment     = Segment::None; ///< SegmentChanged: the new one
        Segment        from        = Segment::None; ///< SegmentChanged: the old one
        uint8_t        trackNumber = 0;
        uint8_t        lane        = 0;    ///< TrackJoined: the lane it was joined on
        uint16_t       kilometre   = 0;    ///< RoadKilometre: 1-based, in the segment
        uint16_t       runLap      = 0;    ///< TrackLap: the lap's number in the whole run
        uint32_t       tMs         = 0;    ///< When it happened (sensor clock)
        float          totalM      = 0.0f; ///< Road plus credited track distance then
        Tracker::Event track;              ///< TrackLap, TrackEvent: as the Tracker raised it
    };

    static constexpr uint8_t kMaxEvents = 16;

    struct Events {
        uint8_t count = 0;
        Event   items[kMaxEvents];
    };

    /** Recent points, one every ringSpacingM of GPS distance: about 1.13 km.
     *  A join noticed reaches back a few hundred metres at most; a stored
     *  track dropped before lap 1 is replayed on another from the start of
     *  the visit (restartOther), up to two laps of lane 8 with GPS distance
     *  12% over. */
    static constexpr uint16_t kRingPoints = 376;

    /** Tracks visited in one run that are remembered (store outcome, rejoin). */
    static constexpr uint8_t kMaxVisited = 2;

    explicit Session(const Config& cfg = Config()) noexcept;

    void          setConfig(const Config& cfg) noexcept;
    const Config& config() const noexcept { return mTracker.config(); }
    void          reset() noexcept;
    /** Road splits every m (1000, or a mile on an imperial
     *  watch). Set before start(); no reset. */
    void          setRoadSplitM(float m) noexcept { mTracker.setRoadSplitM(m); }

    void prime(const Sample& s) noexcept { mTracker.prime(s); }
    bool primedPosition(double& lat, double& lon) const noexcept { return mTracker.primedPosition(lat, lon); }

    /** Start. lane kNoLane: on the road from Start. A lane:
     *  on a track from Start, the stored track near the primed position
     *  (store->findNear) if there is one, else a track that is not stored,
     *  learnt from Start. store may be null. The store must outlive the run:
     *  it is also searched for tracks met on the way. */
    bool start(uint32_t tMs, uint8_t lane, const LaneTable& table, const TrackStore* store = nullptr) noexcept;

    Events update(const Sample& s) noexcept;
    Events finish() noexcept;

    void pause(uint32_t tMs) noexcept;
    void resume(uint32_t tMs) noexcept;

    /** The runner's lane, set at tMs (sensor clock; while paused, at the
     *  pause). On a track a lane is a lane change from here, and kNoLane
     *  leaves the track here (or where the runner went off it, if they
     *  have): the road from there, and no track is joined by itself until
     *  the runner has been away from every known track. kNoLane before the
     *  next reading after a lane joined a track leaves no visit at all. On
     *  the road a lane joins a track here: the known track nearest within
     *  storeNearM, else a track that is not stored, learnt from here; with
     *  no good reading in the last few seconds, at the next good reading
     *  (kNoLane before then cancels it). ok, if given, is false for a lane
     *  not in the table. Returns the events of the change. */
    Events setLane(uint8_t lane, uint32_t tMs, bool* ok = nullptr) noexcept;

    /** The Lap button. On a track the lap starts again from here; on
     *  the road the next split is a whole roadSplitM from here. */
    Events lapPress(uint32_t tMs) noexcept;

    // State ------------------------------------------------------------------

    bool    isRunning() const noexcept { return mRunning; }
    bool    isPaused() const noexcept { return mPaused; }
    Segment segment() const noexcept { return mSeg; }

    /** The stored track's number in this Track segment (or the last one), 0
     *  for a track not stored. */
    uint8_t trackNumber() const noexcept { return mTrackNumber; }
    /** The lane; kNoLane on the road. */
    uint8_t lane() const noexcept { return mLane; }
    /** Joins the Session noticed by itself (TrackJoined) so far in the run. */
    uint16_t autoJoins() const noexcept { return mAutoJoins; }

    /** Never decreases. */
    float totalDistanceM() const noexcept { return mShownM; }
    float roadDistanceM() const noexcept;
    /** On the road, the share of the current road split run so far, 0 to 1
     *  (the run screen's ring); 0 elsewhere. */
    float roadSplitShare() const noexcept;
    /** Road splits completed in this road segment, or since a Lap press on
     *  the road (as the RoadKilometre events number them). */
    uint16_t roadSplitsDone() const noexcept { return mKmCount; }
    float trackDistanceM() const noexcept;

    /** Laps in the current Track segment (since the join), and in the run. */
    uint16_t segmentLaps() const noexcept;
    uint16_t runLaps() const noexcept { return static_cast<uint16_t>(mLapsBase + segmentLaps()); }

    /** Hold the elevation: on a track that has a model (its altitude), not
     *  on the road, where the barometer applies. */
    bool holdElevation() const noexcept;

    /** Positions for the FIT file placed on the lane line: only on a
     *  track in calibrated mode, on the track. Road keeps raw GPS. */
    bool lanePositions() const noexcept;

    /** The latest reading on the runner's lane line, for the FIT
     *  records while lanePositions() holds. */
    bool lanePosition(double& lat, double& lon) const noexcept;

    /** When the total reached totalM, on a track (Tracker::timeOfDistance);
     *  false on the road, where the service interpolates the total itself.
     *  For rep endings and distance splits mid-lap. */
    bool timeOfDistance(float totalM, uint32_t& tMs) const noexcept;

    /** The run's total where the current segment began: road and track
     *  segments closed before it. The total at the latest SegmentChanged. */
    float segmentStartTotalM() const noexcept { return static_cast<float>(mRoadBaseM + mTrackBaseM); }

    /** No event raised from now on is timed before this (sensor clock): the
     *  start of the visit while a stored track is still being lined up, and
     *  otherwise the oldest point a join, exit or credited lap can reach
     *  back to. Splits older than this are settled. */
    uint32_t earliestRetimeTMs() const noexcept;

    /** The latest join: where and when. False before any. */
    bool     joinPoint(double& lat, double& lon) const noexcept;
    uint32_t joinTMs() const noexcept { return mJoinT; }

    /** The tracks visited, for TrackStore::apply(outcome, number, now).
     *  Call after finish(). storeOutcome(number) is the one with the most
     *  laps. */
    uint8_t      visitedCount() const noexcept;
    /** Points in the ring now (for the buffer check). */
    uint16_t     ringCount() const noexcept { return mRingCount; }
    StoreOutcome storeOutcome(uint8_t i, uint8_t& number) const noexcept;
    StoreOutcome storeOutcome(uint8_t& number) const noexcept;

    /** The largest amount the shown total has held above the measured one. */
    float largestHoldM() const noexcept { return mLargestHoldM; }

    const Tracker& tracker() const noexcept { return mTracker; }

private:
    /** A recent point: decimetres in the Session's frame, its time, the GPS
     *  distance since the previous point in decimetres and HDOP in
     *  twentieths (0: none). The time is whole: points are added only while
     *  moving, so two neighbours can be hours apart. */
    struct RingPoint {
        int16_t  x;
        int16_t  y;
        uint32_t tMs;
        uint16_t dOdo10;
        uint8_t  hdop20;
    };

    /** The kernel's GPS distance, as counting mode reads it: deltas, cut to
     *  maxSpeedMps, the step between positions when there is none, nothing
     *  while paused. */
    struct Odometer {
        double   m          = 0.0;
        bool     haveGps    = false;
        float    lastGps    = 0.0f;
        uint32_t lastGpsT   = 0;
        bool     haveStep   = false;
        Vec2     stepP;
        uint32_t stepT      = 0;
        bool     haveT      = false;
        uint32_t lastT      = 0;   ///< The last reading's time
    };

    struct Visited {
        bool         used   = false;
        uint8_t      number = 0;  ///< In the store, 0 if not
        uint8_t      lane   = 0;  ///< Last lane run there
        uint16_t     laps   = 0;
        TrackModel   model;       ///< Lined up with today's GPS, for a rejoin
        StoreOutcome outcome;
    };

    /** How the current or last visit began. */
    enum class Join : uint8_t {
        Start,  ///< At Start, with a lane
        Runner, ///< The runner set a lane on the road
        Auto,   ///< A known track noticed on the road (it can be undone)
    };

    /** Where and when the runner came onto their line at a join,
     *  estimated from the readings either side (entryAt). A join's Tracker
     *  starts there, then replays the ring from point next. */
    struct Entry {
        uint32_t tMs    = 0;
        Vec2     p;            ///< Session frame
        float    odo    = 0.0f; ///< The Session's odometer then
        uint32_t next   = 0;   ///< The first ring point after tMs
        uint8_t  hdop20 = 0;
    };

    const Config& cfg() const noexcept { return mTracker.config(); }
    void clearRun() noexcept;
    bool isGood(const Sample& s) const noexcept;
    void addEvent(Events& ev, EventType type, uint32_t tMs) noexcept;
    void segmentChanged(Events& ev, Segment from, Segment to, uint32_t tMs) noexcept;
    void forward(const Tracker::Events& tev, Events& ev) noexcept;
    void updateShown() noexcept;
    /** A Tracker started at tMs where the runner is (the latest good
     *  reading), on model if given. */
    bool startTracker(uint32_t tMs, uint8_t lane, const TrackModel* model) noexcept;

    // Odometer, frame and ring
    void     odometer(const Sample& s, bool good, Vec2 p) noexcept;
    void     recentre(double lat, double lon) noexcept;
    bool     addPoint(const Sample& s, Vec2 p) noexcept;
    bool     inRing(uint32_t seq) const noexcept;
    Vec2     pointAt(uint32_t seq) const noexcept;
    uint32_t timeAt(uint32_t seq) const noexcept;
    float    odoAt(uint32_t seq) const noexcept;
    float    odoAtTime(uint32_t tMs) const noexcept;
    uint32_t timeAtOdo(float odo, uint32_t prevT, float prevOdo) const noexcept;
    Sample   sampleAt(uint32_t seq, float odo, float gpsNow) const noexcept;

    /** Starts the Tracker at ring point seq (or at entry, then from ring
     *  point seq = entry->next) and replays to the newest point; then the
     *  live reading, if it is not that point. */
    void replay(uint32_t seq, uint8_t lane, const TrackModel* model, const Sample* live, bool addedLive,
                Events* ev, const Entry* entry = nullptr) noexcept;
    bool entryAt(uint32_t join, uint32_t lowest, const TrackModel& m, float c, Entry& out) const noexcept;

    // Segments
    void joinHere(uint8_t lane, uint32_t tMs, Events& ev) noexcept;
    void beginVisit(Join kind, uint32_t tMs, Vec2 p, float odo) noexcept;
    void roadStep(const Sample& s, bool added, Events& ev) noexcept;
    void trackStep(const Sample& s, bool good, Vec2 p, bool added, Events& ev) noexcept;
    void roadKilometres(uint32_t prevT, float prevOdo, Events& ev) noexcept;
    void leaveTrack(uint32_t exitT, float exitOdo, float trackM, Events& ev) noexcept;
    void undoJoin(Events& ev) noexcept;
    bool restartOther(const Sample& s, bool added, Events& ev) noexcept;
    /** The runner paused at tMs (from the acts kept). */
    bool pausedAt(uint32_t tMs) const noexcept;
    void cancelJoin(Events& ev, uint8_t count0) noexcept;
    void endVisit() noexcept;
    void toRoad(uint32_t exitT, float exitOdo) noexcept;

    // Known tracks
    bool nearestKnown(double lat, double lon, float maxM, TrackModel& model, uint8_t& number,
                      int8_t& visited, uint8_t& lane) const noexcept;
    void nearStep(const Sample& s, bool added, Events& ev) noexcept;
    void joinKnown(uint32_t seq, const TrackModel& model, uint8_t number, uint8_t lane, const Sample& s,
                   bool added, Events& ev) noexcept;
    const Entry* joinEntry() const noexcept { return mHaveEntry ? &mEntry : nullptr; }

    Tracker           mTracker;
    LaneTable         mTable;
    const TrackStore* mStore = nullptr;
    LocalFrame        mFrame;  ///< The Session's frame: re-centred as the run goes on
    Odometer          mOdo;

    // Run and clock
    bool     mRunning = false;
    bool     mHaveT   = false;
    bool     mPaused  = false;
    PauseLog mPauses;              ///< Every recent pause, for a step across more than one
    bool     mFinished = false;
    uint32_t mStartT  = 0;
    uint32_t mLastT   = 0;
    uint8_t  mLane    = kNoLane;
    uint8_t  mLastLane = 1;        ///< The last lane run in this run (a stored track with none recorded)
    Segment  mSeg     = Segment::None;
    bool     mHaveGood = false;    ///< mGood holds the latest good reading
    Sample   mGood;

    // Distances
    double   mRoadBaseM  = 0.0; ///< Road segments closed
    double   mTrackBaseM = 0.0; ///< Track segments closed
    float    mRoadSegOdo = 0.0f; ///< Odometer where this road segment began
    uint16_t mLapsBase   = 0;
    float    mShownM     = 0.0f;
    float    mLargestHoldM = 0.0f;
    float    mKmFromM    = 0.0f; ///< Road splits count from here in the segment (a Lap press)
    uint16_t mKmCount    = 0;

    // The current or last Track segment
    uint8_t  mTrackNumber = 0;
    Join     mJoinKind    = Join::Start;
    uint16_t mAutoJoins   = 0;
    bool     mAbandonSeen = false; ///< The Tracker dropping a stored model has been handled
    bool     mRestarted   = false; ///< Started again on another stored track, once
    uint32_t mJoinSeq     = 0;     ///< Ring point of the join
    uint32_t mJoinT       = 0;
    bool     mHaveJoin    = false;
    double   mJoinLat     = 0.0;
    double   mJoinLon     = 0.0;
    Vec2     mSegStartP;           ///< Where the track segment began (Session frame)
    double   mRoadBaseBefore = 0.0; ///< To undo a join: road as it was
    /** The rest of what a join changes, as it was before: put back when the
     *  join is cancelled or undone. */
    struct BeforeJoin {
        uint8_t  trackNumber = 0;
        uint8_t  lane     = 1;
        uint8_t  lastLane = 1;
        bool     haveJoin = false;
        uint32_t joinT    = 0;
        uint32_t lapEdgeT = 0;
        double   joinLat  = 0.0;
        double   joinLon  = 0.0;
    };
    BeforeJoin mBeforeJoin;
    void       keepBeforeJoin() noexcept;
    void       restoreBeforeJoin() noexcept;
    float    mRoadSegBefore  = 0.0f;
    uint8_t  mFarRun      = 0;
    uint32_t mLastOnT     = 0;     ///< The last reading on the track (within offLanesM of the lanes)
    uint32_t mAreaT       = 0;     ///< and the last reading there
    uint32_t mLapEdgeT    = 0;     ///< Bookkeeping only: the latest lap end, Lap press or segment start (earliestRetimeTMs)
    float    mAreaOdo     = 0.0f;
    float    mAreaTrackM  = 0.0f;

    // The track joined
    TrackModel mJoinModel;       ///< The known track joined
    bool     mJoinHasModel = false;
    uint8_t  mJoinNumber   = 0;
    Entry    mEntry;             ///< The join's entry, if found
    bool     mHaveEntry    = false;

    uint8_t  mFarFixes   = 0;      ///< Readings in a row beyond the recentre distance

    /** The runner's recent pauses, resumes, Lap presses and lane changes on
     *  a track, so a replay (a join noticed, a restart on another stored
     *  track) gives its new Tracker them at their own times: paused time
     *  stays out of its laps, a lap restarted stays restarted, and each
     *  stretch is measured on its own lane. */
    struct Act {
        uint32_t tMs  = 0;
        uint8_t  kind = 0; ///< kActPause, kActResume, kActLap or kActLane
        uint8_t  lane = 0; ///< kActLane: the lane set
        uint8_t  from = 0; ///< kActLane: the lane before it
    };
    static constexpr uint8_t kActPause = 0, kActResume = 1, kActLap = 2, kActLane = 3;
    static constexpr uint8_t kActs = 32;
    Act      mActs[kActs];
    uint8_t  mActHead   = 0;
    uint8_t  mActCount  = 0;
    bool     mActsPaused = false; ///< Paused after the oldest act given up
    uint32_t mPauseFromT = 0;      ///< Where the pause in force began
    void addAct(uint32_t tMs, uint8_t kind, uint8_t lane = 0, uint8_t from = 0) noexcept;
    /** The lane at tMs in this visit: before the first lane change after
     *  it, from the acts; else the lane now. */
    uint8_t laneAt(uint32_t tMs) const noexcept;
    /** The acts after afterT, up to and at toT, given to the Tracker. */
    void replayActs(uint32_t afterT, uint32_t toT, Events* ev) noexcept;

    // Watching for known tracks on the road
    uint32_t mStartSeq    = 0;     ///< No join reaches back before this ring point
    float    mBlockOdo    = 0.0f;  ///< No join before this odometer (after an undo)
    bool     mAwayFirst   = false; ///< "No lane" on a track: no join until away from known tracks
    uint8_t  mPendingLane = kNoLane; ///< A lane set on the road with no recent good reading: joined at the next

    Visited mVisited[kMaxVisited];

    // Ring of recent points
    RingPoint mRing[kRingPoints];
    uint32_t  mRingSeq   = 0;    ///< Points ever added: the next point's number
    uint16_t  mRingCount = 0;
    uint32_t  mRingT     = 0;    ///< Exact time of the newest point
    float     mRingOdo   = 0.0f; ///< Odometer at the newest point, as the ring holds it
};

} // namespace Oval

#endif // OVAL_SESSION_HPP
