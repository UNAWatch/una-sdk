// Runs a trace through Oval::LapCounter. See Replay.hpp.

#include "Replay.hpp"

#include <cmath>
#include <limits>

namespace Replay {

Result run(const TraceGen::Trace& tr, const Oval::LaneTable& table, const Oval::Config& cfg)
{
    Result r;
    Oval::LapCounter counter(cfg);
    size_t nextCmd = 0;
    float  last    = 0.0f;

    for (size_t i = 0; i < tr.samples.size(); ++i) {
        if (i < tr.startIndex) {
            counter.prime(tr.samples[i]);
            r.total.push_back(0.0f);
            continue;
        }
        if (i == tr.startIndex) {
            counter.start(tr.startMs, tr.startLane, table);
        }
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            const TraceGen::Command& c = tr.commands[nextCmd];
            switch (c.kind) {
            case TraceGen::CmdKind::Pause:   counter.pause(c.tMs); break;
            case TraceGen::CmdKind::Resume:  counter.resume(c.tMs); break;
            case TraceGen::CmdKind::SetLane: counter.setLane(c.lane); break;
            case TraceGen::CmdKind::LapPress: (void)counter.restartLap(c.tMs); break;
            }
        }

        const Oval::LapCounter::Events ev = counter.update(tr.samples[i]);
        for (uint8_t k = 0; k < ev.count; ++k) {
            r.events.push_back(ev.items[k]);
            r.eventSample.push_back(i);
        }

        const float total = counter.totalDistanceM();
        if (total < last) {
            ++r.decreases;
            if (last - total > r.worstDecrease) {
                r.worstDecrease = last - total;
            }
        }
        last = total;
        r.total.push_back(total);
    }
    // Stop: a pass still pending is credited, as the service will do.
    const Oval::LapCounter::Events fin = counter.finish();
    for (uint8_t k = 0; k < fin.count; ++k) {
        r.events.push_back(fin.items[k]);
        r.eventSample.push_back(tr.samples.empty() ? 0 : tr.samples.size() - 1);
    }
    r.laps       = counter.lapsCompleted();
    r.missed     = counter.missedLapsCredited();
    r.finalTotal = counter.totalDistanceM();
    r.gpsRatio   = counter.gpsRatio();
    return r;
}

Match match(const TraceGen::Trace& tr, const Result& r, uint32_t toleranceMs)
{
    using Oval::LapCounter;
    Match m;
    m.trueLaps    = tr.crossMs.size();
    m.countedLaps = r.events.size();

    bool prevReal = true; // lap 1 starts at Start, which is exact
    for (size_t k = 0; k < r.events.size(); ++k) {
        const LapCounter::Event& e = r.events[k];
        const bool real = e.type == LapCounter::EventType::Lap;
        if (real) {
            ++m.realLaps;
        } else {
            ++m.missedLaps;
        }
        if (k < tr.crossMs.size()) {
            const double endErr = static_cast<double>(Oval::msDiff(e.tMs, tr.crossMs[k]));
            if (std::fabs(endErr) > toleranceMs) {
                ++m.badTimes;
            }
            if (real) {
                if (std::fabs(endErr) > m.worstEndErrMs) {
                    m.worstEndErrMs = std::fabs(endErr);
                }
                if (prevReal) {
                    const double trueDur = static_cast<double>(tr.crossActiveMs[k])
                        - (k == 0 ? 0.0 : static_cast<double>(tr.crossActiveMs[k - 1]));
                    m.lapTimeErrMs.push_back(static_cast<double>(e.lapDurationMs) - trueDur);
                }
            }
        }
        prevReal = real;
    }
    return m;
}

double creditedAfter(const TraceGen::Trace& tr, size_t n)
{
    double sum = 0.0;
    for (size_t k = 0; k < n && k < tr.lapCreditM.size(); ++k) {
        sum += tr.lapCreditM[k];
    }
    return sum;
}

// ---------------------------------------------------------------------------
// Oval::Tracker
// ---------------------------------------------------------------------------

