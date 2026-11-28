// A stored neighbour track picked by mistake (docs/OVAL.md).
//
// The runner is on one track; the store holds another 40 or 60 m away, turned
// 15 to 50 degrees (a warm-up track beside the main one), and that is the one
// picked. Without a check for it, it was lined up and used in about 20% of runs at 40 m
// and 10% at 60 m, almost always with wrong laps. A run that keeps the
// neighbour and counts its laps wrongly must now be at most 1 in 100.
//
// TRACKRUN_NEIGHBOUR_RUNS sets the runs per distance (300 by default).

#include "OvalTestUtil.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/Session.hpp"
#include "Oval/TrackStore.hpp"
#include "Oval/Tracker.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace OvalTest;

namespace {

TraceGen::Track movedBy(const TraceGen::Track& t, double e, double n)
{
    Oval::LocalFrame f;
    f.setOrigin(t.centreLat, t.centreLon);
    TraceGen::Track m = t;
    f.toLatLon(Oval::Vec2{static_cast<float>(e), static_cast<float>(n)}, m.centreLat, m.centreLon);
    return m;
}

// Laps one for one with the truth, each credited its lane's length and ending
// (as refined or re-timed last) within endTolMs of the true pass of the start
// line.
bool lapsRight(const TraceGen::Trace& tr, const Replay::TrackerResult& r, uint8_t lane, uint32_t endTolMs)
{
    if (r.decreases != 0 || r.laps.size() != tr.crossMs.size()) {
        return false;
    }
    const float len = Oval::LaneTable::outdoor().lapLengthM(lane);
    for (size_t k = 0; k < r.laps.size(); ++k) {
        const uint32_t end = r.refined[k] ? r.refinedEndMs[k] : r.laps[k].tMs;
        if (std::abs(Oval::msDiff(end, tr.crossMs[k])) > static_cast<int32_t>(endTolMs)
            || !(std::fabs(r.laps[k].lapLengthM - len) <= 0.01f)) {   // a NaN length is wrong too
            return false;
        }
    }
    return true;
}

} // namespace

