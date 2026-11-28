/*******************************************************************************
 * @file   TrackSource.hpp
 * @brief  Run: the run's distance, live speed and altitude, from the runner's
 *         lane on a track (Oval::Session) or GPS distance on the road.
 *
 *         The service keeps Run's model (laps, workouts, alerts, the FIT
 *         file) and takes its distance from here, where Run takes the
 *         kernel's GPS distance:
 *           - totalM(): the Session's total, which never decreases;
 *           - speedMps(): the total over a rolling window (Run::PaceWindow),
 *             with a correction (a join replayed, a lane set) taken out so it
 *             never shows as a burst of pace;
 *           - altitude(): held on a track at the altitude where the runner
 *             joined it, with any climb since a join found late backed out
 *             of ascentM() and descentM();
 *           - lanePosition(): the record position on the lane line;
 *           - status(): the segment, track, lane and calibration, for the GUI.
 *
 *         All times are on the sensor clock (Run::SensorClock), in ms.
 *
 *         Plain C++, no SDK headers, no heap.
 ******************************************************************************/

#ifndef RUN_TRACK_SOURCE_HPP
#define RUN_TRACK_SOURCE_HPP

#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/Sample.hpp"
#include "Oval/Session.hpp"
#include "Oval/TrackStore.hpp"
#include "Run/PaceWindow.hpp"
#include "Run/TickLog.hpp"
#include "Run/WeakGps.hpp"

#include <cstdint>

namespace Run {

/** Where the run is on a track, for the GUI. */
enum class CalState : uint8_t {
    None,        ///< Not on a track
    Calibrating, ///< A track not stored: counting while calibration collects
    Provisional, ///< A working fit, still being refined
    Locked,      ///< Calibration locked; stored at the end
    NotStandard, ///< Not a standard track: counting only
    Stored,      ///< A stored track
};

struct TrackStatus {
    Oval::Session::Segment segment = Oval::Session::Segment::None;
    uint8_t  trackNumber = 0;     ///< A stored track's number; 0 if not stored
    uint8_t  lane        = 0;     ///< Oval::Session::kNoLane on the road
    CalState cal         = CalState::None;
    uint8_t  calLaps     = 0;     ///< Calibrating: laps covered so far
    uint16_t autoJoins   = 0;     ///< Known tracks the run joined by itself
    bool     weakGps     = false;
};

/** The start screen's track: the stored track the runner is at, if any. */
struct PreRun {
    enum class Status : uint8_t { Unknown, New, Stored };
    Status   status      = Status::Unknown;  ///< Unknown: no fix yet
    uint8_t  trackNumber = 0;
    uint16_t uses        = 0;   ///< Stored: runs that used it
};

/** The barometer, each active tick. */
struct Baro {
    bool  valid    = false;
    float altM     = 0.0f;  ///< Filtered altitude
    float ascentM  = 0.0f;  ///< Ascent and descent so far
    float descentM = 0.0f;
};

class TrackSource {
public:
    explicit TrackSource(const Oval::Config& cfg = Oval::Config()) noexcept;

    // -- Before Start ---------------------------------------------------------
    void   prime(const Oval::Sample& s) noexcept { mSession.prime(s); }
    PreRun preRun(const Oval::TrackStore* store) const noexcept;

    // -- The run --------------------------------------------------------------
    /** Start at tMs in @p lane (Oval::Session::kNoLane: the road). store
     *  must outlive the run. */
    bool start(uint32_t tMs, uint8_t lane, const Oval::LaneTable& table, const Oval::TrackStore* store) noexcept;

    /** Each GPS reading, in time order. */
    void reading(const Oval::Sample& s) noexcept;

    /** Each active 1 Hz tick (not while paused). */
    void tick(uint32_t nowMs, const Baro& baro) noexcept;

    void pause(uint32_t tMs) noexcept;
    void resume(uint32_t tMs) noexcept;

    /** The runner's lane from tMs: a lane or kNoLane. False if not taken, or
     *  with no run. */
    bool setLane(uint8_t lane, uint32_t tMs) noexcept;

    /** Stop. The Session's store outcomes are then ready. */
    void finish() noexcept;

    // -- Outputs --------------------------------------------------------------
    bool        running() const noexcept { return mRunning; }
    float       totalM() const noexcept { return mSession.totalDistanceM(); }
    float       speedMps() const noexcept { return mPace.speedMps(); }
    bool        altitude(float& altM) const noexcept;
    float       ascentM() const noexcept { return mEffAsc; }
    float       descentM() const noexcept { return mEffDesc; }
    bool        lanePosition(double& lat, double& lon) const noexcept;
    TrackStatus status(uint32_t nowMs) const noexcept;

    /** For the GPS trace. */
    const char* traceMode() const noexcept;
    const char* traceSegment() const noexcept;

    const Oval::Session& session() const noexcept { return mSession; }

private:
    void elevation(const Baro& baro) noexcept;

    Oval::Session mSession;
    PaceWindow    mPace;
    WeakGps       mWeak;
    TickLog       mTicks;

    bool     mRunning     = false;
    bool     mPaused      = false;
    bool     mHaveReading = false;
    uint32_t mLastT       = 0;
    bool     mCorrection  = false; ///< A lane set: the next rise is a jump, not pace

    // Elevation
    bool  mHeld         = false;
    bool  mHeldValid    = false;
    float mHeldAlt      = 0.0f;
    bool  mHaveBaro     = false;
    float mBaroAlt      = 0.0f;
    float mLastBaroAsc  = 0.0f;
    float mLastBaroDesc = 0.0f;
    float mEffAsc       = 0.0f;
    float mEffDesc      = 0.0f;
};

} // namespace Run

#endif // RUN_TRACK_SOURCE_HPP
