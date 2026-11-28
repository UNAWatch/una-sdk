// Oval::Session: randomised road and track runs (docs/OVAL.md).
//
// Each run is one of: road to a stored track and back (joined and left by
// itself), road to a track not stored and back (the runner sets the lane on
// arriving), a run on a track only (a lane at Start), or road only (no lane,
// with loops that must not look like a track). Lane 1 to 8, noise 2 to 5 m, a
// random day's GPS offset for stored tracks, random speeds, entry points and
// road routes. TRACKRUN_SESSION_RUNS and TRACKRUN_SESSION_SEED change the
// count and the seed.

#include "SessionTestUtil.hpp"


#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace SessionTest;
using Oval::Session;
using TraceGen::Pt;

namespace {

enum class Kind : uint8_t { Stored, NotStored, TrackOnly, RoadOnly };

const char* kindName(Kind k)
{
    switch (k) {
    case Kind::Stored:    return "stored";
    case Kind::NotStored: return "not stored";
    case Kind::TrackOnly: return "track only";
    case Kind::RoadOnly:  return "road only";
    }
    return "?";
}

uint32_t envU(const char* name, uint32_t dflt)
{
    return OvalTest::envSeed(name, dflt);
}

uint32_t envRuns(const char* name, uint32_t dflt)
{
    return OvalTest::envNumber(name, dflt);
}

struct SweepRun {
    Kind            kind   = Kind::RoadOnly;
    uint8_t         lane   = 1;
    float           noise  = 3.0f;
    double          offE   = 0.0;
    double          offN   = 0.0;
    bool            stored = false; // a stored track for this track
    const char*     loop   = "";    // road only: the loop on the way
    TraceGen::Track tk;
    TraceGen::Trace tr;
};

SweepRun makeRun(uint32_t seed, double maxOffsetM)
{
    std::mt19937 rng(PortableRandom::seeded(seed));
    PortableRandom::Uniform u01(0.0, 1.0);
    SweepRun run;
    const double k = u01(rng);
    run.kind  = k < 0.35 ? Kind::Stored : (k < 0.6 ? Kind::NotStored : (k < 0.8 ? Kind::TrackOnly : Kind::RoadOnly));
    run.lane  = static_cast<uint8_t>(1 + static_cast<int>(8.0 * u01(rng)) % 8);
    run.noise = static_cast<float>(2.0 + 3.0 * u01(rng));
    const double offM = maxOffsetM * u01(rng), offA = 2.0 * kPi * u01(rng);
    run.offE = offM * PortableMath::cos(offA);
    run.offN = offM * PortableMath::sin(offA);
    run.tk   = randomTrack(rng);
    run.stored = run.kind == Kind::Stored || (run.kind == Kind::TrackOnly && u01(rng) < 0.5);

    TraceGen::Generator g(run.tk, OvalTest::noisy(run.noise), seed);
    const float  trackMps = static_cast<float>(3.5 + 2.5 * u01(rng));
    const float  roadMps  = static_cast<float>(2.8 + 1.2 * u01(rng));
    const double laps     = 3.0 + std::floor(5.0 * u01(rng)); // 3 to 7 whole laps, then 120 to 280 m
    const double tail     = 120.0 + 160.0 * u01(rng);
    const double roadIn   = 600.0 + 1900.0 * u01(rng);
    const double roadOut  = 600.0 + 1900.0 * u01(rng);

    switch (run.kind) {
    case Kind::Stored:
    case Kind::NotStored: {
        const double tauIn = 4.0 * u01(rng);
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, run.lane, roadIn, rng, home);
        g.beginAt(home, Session::kNoLane, 1000000u + static_cast<uint32_t>(4.0e9 * u01(rng)));
        g.road(in, jog(roadMps));
        g.joinTrack(tauIn, run.lane);
        if (run.kind == Kind::NotStored) {
            g.setLane(run.lane);
        }
        g.run(laps * g.laneLengthM(run.lane) + tail, OvalTest::runner(trackMps));
        g.leaveToRoad();
        g.road(routeOut(g, run.lane, roadOut, rng), jog(roadMps));
        break;
    }
    case Kind::TrackOnly: {
        // One draw a statement: the order of a call's arguments is the compiler's.
        const float    startAt = static_cast<float>(u01(rng));
        const uint32_t t0      = 1000000u + static_cast<uint32_t>(4.0e9 * u01(rng));
        g.begin(startAt, run.lane, t0);
        g.run((laps + 1.0) * g.laneLengthM(run.lane) + tail, OvalTest::runner(trackMps));
        break;
    }
    case Kind::RoadOnly: {
        const Pt home{-2500.0 + 500.0 * u01(rng), 1500.0 + 500.0 * u01(rng)};
        g.beginAt(home, Session::kNoLane, 1000000u + static_cast<uint32_t>(4.0e9 * u01(rng)));
        std::vector<Pt> route = legsAway(home, 2.0 * kPi * u01(rng), roadIn, rng);
        g.road(route, jog(roadMps));
        // Sometimes a loop on the way that is not a track.
        const double which = u01(rng);
        const int    times = 2 + static_cast<int>(3.0 * u01(rng));
        if (which < 0.25) {
            g.road(loopShape(parkLoop(380.0 + 60.0 * u01(rng)), g.here(), times), jog(roadMps));
            run.loop = "park loop";
        } else if (which < 0.45) {
            g.road(loopShape(circle(15.0 + 15.0 * u01(rng)), g.here(), times + 2), jog(roadMps));
            run.loop = "roundabout";
        } else if (which < 0.6) {
            const double w = 100.0 + 50.0 * u01(rng); // one draw a statement
            const double h = 60.0 + 40.0 * u01(rng);
            g.road(loopShape(carPark(w, h), g.here(), times), jog(roadMps));
            run.loop = "car park";
        } else if (which < 0.7) {
            g.road(loopShape(figureEight(35.0 + 20.0 * u01(rng)), g.here(), times), jog(roadMps));
            run.loop = "figure of eight";
        }
        g.road(legsAway(g.here(), 2.0 * kPi * u01(rng), roadOut, rng), jog(roadMps));
        break;
    }
    }
    run.tr = g.finish();
    return run;
}