TEST(NeighbourSweep, TurnedNeighbourAt40And60m)
{
    const int total = static_cast<int>(envNumber("TRACKRUN_NEIGHBOUR_RUNS", 300));
    for (double apart : {40.0, 60.0}) {
        std::mt19937 rng(PortableRandom::seeded(20261028u + static_cast<uint32_t>(apart)));
        auto uni  = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
        auto pick = [&](int a, int b) { return PortableRandom::UniformInt(a, b)(rng); };
        size_t kept = 0, keptWrong = 0, dropped = 0, droppedWrong = 0, afterLap = 0, unmatched = 0;
        for (int run = 0; run < total; ++run) {
            TraceGen::Track real;
            real.headingDeg  = static_cast<float>(uni(0.0, 180.0));
            real.kerbRadiusM = static_cast<float>(uni(35.0, 38.0));
            real.centreLat   = uni(-55.0, 60.0);
            real.centreLon   = uni(-170.0, 170.0);
            const double bearing = uni(0.0, 2.0 * 3.14159265358979);
            const double turn    = uni(15.0, 50.0);
            const double sign    = pick(0, 1) == 0 ? -1.0 : 1.0;
            TraceGen::Track other = movedBy(real, apart * PortableMath::cos(bearing), apart * PortableMath::sin(bearing));
            other.headingDeg  = static_cast<float>(real.headingDeg + sign * turn);
            other.kerbRadiusM = static_cast<float>(uni(35.0, 38.0));

            const uint8_t lane  = static_cast<uint8_t>(pick(1, 8));
            const float   speed = static_cast<float>(uni(3.5, 6.0));
            const float   sigma = static_cast<float>(uni(2.0, 5.0));
            TraceGen::Noise n = noisy(sigma, static_cast<float>(uni(0.05, 0.15)));
            const uint32_t seed = rng();
            const float    from = static_cast<float>(uni(0.0, 1.0));
            TraceGen::Generator g(real, n, seed);
            g.begin(from, lane);
            g.runLaps(4.5, runner(speed));
            const TraceGen::Trace tr = g.finish();

            const Oval::TrackModel        stored = Replay::truthModel(other);
            const Replay::TrackerResult   r      = Replay::runTracker(tr, Oval::LaneTable::outdoor(), Oval::Config(), &stored);
            const bool right = lapsRight(tr, r, lane, 5000);
            if (r.alignment.abandoned) {
                ++dropped;
                droppedWrong += right ? 0 : 1;
                afterLap += r.alignment.turned && r.alignment.retimed > 0 ? 1 : 0;
                unmatched += r.alignment.unmatched;
                if (!right) {
                    std::printf("[ neighbour]   dropped with wrong laps: %.0f m, run %d, turned %+.0f deg, lane %u, "
                                "%.1f m/s, %.1f m noise: %zu laps of %zu, dropped at %.0f m (%s), %u re-timed, %u "
                                "unmatched\n",
                                apart, run, sign * turn, lane, speed, sigma, r.laps.size(), tr.crossMs.size(),
                                r.alignment.turned ? r.alignment.dropM : r.alignment.endM,
                                r.alignment.turned ? "turned" : "lining up", r.alignment.retimed, r.alignment.unmatched);
                    for (size_t k = 0; k < r.laps.size() && k < tr.crossMs.size(); ++k) {
                        const uint32_t end = r.refined[k] ? r.refinedEndMs[k] : r.laps[k].tMs;
                        std::printf("[ neighbour]     lap %zu: %.2f m, ends %+d ms (first shown %+d ms), mode %d\n", k + 1,
                                    r.laps[k].lapLengthM, Oval::msDiff(end, tr.crossMs[k]),
                                    Oval::msDiff(r.laps[k].tMs, tr.crossMs[k]), static_cast<int>(r.laps[k].mode));
                    }
                }
            } else {
                ++kept;
                keptWrong += right ? 0 : 1;
                if (!right) {
                    std::printf("[ neighbour]   kept with wrong laps: %.0f m, run %d, turned %+.0f deg, lane %u, %.1f m/s, "
                                "%.1f m noise: %zu laps of %zu; turn found %.1f deg (se %.1f), offset %.1f m, scale "
                                "%.1f m, lined up after %.0f m; confirmed %d, first fit at reading %zu of %zu, "
                                "heading %.1f against %.1f stored\n",
                                apart, run, sign * turn, lane, speed, sigma, r.laps.size(), tr.crossMs.size(),
                                r.alignment.turnDeg, r.alignment.turnSeDeg, Oval::length(r.alignment.offset),
                                r.alignment.scaleM, r.alignment.endM, r.alignment.confirmedBy,
                                r.provisionalAt == SIZE_MAX ? 0 : r.provisionalAt - tr.startIndex, tr.samples.size() - tr.startIndex,
                                r.provisionalModel.isValid() ? r.provisionalModel.headingDeg() : -1.0f, stored.headingDeg());
                }
            }
        }
        // Dropped runs with a wrong lap are reported, not asserted: lining up's
        // own drop credits counting mode's laps, which at the noisy end can
        // end a lap early.
        std::printf("[ neighbour] %.0f m apart, %d runs: dropped %zu (%zu of them with wrong laps; %zu after laps "
                    "were shown, re-timed; %zu laps shown that counting mode never saw), kept %zu (%zu of them with "
                    "wrong laps, %.1f%%)\n",
                    apart, total, dropped, droppedWrong, afterLap, unmatched, kept, keptWrong,
                    100.0 * keptWrong / total);
        EXPECT_LE(keptWrong * 100, static_cast<size_t>(total)) << apart << " m: kept with wrong laps in over 1% of runs";
    }
}