TrackerResult runTracker(const TraceGen::Trace& tr, const Oval::LaneTable& table, const Oval::Config& cfg,
                         const Oval::TrackModel* supplied,
                         const std::function<void(size_t, Oval::Tracker&)>& hook)
{
    using Oval::Tracker;
    TrackerResult r;
    Tracker t(cfg);
    size_t nextCmd = 0;
    float  last    = 0.0f;

    auto record = [&](const Tracker::Events& ev, size_t i) {
        for (uint8_t k = 0; k < ev.count; ++k) {
            const Tracker::Event& e = ev.items[k];
            r.events.push_back(e);
            r.eventSample.push_back(i);
            switch (e.type) {
            case Tracker::EventType::Lap:
            case Tracker::EventType::MissedLap:
                r.laps.push_back(e);
                r.lapSample.push_back(i);
                r.refined.push_back(false);
                r.refinedEndMs.push_back(e.tMs);
                r.refinedDurMs.push_back(e.lapDurationMs);
                r.refinedSample.push_back(SIZE_MAX);
                break;
            case Tracker::EventType::LapTimeRefined: {
                const size_t idx = e.lapNumber > 0 ? e.lapNumber - 1u : 0u;
                if (idx < r.laps.size()) {
                    r.refined[idx]       = true;
                    r.refinedEndMs[idx]  = e.tMs;
                    r.refinedDurMs[idx]  = e.lapDurationMs;
                    r.refinedSample[idx] = i;
                    if (e.lapLengthM > 0.0f) {
                        // Credited again (shown on a stored track
                        // found to be another one).
                        r.laps[idx].lapLengthM = e.lapLengthM;
                        r.laps[idx].totalM     = e.totalM;
                    }
                }
                break;
            }
            case Tracker::EventType::CalibrationProvisional:
                r.provisionalAt    = i;
                r.provisionalModel = t.model();
                break;
            case Tracker::EventType::CalibrationLocked:
                r.lockedAt = i;
                t.lockedModel(r.lockedModel);
                break;
            case Tracker::EventType::CalibrationRejected:
                r.rejectedAt = i;
                break;
            case Tracker::EventType::ModeChanged:
                if (e.mode == Tracker::Mode::Calibrated && r.calibratedAt == SIZE_MAX) {
                    r.calibratedAt = i;
                }
                break;
            }
        }
    };

    for (size_t i = 0; i < tr.samples.size(); ++i) {
        if (i < tr.startIndex) {
            t.prime(tr.samples[i]);
            r.total.push_back(0.0f);
            r.mode.push_back(static_cast<uint8_t>(t.mode()));
            r.lateral.push_back(std::numeric_limits<float>::quiet_NaN());
            continue;
        }
        if (i == tr.startIndex) {
            t.start(tr.startMs, tr.startLane, table, supplied);
            if (t.mode() == Tracker::Mode::Calibrated) {
                r.calibratedAt = i;
            }
        }
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            const TraceGen::Command& c = tr.commands[nextCmd];
            switch (c.kind) {
            case TraceGen::CmdKind::Pause:   t.pause(c.tMs); break;
            case TraceGen::CmdKind::Resume:  t.resume(c.tMs); break;
            case TraceGen::CmdKind::SetLane: t.setLane(c.lane); break;
            case TraceGen::CmdKind::LapPress: record(t.restartLap(c.tMs), i); break;
            }
        }
        record(t.update(tr.samples[i]), i);

        const float total = t.totalDistanceM();
        if (total < last) {
            ++r.decreases;
            if (last - total > r.worstDecrease) {
                r.worstDecrease = last - total;
            }
        }
        last = total;
        r.total.push_back(total);
        r.mode.push_back(static_cast<uint8_t>(t.mode()));
        r.lateral.push_back(t.hasLateralOffset() ? t.lateralOffsetM() : std::numeric_limits<float>::quiet_NaN());
        if (r.alignedAt == SIZE_MAX && (t.alignment().done || t.alignment().abandoned)) {
            r.alignedAt = i;
        }
        if (hook) {
            hook(i, t);
        }
    }
    record(t.finish(), tr.samples.empty() ? 0 : tr.samples.size() - 1);
    r.finalTotal  = t.totalDistanceM();
    r.largestHold = t.largestHoldM();
    r.finalMode   = t.mode();
    r.finalStage  = t.calibrationStage();
    r.frame       = t.counter().frame();
    r.missed      = t.missedLapsCredited();
    r.alignment   = t.alignment();
    r.visit       = t.visitCheck();
    r.outcome     = t.storeOutcome();
    return r;
}

Oval::TrackModel shiftedModel(const Oval::TrackModel& m, double eastM, double northM)
{
    Oval::LocalFrame f;
    f.setOrigin(m.centreLat(), m.centreLon());
    double lat = 0.0, lon = 0.0;
    f.toLatLon(Oval::Vec2{static_cast<float>(eastM), static_cast<float>(northM)}, lat, lon);
    return Oval::TrackModel::fromGeo(lat, lon, m.headingDeg(), m.kerbRadiusM(), m.clockwise(), m.altitudeM());
}

Oval::TrackModel truthModel(const TraceGen::Track& track)
{
    return Oval::TrackModel::fromGeo(track.centreLat, track.centreLon, track.headingDeg, track.kerbRadiusM,
                                     false, 50.0f);
}