struct Totals {
    size_t runs = 0, failed = 0, laps = 0, falseEntries = 0, missedEntries = 0, falseExits = 0;
    size_t falseLaps = 0, missedLaps = 0, inexact = 0, decreases = 0, ambiguous = 0;
    std::vector<double> roadErrPct;   // |road - GPS on the road| / road, %
    std::vector<double> roadErrM;     // the same in metres
    std::vector<double> roadNetM;     // the same, less the GPS distance between the Session's joins and exits and the true ones
    std::vector<double> totalErrM;    // total - (GPS distance on the road + true track distance), signed
    std::vector<double> entryS[4];    // stored tracks: TrackJoined after the true entry, s, by offset 0-5, 5-10, 10-15, 15-20 m
    std::vector<double> entryM[4];    // and metres run on the track by then
    std::vector<double> exitS;        // exit placed from the true exit, s
    std::vector<double> joinS;        // join point placed from the true entry, s
};

void sweep(uint32_t seed0, uint32_t runs, double maxOffsetM, Totals& t)
{
    for (uint32_t n = 0; n < runs; ++n) {
        const uint32_t seed = seed0 * 100003u + n;
        SweepRun run = makeRun(seed, maxOffsetM);
        Oval::TrackStore store;
        uint8_t number = 0;
        if (run.stored) {
            number = storeTrack(store, run.tk, run.offE, run.offN, run.lane); // the lane last run there
        }
        const Replay::SessionResult r =
            Replay::runSession(run.tr, Oval::LaneTable::outdoor(), Oval::Config(), run.stored ? &store : nullptr);
        const TraceGen::Trace& tr = run.tr;
        const float laneLen = Oval::LaneTable::outdoor().lapLengthM(run.lane);
        ++t.runs;
        t.laps += r.laps.size();
        bool bad = false;
        std::string why;
        bool   haveExit = false;   // this run's exit against the true one
        double exitS    = 0.0;

        if (r.decreases != 0) {
            ++t.decreases;
            bad = true;
            why += " total went down;";
        }
        for (const Session::Event& e : r.laps) {
            if (std::fabs(e.track.lapLengthM - laneLen) > 0.01f) {
                ++t.inexact;
                bad = true;
                why += " a lap not exact;";
            }
        }

        if (run.kind == Kind::Stored || run.kind == Kind::NotStored) {
            // Joins noticed: one for a stored track, none for the runner's lane.
            const size_t joins = r.joins.size();
            const size_t wantJoins = run.kind == Kind::Stored ? 1u : 0u;
            if (joins > wantJoins) {
                t.falseEntries += joins - wantJoins;
                bad = true;
                why += " extra entry;";
            }
            if (joins < wantJoins) {
                ++t.missedEntries;
                bad = true;
                why += " never joined;";
            }
            // Road to Track, Track to Road.
            size_t exits = 0, entries = 0;
            for (const Session::Event& c : r.changes) {
                exits   += (c.from == Session::Segment::Track && c.segment == Session::Segment::Road) ? 1u : 0u;
                entries += (c.segment == Session::Segment::Track) ? 1u : 0u;
            }
            if (exits > 1) {
                t.falseExits += exits - 1;
                bad = true;
                why += " extra exit;";
            }
            if (entries == 1 && exits == 1 && r.changes.size() == 2) {
                const size_t js = sampleAtOrAfter(tr, r.changes[0].tMs);
                const size_t xs = sampleAtOrAfter(tr, r.changes[1].tMs);
                const TrueLaps tl   = trueLapsBetween(tr, js, xs, laneLen, 15.0);
                const size_t   want = tl.want;
                const SegLaps  s    = lapsBetween(r, r.changes[0].tMs, r.changes[1].tMs, laneLen);
                if (tl.ambiguous) {
                    ++t.ambiguous;
                    if (s.laps < tl.lo || s.laps > tl.hi) {
                        bad = true;
                        why += " laps far off;";
                    }
                } else if (s.laps > want) {
                    t.falseLaps += s.laps - want;
                    bad = true;
                    why += " false lap;";
                } else if (s.laps < want) {
                    t.missedLaps += want - s.laps;
                    bad = true;
                    why += " missed lap;";
                }
                exitS    = Oval::msDiff(r.changes[1].tMs, tr.visits[0].exitMs) / 1000.0;
                haveExit = true;
                t.exitS.push_back(exitS);
                t.joinS.push_back(Oval::msDiff(r.changes[0].tMs, tr.visits[0].joinMs) / 1000.0);
                if (run.kind == Kind::Stored && !r.joinSample.empty()) {
                    const double off = std::hypot(run.offE, run.offN);
                    const int    bin = std::min(3, static_cast<int>(off / 5.0));
                    t.entryS[bin].push_back(Oval::msDiff(tr.samples[r.joinSample[0]].tMs, tr.visits[0].joinMs) / 1000.0);
                    t.entryM[bin].push_back(tr.truePhysM[r.joinSample[0]] - tr.truePhysM[tr.visits[0].joinSample]);
                }
            } else if (!bad) {
                bad = true;
                why += " segments not road, track, road;";
            }
            if (!r.joins.empty() && r.joins[0].trackNumber != number) {
                bad = true;
                why += " wrong track number;";
            }
            const double road = tr.gpsRoadM.back();
            if (road > 0.0) {
                t.roadErrM.push_back(std::fabs(r.finalRoad - road));
                t.roadErrPct.push_back(100.0 * std::fabs(r.finalRoad - road) / road);
                const Allowance a = allowance(tr, r);
                t.roadNetM.push_back(std::max(0.0, std::fabs(r.finalRoad - road) - a.gpsM));
                t.totalErrM.push_back(r.finalTotal - (road + tr.trueTrackM.back()));
                if (std::fabs(t.totalErrM.back()) > 25.0 || (haveExit && std::fabs(exitS) > 10.0)) {
                    std::printf("[ sweep    ] note: run %u (seed %u, %s, lane %u, noise %.1f, offset %.1f m): total %+.1f m, exit %+.1f s\n",
                                n, seed, kindName(run.kind), run.lane, run.noise, std::hypot(run.offE, run.offN),
                                t.totalErrM.back(), haveExit ? exitS : 0.0);
                }
            }
        } else if (run.kind == Kind::TrackOnly) {
            if (!r.joins.empty()) {
                t.falseEntries += r.joins.size();
                bad = true;
                why += " joined while on the track;";
            }
            for (const Session::Event& c : r.changes) {
                if (c.segment == Session::Segment::Road) {
                    ++t.falseExits;
                    bad = true;
                    why += " went to road;";
                }
            }
            if (r.laps.size() > tr.crossMs.size()) {
                t.falseLaps += r.laps.size() - tr.crossMs.size();
                bad = true;
                why += " false lap;";
            } else if (r.laps.size() < tr.crossMs.size()) {
                t.missedLaps += tr.crossMs.size() - r.laps.size();
                bad = true;
                why += " missed lap;";
            }
        } else {
            if (!r.joins.empty()) {
                t.falseEntries += r.joins.size();
                bad = true;
                why += " joined a track that is not there;";
            }
            if (!r.laps.empty()) {
                t.falseLaps += r.laps.size();
                bad = true;
                why += " laps on the road;";
            }
            for (const Session::Event& c : r.changes) {
                if (c.segment == Session::Segment::Track) {
                    ++t.falseEntries;
                    bad = true;
                    why += " a track segment on the road;";
                }
            }
            const double gps = tr.gpsRoadM.back();
            t.roadErrM.push_back(std::fabs(r.finalRoad - gps));
            t.roadErrPct.push_back(gps > 0.0 ? 100.0 * std::fabs(r.finalRoad - gps) / gps : 0.0);
        }
        if (bad) {
            ++t.failed;
            std::printf("[ sweep    ] run %u (seed %u, %s%s%s, lane %u, noise %.1f, offset %.1f m): %s\n", n, seed,
                        kindName(run.kind), run.loop[0] ? ", " : "", run.loop, run.lane, run.noise,
                        std::hypot(run.offE, run.offN), why.c_str());
        }
    }
}