TEST(NeighbourSweep, SessionEndsOnTheRightTrack)
{
    // A warm-up track W 40 m from the main track M, turned 30 degrees, and
    // Start pressed on M where W's centre is the nearer: W is picked. With
    // only W stored, the run is a new track and W is left alone; with both
    // stored, the run ends on M. Every lap right either way.
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    size_t runs = 0, right = 0, restarted = 0;
    for (int both = 0; both < 2; ++both) {
        for (uint32_t seed = 1; seed <= 10; ++seed) {
            std::mt19937 rng(PortableRandom::seeded(4000u + seed));
            auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
            TraceGen::Track m;
            m.headingDeg = static_cast<float>(uni(0.0, 180.0));
            const double bearing = uni(0.0, 2.0 * 3.14159265358979);
            TraceGen::Track w = movedBy(m, 40.0 * PortableMath::cos(bearing), 40.0 * PortableMath::sin(bearing));
            w.headingDeg = m.headingDeg + (seed % 2 == 0 ? 30.0f : -30.0f);
            const Oval::TrackModel wm = Replay::truthModel(w);
            const Oval::TrackModel mm = Replay::truthModel(m);
            const uint8_t lane  = static_cast<uint8_t>(1 + seed % 8);
            const float   speed = static_cast<float>(uni(3.5, 5.5));
            const TraceGen::Noise n = noisy(static_cast<float>(uni(2.0, 4.0)));

            // A start on M nearer W's centre than M's.
            float from = -1.0f;
            for (int k = 0; k < 40 && from < 0.0f; ++k) {
                TraceGen::Generator probe(m, n, seed);
                probe.begin(0.025f * static_cast<float>(k), lane);
                const TraceGen::Trace pt = probe.finish();
                Oval::LocalFrame f;
                f.setOrigin(pt.startLat, pt.startLon);
                if (Oval::length(f.toLocal(wm.centreLat(), wm.centreLon()))
                    < Oval::length(f.toLocal(mm.centreLat(), mm.centreLon())) - 5.0f) {
                    from = 0.025f * static_cast<float>(k);
                }
            }
            ASSERT_GE(from, 0.0f) << "seed " << seed;

            Oval::TrackStore store;
            const uint8_t wNum = store.saveNew(wm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u).number;
            const uint8_t mNum = both ? store.saveNew(mm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u).number : 0;

            TraceGen::Generator g(m, n, 5000u + seed);
            g.begin(from, lane);
            g.runLaps(4.5, runner(speed));
            const TraceGen::Trace tr = g.finish();
            const Replay::SessionResult r = Replay::runSession(tr, table, Oval::Config(), &store);

            // Each lap as it stands at the end: as raised, then as refined or
            // re-timed and credited again (as the service's lap list takes it).
            std::vector<uint32_t> endMs;
            std::vector<float>    lenM;
            for (const auto& e : r.laps) {
                endMs.push_back(e.tMs);
                lenM.push_back(e.track.lapLengthM);
            }
            for (const auto& e : r.events) {
                if (e.type == Oval::Session::EventType::TrackLap
                    && e.track.type == Oval::Tracker::EventType::LapTimeRefined && e.runLap >= 1
                    && e.runLap <= endMs.size()) {
                    endMs[e.runLap - 1] = e.tMs;
                    if (e.track.lapLengthM > 0.0f) {
                        lenM[e.runLap - 1] = e.track.lapLengthM;
                    }
                }
            }
            bool lapsOk = r.laps.size() == tr.crossMs.size() && r.decreases == 0;
            for (size_t k = 0; lapsOk && k < r.laps.size(); ++k) {
                lapsOk = std::fabs(lenM[k] - table.lapLengthM(lane)) <= 0.01f
                    && std::abs(Oval::msDiff(endMs[k], tr.crossMs[k])) <= 5000;
            }
            const bool onM = both ? (r.outcomeNumber == mNum
                                     && (r.outcome.kind == Oval::StoreOutcome::Kind::Updated
                                         || r.outcome.kind == Oval::StoreOutcome::Kind::Unchanged))
                                  : (r.outcome.kind == Oval::StoreOutcome::Kind::New && r.outcomeNumber != wNum);
            ++runs;
            right += lapsOk && onM ? 1 : 0;
            restarted += both && !r.laps.empty() && r.laps[0].trackNumber == mNum ? 1 : 0;
            std::printf("[ neighbour] session, %s stored, seed %u: %zu of %zu laps%s, outcome %d for track %u\n",
                        both ? "W and M" : "W only", seed, r.laps.size(), tr.crossMs.size(), lapsOk ? "" : " (wrong)",
                        static_cast<int>(r.outcome.kind), r.outcomeNumber);
            EXPECT_TRUE(lapsOk) << (both ? "both" : "W only") << " seed " << seed;
            EXPECT_TRUE(onM) << (both ? "both" : "W only") << " seed " << seed << ": outcome "
                             << static_cast<int>(r.outcome.kind) << " for track " << int(r.outcomeNumber);
        }
    }
    std::printf("[ neighbour] session: %zu of %zu runs right; with both stored, %zu of 10 on M from lap 1 (started "
                "again on it at Start), the rest matched to it at the end\n",
                right, runs, restarted);
}

