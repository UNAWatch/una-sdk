// Runs a trace through Oval::LapCounter the way the service will: prime() on
// the pre-run readings, start() at Start, then update() on every reading with
// pause, resume and lane changes applied in between, and finish() at the end.
// Records every event and checks after every reading that the total never
// decreased.
//
// Host only.
#pragma once

#include "TraceGen.hpp"

#include "Oval/Config.hpp"
#include "Oval/LaneTable.hpp"
#include "Oval/LapCounter.hpp"
#include "Oval/TrackModel.hpp"
#include "Oval/Tracker.hpp"
#include "Oval/Session.hpp"
#include "Oval/TrackStore.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace Replay {

struct Result {
    std::vector<Oval::LapCounter::Event> events;
    std::vector<size_t> eventSample;     // the reading whose update() raised each event
    std::vector<float>  total;           // totalDistanceM() after each reading (0 before Start)
    size_t   decreases     = 0;          // readings where the total went down
    float    worstDecrease = 0.0f;
    uint16_t laps          = 0;
    uint16_t missed        = 0;
    float    finalTotal    = 0.0f;
    float    gpsRatio      = 1.0f;
};

Result run(const TraceGen::Trace& tr,
           const Oval::LaneTable& table = Oval::LaneTable::outdoor(),
           const Oval::Config& cfg = Oval::Config());

// How the counted laps compare with the truth.
struct Match {
    size_t trueLaps      = 0;
    size_t countedLaps   = 0;
    size_t realLaps      = 0;    // ended on a real crossing
    size_t missedLaps    = 0;    // credited
    size_t badTimes      = 0;    // laps whose end is further than the tolerance from the truth
    double worstEndErrMs = 0.0;  // worst |lap end - true pass|, real crossings only
    // Lap duration errors (ms, signed), for laps that start and end on real
    // crossings, or on Start and a real crossing.
    std::vector<double> lapTimeErrMs;
};

Match match(const TraceGen::Trace& tr, const Result& r, uint32_t toleranceMs);

// Expected credited distance after n laps of the truth.
double creditedAfter(const TraceGen::Trace& tr, size_t n);

// ---------------------------------------------------------------------------
// Oval::Tracker
// ---------------------------------------------------------------------------

struct TrackerResult {
    std::vector<Oval::Tracker::Event> events;      // every event, in order
    std::vector<size_t>               eventSample; // the reading that raised each

    // Laps (Lap and MissedLap), with the refined end and duration when a
    // LapTimeRefined event came for them.
    std::vector<Oval::Tracker::Event> laps;
    std::vector<size_t>               lapSample;
    std::vector<bool>                 refined;
    std::vector<uint32_t>             refinedEndMs;
    std::vector<uint32_t>             refinedDurMs;
    std::vector<size_t>               refinedSample;

    std::vector<float>   total;    // totalDistanceM() after each reading (0 before Start)
    std::vector<uint8_t> mode;     // mode() after each reading
    std::vector<float>   lateral;  // lateralOffsetM() after each reading (NaN if none)
    size_t decreases     = 0;
    float  worstDecrease = 0.0f;
    float  finalTotal    = 0.0f;
    float  largestHold   = 0.0f;

    Oval::Tracker::Mode     finalMode  = Oval::Tracker::Mode::Counting;
    Oval::Tracker::CalStage finalStage = Oval::Tracker::CalStage::None;
    size_t provisionalAt = SIZE_MAX;  // reading of each calibration event
    size_t lockedAt      = SIZE_MAX;
    size_t rejectedAt    = SIZE_MAX;
    size_t calibratedAt  = SIZE_MAX;  // first switch to calibrated mode
    Oval::TrackModel provisionalModel;
    Oval::TrackModel lockedModel;
    Oval::LocalFrame frame;
    uint16_t missed = 0;