void print(const char* title, const Totals& t)
{
    std::printf("[ sweep    ] %s: %zu runs, %zu failed, %zu laps; false entries %zu, missed entries %zu, false exits %zu, "
                "false laps %zu, missed laps %zu, inexact %zu, decreases %zu, ambiguous lap counts %zu\n",
                title, t.runs, t.failed, t.laps, t.falseEntries, t.missedEntries, t.falseExits, t.falseLaps,
                t.missedLaps, t.inexact, t.decreases, t.ambiguous);
    OvalTest::printStats("road error, % of road", t.roadErrPct);
    OvalTest::printStats("road error, m", t.roadErrM);
    OvalTest::printStats("  less the GPS between transitions, m", t.roadNetM);
    OvalTest::printStats("total error (road + track runs), m", t.totalErrM);
    const char* bins[4] = {"stored entry, offset 0-5 m, s", "stored entry, offset 5-10 m, s",
                           "stored entry, offset 10-15 m, s", "stored entry, offset 15-20 m, s"};
    const char* binsM[4] = {"  and metres on the track, 0-5 m", "  and metres on the track, 5-10 m",
                            "  and metres on the track, 10-15 m", "  and metres on the track, 15-20 m"};
    for (int b = 0; b < 4; ++b) {
        if (!t.entryS[b].empty()) {
            OvalTest::printStats(bins[b], t.entryS[b]);
            OvalTest::printStats(binsM[b], t.entryM[b]);
        }
    }
    OvalTest::printStats("join placed from the true entry, s", t.joinS);
    OvalTest::printStats("exit placed from the true exit, s", t.exitS);
}

} // namespace