TEST(NeighbourSweep, UpdateTiming)
{
    // The neighbour checks' work in one Session::update(), timed on this PC (Release):
    // a turn test (every 30 m on an unconfirmed stored track), the drop of a
    // turned neighbour (with laps re-timed when already shown), and the
    // restart on the other stored track with its replay from Start. Compare
    // with RunCore.UpdateTiming's worst replay, timed the same way.
    if (std::getenv("TRACKRUN_BENCH") == nullptr) {
        GTEST_SKIP() << "set TRACKRUN_BENCH=1 (Release build) to time Session::update()";
    }
    double worstTurn = 0.0, worstDrop = 0.0, worstRestart = 0.0, worstPlain = 0.0;
    size_t turns = 0, drops = 0, restarts = 0;
    for (int both = 0; both < 2; ++both) {
        for (uint32_t seed = 1; seed <= 20; ++seed) {
            std::mt19937 rng(PortableRandom::seeded(6000u + seed));
            auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
            TraceGen::Track m;
            m.headingDeg = static_cast<float>(uni(0.0, 180.0));
            const double bearing = uni(0.0, 2.0 * 3.14159265358979);
            TraceGen::Track w = movedBy(m, 40.0 * PortableMath::cos(bearing), 40.0 * PortableMath::sin(bearing));
            w.headingDeg = m.headingDeg + (seed % 2 == 0 ? 30.0f : -30.0f);
            Oval::TrackStore store;
            store.saveNew(Replay::truthModel(w), Oval::FitQuality{3.0f, 0.5f}, 1760000000u);
            if (both) {
                store.saveNew(Replay::truthModel(m), Oval::FitQuality{3.0f, 0.5f}, 1760000000u);
            }
            // Start anywhere on M, lane 8: stores where W is picked are the
            // ones timed.
            TraceGen::Generator g(m, noisy(3.0f), 7000u + seed);
            g.begin(static_cast<float>(uni(0.0, 1.0)), 8);
            g.runLaps(4.5, runner(4.5f));
            const TraceGen::Trace tr = g.finish();
            static Oval::Session s;
            s = Oval::Session();
            for (size_t i = 0; i < tr.samples.size(); ++i) {
                if (i < tr.startIndex) {
                    s.prime(tr.samples[i]);
                    continue;
                }
                if (i == tr.startIndex) {
                    s.start(tr.startMs, tr.startLane, Oval::LaneTable::outdoor(), &store);
                }
                const Oval::Tracker::Alignment before = s.tracker().alignment();
                const uint8_t number = s.trackNumber();
                const auto t0 = std::chrono::steady_clock::now();
                (void)s.update(tr.samples[i]);
                const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
                const Oval::Tracker::Alignment& after = s.tracker().alignment();
                if (number != 0 && s.trackNumber() != number && s.trackNumber() != 0) {
                    ++restarts;
                    worstRestart = std::max(worstRestart, us);
                } else if (after.turned && !before.turned) {
                    ++drops;
                    worstDrop = std::max(worstDrop, us);
                } else if (after.turnDeg != before.turnDeg || after.turnSeDeg != before.turnSeDeg) {
                    ++turns;
                    worstTurn = std::max(worstTurn, us);
                } else {
                    worstPlain = std::max(worstPlain, us);
                }
            }
        }
    }
    std::printf("[ timing   ] on this PC: %zu turn tests, worst %.0f us; %zu turned drops, worst %.0f us; "
                "%zu restarts on the other stored track, worst %.0f us; every other reading, worst %.0f us\n",
                turns, worstTurn, drops, worstDrop, restarts, worstRestart, worstPlain);
    EXPECT_GT(turns, 0u);
}