TimeErrors lapTimeErrors(const TraceGen::Trace& tr, const TrackerResult& r)
{
    TimeErrors out;
    bool prevReal = true; // lap 1 starts at Start, which is exact
    for (size_t k = 0; k < r.laps.size() && k < tr.crossMs.size(); ++k) {
        const bool real = r.laps[k].type == Oval::Tracker::EventType::Lap;
        if (real && prevReal) {
            const double trueDur = static_cast<double>(tr.crossActiveMs[k])
                - (k == 0 ? 0.0 : static_cast<double>(tr.crossActiveMs[k - 1]));
            out.provisional.push_back(static_cast<double>(r.laps[k].lapDurationMs) - trueDur);
            out.refined.push_back(static_cast<double>(r.refinedDurMs[k]) - trueDur);
            out.mode.push_back(r.laps[k].mode);
            if (r.refined[k]) {
                out.refineDelayMs.push_back(
                    static_cast<double>(Oval::msDiff(tr.samples[r.refinedSample[k]].tMs, tr.crossMs[k])));
                out.refineAfterLapMs.push_back(
                    static_cast<double>(Oval::msDiff(tr.samples[r.refinedSample[k]].tMs, r.laps[k].tMs)));
            }
        }
        prevReal = real;
    }
    return out;
}

SessionResult runSession(const TraceGen::Trace& tr, const Oval::LaneTable& table, const Oval::Config& cfg,
                         const Oval::TrackStore* store,
                         const std::function<void(size_t, Oval::Session&)>& hook)
{
    using Oval::Session;
    SessionResult r;
    Session s(cfg);
    size_t nextCmd = 0;
    float  last    = 0.0f;

    auto record = [&](const Session::Events& ev, size_t i) {
        for (uint8_t k = 0; k < ev.count; ++k) {
            const Session::Event& e = ev.items[k];
            r.events.push_back(e);
            r.eventSample.push_back(i);
            switch (e.type) {
            case Session::EventType::TrackLap:
                if (e.track.type == Oval::Tracker::EventType::Lap || e.track.type == Oval::Tracker::EventType::MissedLap) {
                    r.laps.push_back(e);
                }
                break;
            case Session::EventType::TrackJoined:
                r.joins.push_back(e);
                r.joinSample.push_back(i);
                break;
            case Session::EventType::SegmentChanged:
                r.changes.push_back(e);
                r.changeSample.push_back(i);
                break;
            case Session::EventType::RoadKilometre:
                r.kms.push_back(e);
                break;
            case Session::EventType::TrackEvent:
                break;
            }
        }
    };

    for (size_t i = 0; i < tr.samples.size(); ++i) {
        if (i < tr.startIndex) {
            s.prime(tr.samples[i]);
            r.total.push_back(0.0f);
            r.road.push_back(0.0f);
            r.track.push_back(0.0f);
            r.segment.push_back(0);
            r.hold.push_back(0);
            continue;
        }
        if (i == tr.startIndex) {
            s.start(tr.startMs, tr.startLane, table, store);
        }
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            const TraceGen::Command& c = tr.commands[nextCmd];
            switch (c.kind) {
            case TraceGen::CmdKind::Pause:    s.pause(c.tMs); break;
            case TraceGen::CmdKind::Resume:   s.resume(c.tMs); break;
            case TraceGen::CmdKind::SetLane:  record(s.setLane(c.lane, c.tMs), i); break;
            case TraceGen::CmdKind::LapPress: record(s.lapPress(c.tMs), i); break;
            }
        }
        record(s.update(tr.samples[i]), i);

        const float total = s.totalDistanceM();
        if (total < last) {
            ++r.decreases;
            if (last - total > r.worstDecrease) {
                r.worstDecrease = last - total;
            }
        }
        last = total;
        r.total.push_back(total);
        r.road.push_back(s.roadDistanceM());
        r.track.push_back(s.trackDistanceM());
        r.segment.push_back(static_cast<uint8_t>(s.segment()));
        r.hold.push_back(s.holdElevation() ? 1 : 0);
        if (hook) {
            hook(i, s);
        }
    }
    record(s.finish(), tr.samples.empty() ? 0 : tr.samples.size() - 1);
    r.finalTotal   = s.totalDistanceM();
    r.finalRoad    = s.roadDistanceM();
    r.finalTrack   = s.trackDistanceM();
    r.runLaps      = s.runLaps();
    r.largestHold  = s.largestHoldM();
    r.autoJoins    = s.autoJoins();
    r.finalStage   = s.tracker().calibrationStage();
    r.outcome      = s.storeOutcome(r.outcomeNumber);
    r.visited      = s.visitedCount();
    return r;
}

double trueTimeOfPhys(const TraceGen::Trace& tr, double metres)
{
    for (size_t i = tr.startIndex + 1; i < tr.samples.size(); ++i) {
        const double a = tr.truePhysM[i - 1], b = tr.truePhysM[i];
        if (b >= metres && a < metres) {
            const double ta = Oval::msDiff(tr.samples[i - 1].tMs, tr.startMs);
            const double tb = Oval::msDiff(tr.samples[i].tMs, tr.startMs);
            return ta + (metres - a) / (b - a) * (tb - ta);
        }
    }
    return -1.0;
}

} // namespace Replay