TEST(SessionSweep, MixedRuns)
{
    const uint32_t runs = envRuns("TRACKRUN_SESSION_RUNS", 200);
    const uint32_t seed = envU("TRACKRUN_SESSION_SEED", 1);
    Totals t;
    sweep(seed, runs, 5.0, t);
    print("mixed runs, daily offsets up to 5 m", t);
    EXPECT_EQ(t.failed, 0u);
    EXPECT_EQ(t.falseEntries, 0u);
    EXPECT_EQ(t.missedEntries, 0u);
    EXPECT_EQ(t.falseExits, 0u);
    EXPECT_EQ(t.falseLaps, 0u);
    EXPECT_EQ(t.missedLaps, 0u);
    EXPECT_EQ(t.inexact, 0u);
    EXPECT_EQ(t.decreases, 0u);
}

TEST(SessionSweep, LargeDailyOffsets)
{
    // The same invariants with a day's offset up to 20 m: only the entry
    // to a stored track may come later.
    const uint32_t runs = envRuns("TRACKRUN_SESSION_RUNS", 200);
    const uint32_t seed = envU("TRACKRUN_SESSION_SEED", 1) + 7u;
    Totals t;
    sweep(seed, runs, 20.0, t);
    print("mixed runs, daily offsets up to 20 m", t);
    EXPECT_EQ(t.failed, 0u);
    EXPECT_EQ(t.falseEntries, 0u);
    EXPECT_EQ(t.missedEntries, 0u);
    EXPECT_EQ(t.falseExits, 0u);
    EXPECT_EQ(t.falseLaps, 0u);
    EXPECT_EQ(t.missedLaps, 0u);
    EXPECT_EQ(t.inexact, 0u);
    EXPECT_EQ(t.decreases, 0u);
}