    // Stored tracks
    Oval::Tracker::Alignment          alignment;      // at the end of the run
    Oval::Tracker::VisitCheck         visit;
    Oval::StoreOutcome                outcome;        // storeOutcome() after finish()
    size_t                            alignedAt = SIZE_MAX; // first reading after alignment ended
};

// Runs a trace through a Tracker as the service will. hook, if given, is
// called after each reading's update() with the reading's index.
TrackerResult runTracker(const TraceGen::Trace& tr,
                         const Oval::LaneTable& table = Oval::LaneTable::outdoor(),
                         const Oval::Config& cfg = Oval::Config(),
                         const Oval::TrackModel* supplied = nullptr,
                         const std::function<void(size_t, Oval::Tracker&)>& hook = nullptr);

// The true track as a stored model, for "starting anywhere" tests.
Oval::TrackModel truthModel(const TraceGen::Track& track);

// A stored model moved east and north by these metres. Storing the true
// track moved by minus the day's GPS offset is the same as today's GPS
// (primed fixes included) being offset from where it was stored.
Oval::TrackModel shiftedModel(const Oval::TrackModel& m, double eastM, double northM);

// Lap time errors of a Tracker run against the truth (ms, signed), for laps
// that end on real crossings, with the lap before also on one (or Start).
struct TimeErrors {
    std::vector<double> provisional; // as first reported
    std::vector<double> refined;     // after LapTimeRefined (provisional if none came)
    std::vector<double> refineDelayMs; // reading time of the refined event minus the true crossing
    std::vector<double> refineAfterLapMs; // the same minus the crossing as first reported
    std::vector<Oval::Tracker::Mode> mode; // the mode each lap (of provisional/refined) ended in
};
TimeErrors lapTimeErrors(const TraceGen::Trace& tr, const TrackerResult& r);

// The true time (ms after Start) at which the physical truth reached metres.
double trueTimeOfPhys(const TraceGen::Trace& tr, double metres);

// ---------------------------------------------------------------------------
// Oval::Session
// ---------------------------------------------------------------------------

struct SessionResult {
    std::vector<Oval::Session::Event> events;      // every event, in order
    std::vector<size_t>               eventSample; // the reading that raised each
    std::vector<Oval::Session::Event> laps;        // TrackLap events that are Lap or MissedLap
    std::vector<Oval::Session::Event> joins;       // TrackJoined
    std::vector<size_t>               joinSample;
    std::vector<Oval::Session::Event> changes;     // SegmentChanged
    std::vector<size_t>               changeSample;
    std::vector<Oval::Session::Event> kms;         // RoadKilometre

    std::vector<float>   total;   // totalDistanceM() after each reading (0 before Start)
    std::vector<float>   road;
    std::vector<float>   track;
    std::vector<uint8_t> segment;
    std::vector<uint8_t> hold;    // holdElevation()
    std::vector<uint8_t> trackNumbers; // trackNumber() from Start, each change once
    size_t decreases     = 0;
    float  worstDecrease = 0.0f;
    float  finalTotal    = 0.0f;
    float  finalRoad     = 0.0f;
    float  finalTrack    = 0.0f;
    uint16_t runLaps     = 0;
    float  largestHold   = 0.0f;
    uint16_t autoJoins   = 0;           // joins the Session noticed by itself

    Oval::Tracker::CalStage finalStage = Oval::Tracker::CalStage::None;
    Oval::StoreOutcome      outcome;        // storeOutcome(number) after finish()
    uint8_t                 outcomeNumber = 0;
    uint8_t                 visited       = 0;
};

// Runs a trace through a Session as the service will: the trace's SetLane
// commands are the runner's lane (0: "No lane"). hook, if given, is called
// after each reading's update().
SessionResult runSession(const TraceGen::Trace& tr,
                         const Oval::LaneTable& table = Oval::LaneTable::outdoor(),
                         const Oval::Config& cfg = Oval::Config(),
                         const Oval::TrackStore* store = nullptr,
                         const std::function<void(size_t, Oval::Session&)>& hook = nullptr);

} // namespace Replay