// ---------------------------------------------------------------------------
TEST(SessionSweep, DISABLED_One)
{
    const uint32_t seed = envU("TRACKRUN_SESSION_ONE", 100014);
    const double   maxOff = envU("TRACKRUN_SESSION_OFF", 5);
    SweepRun run = makeRun(seed, maxOff);
    Oval::TrackStore store;
    if (run.stored) {
        storeTrack(store, run.tk, run.offE, run.offN, run.lane);
    }
    const bool trace = std::getenv("TRACKRUN_SESSION_TRACE") != nullptr;
    const Replay::SessionResult r = Replay::runSession(
        run.tr, Oval::LaneTable::outdoor(), Oval::Config(), run.stored ? &store : nullptr,
        [&](size_t i, Session& s) {
            if (!trace || s.segment() != Session::Segment::Track) {
                return;
            }
            const Oval::Tracker& t = s.tracker();
            std::printf("  %zu t %.0f mode %d off %d left %d lat %.1f aligning %d done %d drift (%.1f, %.1f) laps %u "
                        "total %.1f\n",
                        i, Oval::msDiff(run.tr.samples[i].tMs, run.tr.startMs) / 1000.0, static_cast<int>(t.mode()),
                        t.isOffTrack(), t.hasLeft(), t.lateralOffsetM(), t.alignment().aligning, t.alignment().done,
                        t.driftOffset().x, t.driftOffset().y, t.lapsCompleted(), t.totalDistanceM());
        });
    const TraceGen::Trace& tr = run.tr;
    std::printf("%s %s lane %u noise %.1f, truth visits %zu\n", kindName(run.kind), run.loop, run.lane, run.noise,
                tr.visits.size());
    for (const TraceGen::TrackVisit& v : tr.visits) {
        std::printf("truth: join %.1f s exit %.1f s laps %zu\n", Oval::msDiff(v.joinMs, tr.startMs) / 1000.0,
                    Oval::msDiff(v.exitMs, tr.startMs) / 1000.0, v.laps);
    }
    for (size_t k = 0; k < r.changes.size(); ++k) {
        const size_t i  = r.changeSample[k];
        const size_t at = sampleAtOrAfter(tr, r.changes[k].tMs);
        std::printf("change %zu at sample %zu (time->sample %zu): road %.1f->%.1f track %.1f->%.1f total %.1f->%.1f | "
                    "truth at time: gpsRoad %.1f track %.1f | truth now: gpsRoad %.1f track %.1f\n",
                    k, i, at, r.road[i - 1], r.road[i], r.track[i - 1], r.track[i], r.total[i - 1], r.total[i],
                    tr.gpsRoadM[at], tr.trueTrackM[at], tr.gpsRoadM[i], tr.trueTrackM[i]);
    }
    std::printf("final: road %.1f track %.1f total %.1f | truth gpsRoad %.1f track %.1f\n", r.finalRoad, r.finalTrack,
                r.finalTotal, tr.gpsRoadM.back(), tr.trueTrackM.back());
    for (size_t k = 0; k < r.events.size(); ++k) {
        const Session::Event& e = r.events[k];
        std::printf("  sample %zu  %.1f s type %d trk %d lap %u len %.2f from %d to %d\n", r.eventSample[k],
                    Oval::msDiff(e.tMs, tr.startMs) / 1000.0, static_cast<int>(e.type), static_cast<int>(e.track.type),
                    e.track.lapNumber, e.track.lapLengthM, static_cast<int>(e.from), static_cast<int>(e.segment));
    }
}

TEST(SessionSweep, DISABLED_Lap2Margins)
{
    // The calibrator's fit at two laps of coverage: standard tracks (R 35 to
    // 38 m, lanes 1 to 8, noise 2 to 5 m) against park loops of 380 to 440 m.
    const uint32_t runs = envRuns("TRACKRUN_SESSION_RUNS", 300);
    for (int park = 0; park < 2; ++park) {
        std::vector<double> rr, shape, scale;
        size_t passed = 0, total = 0;
        for (uint32_t n = 0; n < runs; ++n) {
            std::mt19937 rng(PortableRandom::seeded(5000u + n + 100000u * park));
            PortableRandom::Uniform u01(0.0, 1.0);
            TraceGen::Track tk = park ? TraceGen::parkLoopTrack(380.0 + 60.0 * u01(rng)) : randomTrack(rng);
            if (park) {
                tk.headingDeg = static_cast<float>(180.0 * u01(rng));
            }
            const uint8_t lane = static_cast<uint8_t>(1 + static_cast<int>(8.0 * u01(rng)) % 8);
            TraceGen::Generator g(tk, OvalTest::noisy(static_cast<float>(2.0 + 3.0 * u01(rng))), 7000u + n);
            g.begin(static_cast<float>(u01(rng)), lane);
            g.runLaps(2.6, OvalTest::runner(static_cast<float>(2.8 + 2.5 * u01(rng))));
            const TraceGen::Trace tr = g.finish();
            bool done = false;
            Replay::runTracker(tr, Oval::LaneTable::outdoor(), Oval::Config(), nullptr, [&](size_t, Oval::Tracker& t) {
                const Oval::Calibrator::FitInfo& f = t.calibrator().lastFit();
                if (!done && f.valid && f.laps >= 2) {
                    done = true;
                    ++total;
                    rr.push_back(f.kerbRadiusM);
                    shape.push_back(f.shapeM);
                    scale.push_back(f.scaleM);
                    passed += f.failed == Oval::Calibrator::Reject::None ? 1u : 0u;
                }
            });
        }
        std::printf("[ margins  ] %s: %zu fits at lap 2, %zu passed\n", park ? "park loops" : "standard", total, passed);
        OvalTest::printStats("R", rr);
        OvalTest::printStats("shape", shape);
        OvalTest::printStats("scale", scale);
        std::sort(rr.begin(), rr.end());
        std::sort(shape.begin(), shape.end());
        std::printf("[ margins  ] R lowest %.2f, shape lowest %.2f\n", rr.empty() ? 0.0 : rr.front(), shape.empty() ? 0.0 : shape.front());
    }
}
