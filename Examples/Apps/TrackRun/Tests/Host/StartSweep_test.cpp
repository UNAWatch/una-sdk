// How a run starts.
//
// A. Rolling starts: the runner is already moving when Start is pressed (the
//    simulated runner always is). Lap 1 must be measured from where the
//    runner was at the press, not from where GPS put them over the seconds
//    before it, on a stored track, on an unknown track (the Tracker from
//    scratch) and in counting mode alone.
// C. A GPS jump at Start: the primed fixes are off by 10 to 50 m (a fix still
//    settling), and the runner stands a while, sets off at once, or rolls in.
//    The start point must be corrected, and every lap counted.
//
// Figures are printed per case; the assertions are the targets.

#include "OvalTestUtil.hpp"
#include "SessionTestUtil.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/Session.hpp"
#include "Oval/Tracker.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace OvalTest;

namespace {

constexpr double kPi = 3.14159265358979;

enum class Via { Counting, Scratch, Stored, Session };

const char* viaName(Via v)
{
    switch (v) {
    case Via::Counting: return "counting";
    case Via::Scratch:  return "scratch";
    case Via::Stored:   return "stored";
    default:            return "session";
    }
}

struct Outcome {
    size_t   laps       = 0;    // counted
    size_t   trueLaps   = 0;
    bool     exact      = true; // every lap its lane's length
    double   lap1EndMs  = 0.0;  // lap 1's end less the true return to the press point (signed)
    double   worstEndMs = 0.0;  // worst |lap end - truth| over all laps
    double   excessM    = 0.0;  // the total ahead of the truth when it first moves off 0
    double   firstShownS = 0.0; // and when that was, s after the press
    double   startErrM  = 0.0;  // counting mode's start point from where the runner was at the press
    bool     rolled     = false; // counting mode judged it a rolling start
    double   initErrM   = 0.0;  // the same at Start, before any reading (the primed point)
    size_t   decreases  = 0;
};

Outcome replayVia(Via via, const TraceGen::Trace& tr, const TraceGen::Track& track, uint8_t lane)
{
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    Outcome o;
    o.trueLaps = tr.crossMs.size();
    std::vector<uint32_t> ends;
    std::vector<float>    lens;
    std::vector<float>    total;
    if (via == Via::Counting) {
        const Replay::Result r = Replay::run(tr, table);
        for (const auto& e : r.events) {
            ends.push_back(e.tMs);
            lens.push_back(e.lapLengthM);
        }
        total     = r.total;
        o.decreases = r.decreases;
    } else if (via == Via::Session) {
        const Replay::SessionResult r = Replay::runSession(tr, table);
        for (const auto& e : r.laps) {
            ends.push_back(e.tMs);
            lens.push_back(e.track.lapLengthM);
        }
        total     = r.total;
        o.decreases = r.decreases;
    } else {
        const Oval::TrackModel stored = Replay::truthModel(track);
        const Replay::TrackerResult r =
            Replay::runTracker(tr, table, Oval::Config(), via == Via::Stored ? &stored : nullptr);
        for (size_t k = 0; k < r.laps.size(); ++k) {
            ends.push_back(r.laps[k].tMs);
            lens.push_back(r.laps[k].lapLengthM);
        }
        total     = r.total;
        o.decreases = r.decreases;
    }
    o.laps = ends.size();
    for (size_t k = 0; k < ends.size() && k < tr.crossMs.size(); ++k) {
        const double e = static_cast<double>(Oval::msDiff(ends[k], tr.crossMs[k]));
        if (k == 0) {
            o.lap1EndMs = e;
        }
        o.worstEndMs = std::fmax(o.worstEndMs, std::fabs(e));
        o.exact = o.exact && std::fabs(lens[k] - table.lapLengthM(lane)) <= 0.01f;
    }
    // The display: when the total first moves off 0, how far it is ahead of
    // the metres run since the press.
    for (size_t i = tr.startIndex; i < tr.samples.size() && i < total.size(); ++i) {
        if (total[i] > 0.0f) {
            o.excessM = static_cast<double>(total[i]) - tr.trueTrackM[i];
            o.firstShownS = 0.001 * Oval::msDiff(tr.samples[i].tMs, tr.startMs);
            break;
        }
    }
    // Counting mode's start point once settled (the Tracker's is the same),
    // against where the runner was at the press in the run's own GPS: the
    // true start point moved by the GPS error then (drift and any jump).
    Oval::LapCounter c;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        c.prime(tr.samples[i]);
    }
    c.start(tr.startMs, lane, table);
    for (size_t i = tr.startIndex; i < tr.samples.size() && i < tr.startIndex + 15; ++i) {
        c.update(tr.samples[i]);
    }
    const Oval::Vec2 truth = c.frame().toLocal(tr.startLat, tr.startLon)
        + Oval::Vec2{tr.offsetE[tr.startIndex], tr.offsetN[tr.startIndex]};
    o.startErrM = Oval::length(c.startPoint() - truth);
    o.rolled    = c.rollingStart();
    o.initErrM  = Oval::length(truth); // the primed point is the frame's origin
    return o;
}

double meanOf(const std::vector<double>& v)
{
    double s = 0.0;
    for (double x : v) {
        s += x;
    }
    return v.empty() ? 0.0 : s / static_cast<double>(v.size());
}

} // namespace

// ---------------------------------------------------------------------------
// A. Rolling starts
// ---------------------------------------------------------------------------

TEST(StartSweep, RollingStartCreditsLap1FromThePress)
{
    // Lap 1's credited distance against what was run from the press: a lap is
    // always credited its lane's length, so lap 1 measured from a point d
    // metres behind the press ends d metres early. The error in metres is
    // the end's error times the speed.
    // 1.0 to 1.5 m/s: a walker, reported per speed; the figures asserted are
    // over running speeds (2.5 m/s and up).
    const float speeds[] = {1.0f, 1.2f, 1.5f, 2.5f, 3.5f, 5.0f, 6.5f, 8.0f};
    const Via   vias[]   = {Via::Stored, Via::Scratch, Via::Counting, Via::Session};
    for (Via via : vias) {
        std::vector<double> err, excess, shownS;
        size_t wrongLaps = 0, decreases = 0;
        for (float v : speeds) {
            std::vector<double> errV;
            size_t rolledV = 0, wrongV = 0;
            for (uint8_t lane : {uint8_t(1), uint8_t(8)}) {
                for (uint32_t seed = 1; seed <= 4; ++seed) {
                    TraceGen::Track track;
                    track.headingDeg = 25.0f * static_cast<float>(seed);
                    TraceGen::Generator g(track, noisy(3.0f), 700u + seed * 13u + lane);
                    g.beginRolling(0.05f + 0.2f * static_cast<float>(seed), lane, runner(v), 8.0 * v);
                    g.runLaps(4, runner(v));
                    g.run(40.0, runner(v));
                    const TraceGen::Trace tr = g.finish();
                    const Outcome o = replayVia(via, tr, track, lane);
                    wrongLaps += (o.laps != o.trueLaps || !o.exact) ? 1 : 0;
                    if (o.laps != o.trueLaps || !o.exact) {
                        std::printf("[ rolling  ]   wrong: %s %.1f m/s lane %u seed %u: %zu of %zu laps, exact %d\n",
                                    viaName(via), v, lane, seed, o.laps, o.trueLaps, o.exact ? 1 : 0);
                    }
                    wrongV += (o.laps != o.trueLaps || !o.exact) ? 1 : 0;
                    decreases += o.decreases;
                    const double e = -o.lap1EndMs * 0.001 * v; // metres credited beyond what was run
                    errV.push_back(e);
                    rolledV += o.rolled ? 1 : 0;
                    if (v >= 2.0f) {
                        err.push_back(e);
                        excess.push_back(o.excessM);
                        shownS.push_back(o.firstShownS);
                    }
                }
            }
            std::printf("[ rolling  ] %-8s %.1f m/s: judged rolling in %zu of 8, laps wrong in %zu; lap 1 over-credit "
                        "mean %+.1f m, worst |%.1f| m\n",
                        viaName(via), v, rolledV, wrongV, meanOf(errV), percentile([&] {
                            std::vector<double> a;
                            for (double x : errV) {
                                a.push_back(std::fabs(x));
                            }
                            return a;
                        }(), 1.0));
        }
        std::vector<double> absErr;
        for (double x : err) {
            absErr.push_back(std::fabs(x));
        }
        std::printf("[ rolling  ] %-8s %zu runs (running speeds): lap 1 over-credit mean %+.2f m, |95th| %.2f m; the total first "
                    "moves %.1f s after the press (95th %.1f s), ahead of the metres run by %+.1f m mean, 95th %.1f m, "
                    "worst %.1f m; wrong laps %zu\n",
                    viaName(via), err.size(), meanOf(err), percentile(absErr, 0.95), meanOf(shownS),
                    percentile(shownS, 0.95), meanOf(excess), percentile(excess, 0.95), percentile(excess, 1.0),
                    wrongLaps);
        EXPECT_EQ(wrongLaps, 0u) << viaName(via);
        EXPECT_EQ(decreases, 0u) << viaName(via);
        // Within GPS noise of the distance run from the press. Measured from
        // the primed point instead, 2.5 to 3 s of running behind it, lap 1 was
        // on a stored track a mean of +12 to +14 m, 95th percentile
        // 26 m, and the total first showed 19 to 23 m too much (95th). Lap 1's
        // own end carries the crossing's timing noise times the speed, a few
        // metres at a sprint.
        EXPECT_LT(std::fabs(meanOf(err)), 4.0) << viaName(via);
        EXPECT_LT(percentile(absErr, 0.95), 12.0) << viaName(via);
        EXPECT_LT(percentile(excess, 0.95), 10.0) << viaName(via);
    }
}

TEST(StartSweep, JoinCreditsLap1FromTheJoin)
{
    // A runner arriving at a track mid-run is already moving. A stored track
    // the run notices starts a fresh Tracker at one ring reading, at that
    // reading's own time (Session::replay); a lane the runner sets on
    // arriving at a track that is not stored starts one at the latest
    // reading. Either way lap 1 is measured from where the runner was then:
    // the primed fixes before Start play no part. Lap 1's credit against the
    // metres actually run from where the runner came onto the track, with
    // laps 2 to 4 alongside for comparison.
    std::vector<double> lap1[2], later[2];
    size_t wrong[2] = {0, 0};
    for (int stored = 0; stored < 2; ++stored) {
        for (uint32_t seed = 1; seed <= 12; ++seed) {
            std::mt19937          rng(PortableRandom::seeded(seed * 31u + 7u));
            const TraceGen::Track tk   = SessionTest::randomTrack(rng);
            const uint8_t         lane = static_cast<uint8_t>(1 + (seed % 8));
            const float           v    = 3.5f + 0.25f * static_cast<float>(seed % 10);
            TraceGen::Generator   g(tk, noisy(2.0f + 0.2f * static_cast<float>(seed % 10)), 40u + seed);
            PortableRandom::Uniform u01(0.0, 1.0);
            const double tauIn = 4.0 * u01(rng);
            TraceGen::Pt home;
            const std::vector<TraceGen::Pt> in = SessionTest::routeIn(g, tauIn, lane, 600.0, rng, home);
            g.beginAt(home, Oval::Session::kNoLane);
            g.road(in, SessionTest::jog(3.3f));
            g.joinTrack(tauIn, lane);
            if (stored == 0) {
                g.setLane(lane); // not stored: the runner sets the lane on arriving
            }
            g.runLaps(5.0, runner(v));
            g.run(40.0, runner(v));
            const TraceGen::Trace tr = g.finish();

            Oval::TrackStore store;
            if (stored != 0) {
                SessionTest::storeTrack(store, tk, 2.0, -1.5, lane);
            }
            const Replay::SessionResult r =
                Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), stored != 0 ? &store : nullptr);
            const float laneLen = Oval::LaneTable::outdoor().lapLengthM(lane);
            if (r.laps.size() != 5u || tr.crossMs.size() != 5u) {
                ++wrong[stored];
                std::printf("[ join     ]   %s seed %u: %zu laps of %zu\n", stored ? "stored" : "runner", seed,
                            r.laps.size(), tr.crossMs.size());
                continue;
            }
            // TraceGen's laps after a join count from the entry station: each
            // lap's credit beyond what was run is its end's error less the
            // lap before's, times the speed (laps credit their lane's length).
            double before = 0.0;
            for (size_t k = 0; k < 4; ++k) {
                const Oval::Session::Event& e = r.laps[k];
                const double endM = -0.001 * static_cast<double>(Oval::msDiff(e.tMs, tr.crossMs[k])) * v;
                (k == 0 ? lap1[stored] : later[stored]).push_back(endM - before);
                before = endM;
                wrong[stored] += std::fabs(e.track.lapLengthM - laneLen) > 0.01f ? 1 : 0;
            }
        }
    }
    for (int stored = 0; stored < 2; ++stored) {
        std::vector<double> a1, aL;
        for (double x : lap1[stored]) {
            a1.push_back(std::fabs(x));
        }
        for (double x : later[stored]) {
            aL.push_back(std::fabs(x));
        }
        std::printf("[ join     ] %-7s %zu runs: lap 1 credit less the metres run from the entry, mean %+.2f m, |95th| %.2f m "
                    "(laps 2 to 4: mean %+.2f m, |95th| %.2f m); runs or laps wrong %zu\n",
                    stored ? "stored" : "runner", lap1[stored].size(), meanOf(lap1[stored]), percentile(a1, 0.95),
                    meanOf(later[stored]), percentile(aL, 0.95), wrong[stored]);
        EXPECT_EQ(wrong[stored], 0u);
        // Not the rolling start's targets: lap 1 at a join starts at a
        // reading, which can be a reading or so before or after the entry.
        EXPECT_LT(std::fabs(meanOf(lap1[stored])), 8.0);
        EXPECT_LT(percentile(a1, 0.95), 15.0);
    }
}

// ---------------------------------------------------------------------------
// C. A GPS jump at Start
// ---------------------------------------------------------------------------

TEST(StartSweep, JumpsAtStartStandingAndRolling)
{
    // Jumps of 10 to 50 m in eight directions, through counting mode alone
    // and the Tracker from scratch. Standing: the runner stands 3 s after
    // Start, or sets off at once; rolling: runs in at 4 to 6 m/s.
    //
    // Every lap counted and none false, in every run. Where the start point
    // was really wrong by more than 20 m (the primed point against where the
    // runner was at the press, in the run's own GPS: the jump, and for a
    // rolling start the primed fixes' lag behind the press too, which a jump
    // back along the way can cancel), it should be corrected: counting mode's
    // start point (the Tracker's too) within kCorrectedM of the press, in at
    // least 9 runs in 10 (the misses are listed). Up to
    // staleStartM (15 m) it stays where the primed fixes put it (crossings
    // still fall inside gateRadiusM). Lap ends are reported.
    constexpr double kTargetM    = 20.0;
    constexpr double kCorrectedM = 10.0;
    const double     staleM      = Oval::Config().staleStartM;
    const float shifts[] = {10.0f, 15.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    const char* starts[] = {"stands", "at once", "rolling"};
    for (Via via : {Via::Counting, Via::Scratch}) {
        for (int how = 0; how < 3; ++how) {
            size_t runs = 0, failed = 0, falseLaps = 0, missedLaps = 0, target = 0, middle = 0, middleBetter = 0;
            size_t uncorrected = 0;
            std::vector<double> endTarget, endSmall, startErr;
            for (float shift : shifts) {
                for (int b = 0; b < 8; ++b) {
                    const double a = 2.0 * kPi * b / 8.0;
                    TraceGen::Track track;
                    track.headingDeg = 10.0f + 40.0f * static_cast<float>(b);
                    TraceGen::Noise n = noisy(2.0f + 0.25f * static_cast<float>(b % 5), 0.05f);
                    n.shiftEastM  = static_cast<float>(shift * PortableMath::cos(a));
                    n.shiftNorthM = static_cast<float>(shift * PortableMath::sin(a));
                    const uint32_t seed = 900u + static_cast<uint32_t>(how * 97 + b * 7) + static_cast<uint32_t>(shift);
                    const uint8_t  lane = static_cast<uint8_t>(1 + (b % 8));
                    const float    v    = 4.0f + 0.5f * static_cast<float>(b % 5);
                    TraceGen::Generator g(track, n, seed);
                    if (how == 2) {
                        g.beginRolling(0.1f * static_cast<float>(b), lane, runner(v), 8.0 * v);
                    } else {
                        g.begin(0.1f * static_cast<float>(b), lane);
                        if (how == 0) {
                            g.stand(3.0);
                        }
                    }
                    g.runLaps(4, runner(v));
                    g.run(40.0, runner(v));
                    const TraceGen::Trace tr = g.finish();
                    const Outcome o = replayVia(via, tr, track, lane);
                    ++runs;
                    const bool isTarget  = o.initErrM > kTargetM;
                    const bool corrected = !isTarget || o.startErrM <= kCorrectedM;
                    const bool ok = o.laps == o.trueLaps && o.exact && o.decreases == 0;
                    failed += ok ? 0 : 1;
                    uncorrected += corrected ? 0 : 1;
                    falseLaps += o.laps > o.trueLaps ? o.laps - o.trueLaps : 0;
                    missedLaps += o.trueLaps > o.laps ? o.trueLaps - o.laps : 0;
                    if (isTarget) {
                        ++target;
                        startErr.push_back(o.startErrM);
                        endTarget.push_back(o.worstEndMs);
                    } else if (o.initErrM > staleM) {
                        ++middle;
                        middleBetter += o.startErrM < o.initErrM ? 1 : 0;
                    } else {
                        endSmall.push_back(o.worstEndMs);
                    }
                    if (!ok || !corrected) {
                        std::printf("[ jump     ]   %s: %s %.0f m (start %.1f m out at first), bearing %d, lane %u, "
                                    "%.1f m/s, %.2f m noise: %zu of %zu laps, start point %.1f m out, lap 1 end %+.0f "
                                    "ms, worst end %.0f ms\n",
                                    ok ? "not corrected" : "failed", starts[how], shift, o.initErrM, b * 45, lane, v,
                                    n.posSigmaM, o.laps, o.trueLaps,
                                    o.startErrM, o.lap1EndMs, o.worstEndMs);
                    }
                }
            }
            std::printf("[ jump     ] %-8s %-7s %zu runs: %zu failed, %zu false laps, %zu missed laps. Start over 20 m "
                        "out (%zu runs, %zu not corrected): corrected to 95th %.1f m (worst %.1f m), worst lap end per "
                        "run 95th %.0f ms. "
                        "15 to 20 m out: %zu runs, %zu moved closer. Up to 15 m: worst lap end 95th %.0f ms (worst "
                        "%.0f)\n",
                        viaName(via), starts[how], runs, failed, falseLaps, missedLaps, target, uncorrected,
                        percentile(startErr, 0.95), percentile(startErr, 1.0), percentile(endTarget, 0.95), middle,
                        middleBetter, percentile(endSmall, 0.95), percentile(endSmall, 1.0));
            EXPECT_EQ(falseLaps, 0u) << viaName(via) << " " << starts[how];
            EXPECT_EQ(failed, 0u) << viaName(via) << " " << starts[how];
            EXPECT_GT(target, 0u) << viaName(via) << " " << starts[how] << ": no start over 20 m out to correct";
            // A start judged from four readings at 2 to 3 m noise: where they
            // do not agree it is left as it was (the laps still count), so a
            // few are missed; at least 9 in 10 corrected.
            EXPECT_LE(uncorrected * 10, target) << viaName(via) << " " << starts[how] << ": " << uncorrected
                                                << " of " << target << " not corrected";
        }
    }
}

namespace {

uint32_t envCount(const char* name, uint32_t dflt)
{
    return envSeed(name, dflt);
}

} // namespace

// ---------------------------------------------------------------------------
// Lap 1 at a join
// ---------------------------------------------------------------------------

TEST(StartSweep, DISABLED_JoinOne)
{
    // One run of JoinLap1AtEveryApproach: TRACKRUN_JOIN_SEED (its seed),
    // _BACK and _OUT (the approach, m), _CARPARK, _STORED (0: the runner sets
    // the lane on arriving). Lap by lap, with the truth.
    const uint32_t seed    = envCount("TRACKRUN_JOIN_SEED", 103003);
    const double   backM   = static_cast<double>(envCount("TRACKRUN_JOIN_BACK", 12));
    const double   outM    = static_cast<double>(envCount("TRACKRUN_JOIN_OUT", 22));
    const bool     carPark = envCount("TRACKRUN_JOIN_CARPARK", 0) != 0;
    const bool     stored  = envCount("TRACKRUN_JOIN_STORED", 1) != 0;
    const uint32_t n       = seed % 1000u;
    std::mt19937 rng(PortableRandom::seeded(seed));
    PortableRandom::Uniform u01(0.0, 1.0);
    const TraceGen::Track tk   = SessionTest::randomTrack(rng);
    const uint8_t         lane = static_cast<uint8_t>(1 + n % 8u);
    const float           v    = static_cast<float>(3.5 + 2.25 * u01(rng));
    const float           nzDrawn = static_cast<float>(2.0 + 2.0 * u01(rng));
    const float           nz   = envCount("TRACKRUN_JOIN_NOISE", 0) > 0
        ? 0.1f * static_cast<float>(envCount("TRACKRUN_JOIN_NOISE", 0))
        : nzDrawn;
    const double          tauIn = 4.0 * u01(rng);
    const double          inM   = carPark ? 80.0 + 40.0 * u01(rng) : 600.0;
    TraceGen::Generator   g(tk, noisy(nz), seed);
    TraceGen::Pt home;
    const std::vector<TraceGen::Pt> in = SessionTest::routeIn(g, tauIn, lane, inM, rng, home, backM, outM);
    g.beginAt(home, Oval::Session::kNoLane);
    g.road(in, SessionTest::jog(3.3f));
    g.joinTrack(tauIn, lane);
    if (!stored) {
        g.setLane(lane);
    }
    g.runLaps(5.0, runner(v));
    g.run(40.0, runner(v));
    const TraceGen::Trace tr = g.finish();
    Oval::TrackStore store;
    if (stored) {
        SessionTest::storeTrack(store, tk, 2.0, -1.5, lane);
    }
    const Replay::SessionResult r =
        Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), stored ? &store : nullptr);
    std::printf("lane %u, %.2f m/s, noise %.2f m; truth entry %.1f s\n", lane, v, nz,
                tr.visits.empty() ? 0.0 : 0.001 * Oval::msDiff(tr.visits[0].joinMs, tr.startMs));
    for (size_t k = 0; k < r.events.size(); ++k) {
        const Oval::Session::Event& e = r.events[k];
        std::printf("  sample %zu %.1f s type %d tracker type %d lap %u len %.2f\n", r.eventSample[k],
                    0.001 * Oval::msDiff(e.tMs, tr.startMs), static_cast<int>(e.type), static_cast<int>(e.track.type),
                    e.track.lapNumber, e.track.lapLengthM);
    }
    for (uint32_t c : tr.crossMs) {
        std::printf("truth: lap end %.1f s\n", 0.001 * Oval::msDiff(c, tr.startMs));
    }
}

namespace {

// Pooled over the slanting approaches and the car park starts, a lane the
// runner sets on arriving at a track not stored [0] and a stored track the
// run notices [1]: lap 1's provisional credit less the metres run (signed
// and absolute), and the runs with a lap wrong (any approach).
struct JoinSweep {
    std::vector<double> lap1[2];
    std::vector<double> abs1[2];
    size_t              wrong[2] = {0, 0};
};

JoinSweep joinApproachSweep()
{
    // JoinCreditsLap1FromTheJoin's runs, at more approaches and more draws:
    // the road's last leg comes onto the lane from 12 m back and 22 m out
    // (the default), steeply (2 m back, 22 m out), shallowly (30 m back, 8 m
    // out) or along the lane (40 m back, 1 m out); and a start with no lane
    // in a car park 80 to 120 m from the track (not stored: a stored track
    // that near Start is found from a lane at Start). A stored track the run
    // notices, and a track not stored whose lane the runner sets on arriving;
    // noise 2 to 4 m.
    //
    // Lap 1's credit against the metres run from where the runner came onto
    // the lane (as in JoinCreditsLap1FromTheJoin), provisional (the popup) and
    // refined (the lap list and the FIT file), laps 2 to 4 alongside; and the
    // entry the Session placed against the true one. The shallow approach
    // comes within 8 m of the lane for 30 m, which looks like a lane change
    // from lane 8, and along the lane the runner is on the line 40 m before
    // the route's last point: both are reported, not pooled.
    // TRACKRUN_JOIN_RUNS changes the draws per case (30).
    JoinSweep out;
    struct Approach {
        const char* name;
        double      backM;
        double      outM;
        bool        carPark;
        bool        target;
    };
    const Approach approaches[] = {
        {"12 m back, 22 m out", 12.0, 22.0, false, true},
        {"steep", 2.0, 22.0, false, true},
        {"shallow", 30.0, 8.0, false, false},
        {"along the lane", 40.0, 1.0, false, false},
        {"car park start", 12.0, 22.0, true, true},
    };
    const uint32_t runs = envNumber("TRACKRUN_JOIN_RUNS", 30);
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    for (int stored = 0; stored < 2; ++stored) {
        std::vector<double> lap1All, aAll, lap1AllR, aAllR, bandLo, bandHi;
        size_t wrongAll = 0;
        for (size_t a = 0; a < sizeof(approaches) / sizeof(approaches[0]); ++a) {
            const Approach& ap = approaches[a];
            if (ap.carPark && stored != 0) {
                // Not a join noticed: it is within storeNearM of the start.
                continue;
            }
            std::vector<double> lap1, later, a1, aL, lap1R, laterR, a1R, aLR, joinS, joinAbs, entryM, entryAbs;
            size_t wrong = 0;
            for (uint32_t n = 0; n < runs; ++n) {
                const uint32_t seed = 3000u + 1000u * static_cast<uint32_t>(a) + 100000u * static_cast<uint32_t>(stored) + n;
                std::mt19937 rng(PortableRandom::seeded(seed));
                PortableRandom::Uniform u01(0.0, 1.0);
                const TraceGen::Track tk   = SessionTest::randomTrack(rng);
                const uint8_t         lane = static_cast<uint8_t>(1 + n % 8u);
                const float           v    = static_cast<float>(3.5 + 2.25 * u01(rng));
                const float           nzDrawn = static_cast<float>(2.0 + 2.0 * u01(rng));
                // TRACKRUN_JOIN_NOISE (tenths of a metre) sets every run's noise, for study.
                const float           nz = envCount("TRACKRUN_JOIN_NOISE", 0) > 0
                    ? 0.1f * static_cast<float>(envCount("TRACKRUN_JOIN_NOISE", 0))
                    : nzDrawn;
                const double          tauIn = 4.0 * u01(rng);
                const double          inM   = ap.carPark ? 80.0 + 40.0 * u01(rng) : 600.0;
                TraceGen::Generator   g(tk, noisy(nz), seed);
                TraceGen::Pt home;
                const std::vector<TraceGen::Pt> in =
                    SessionTest::routeIn(g, tauIn, lane, inM, rng, home, ap.backM, ap.outM);
                g.beginAt(home, Oval::Session::kNoLane);
                g.road(in, SessionTest::jog(3.3f));
                g.joinTrack(tauIn, lane);
                if (stored == 0) {
                    g.setLane(lane);
                }
                g.runLaps(5.0, runner(v));
                g.run(40.0, runner(v));
                const TraceGen::Trace tr = g.finish();

                Oval::TrackStore store;
                if (stored != 0) {
                    SessionTest::storeTrack(store, tk, 2.0, -1.5, lane);
                }
                double   jLat = 0.0, jLon = 0.0;
                uint32_t jT   = 0;
                bool     haveJ = false;
                const Replay::SessionResult r = Replay::runSession(
                    tr, table, Oval::Config(), stored != 0 ? &store : nullptr, [&](size_t, Oval::Session& s) {
                        if (!haveJ && s.segment() == Oval::Session::Segment::Track && s.joinPoint(jLat, jLon)) {
                            haveJ = true;
                            jT    = s.joinTMs();
                        }
                    });
                if (haveJ && !tr.visits.empty()) {
                    // The entry the Session placed, against the true one: the
                    // station along the true kerb line (the GPS error of the
                    // moment taken out), and the time.
                    Oval::LocalFrame f;
                    f.setOrigin(tk.centreLat, tk.centreLon);
                    Oval::TrackModel truth = Replay::truthModel(tk);
                    truth.placeIn(f);
                    const size_t     at = SessionTest::sampleAtOrAfter(tr, jT);
                    const Oval::Vec2 pj = f.toLocal(jLat, jLon) - Oval::Vec2{tr.offsetE[at], tr.offsetN[at]};
                    const TraceGen::Pt e  = g.trackPointAt(tauIn, lane, 0.0);
                    const double       hh = static_cast<double>(tk.headingDeg) * kPi / 180.0;
                    const Oval::Vec2   pe{static_cast<float>(e.u * PortableMath::cos(hh) - e.v * PortableMath::sin(hh)),
                                          static_cast<float>(e.u * PortableMath::sin(hh) + e.v * PortableMath::cos(hh))};
                    const float lapK = truth.lapLengthM(0.0f);
                    float ds = truth.project(pe, 0.0f).s - truth.project(pj, 0.0f).s;
                    ds -= std::floor(ds / lapK + 0.5f) * lapK;
                    entryM.push_back(ds);
                    entryAbs.push_back(std::fabs(ds));
                    if (std::fabs(ds) > 0.1f * static_cast<float>(envCount("TRACKRUN_JOIN_SHOW", 60))) {
                        std::printf("[ join7c   ]   %s, %s, seed %u: entry %+.1f m behind the true one, %+.1f s, noise "
                                    "%.1f m\n",
                                    stored ? "stored" : "runner", ap.name, seed, ds,
                                    0.001 * Oval::msDiff(jT, tr.visits[0].joinMs), nz);
                    }
                }
                const float laneLen = table.lapLengthM(lane);
                bool bad = r.laps.size() != 5u || tr.crossMs.size() != 5u;
                for (const auto& e : r.laps) {
                    bad = bad || std::fabs(e.track.lapLengthM - laneLen) > 0.01f;
                }
                if (bad) {
                    ++wrong;
                    std::printf("[ join7c   ]   %s, %s, run %u: %zu laps of %zu\n", stored ? "stored" : "runner",
                                ap.name, n, r.laps.size(), tr.crossMs.size());
                    continue;
                }
                // Each lap's end as provisional (the popup) and as refined
                // (the lap list and the FIT file), when a refinement came.
                double before = 0.0, beforeR = 0.0;
                for (size_t k = 0; k < 4; ++k) {
                    uint32_t endR = r.laps[k].tMs;
                    for (const auto& e : r.events) {
                        if (e.type == Oval::Session::EventType::TrackLap
                            && e.track.type == Oval::Tracker::EventType::LapTimeRefined && e.runLap == r.laps[k].runLap) {
                            endR = e.tMs;
                        }
                    }
                    const double endM  = -0.001 * static_cast<double>(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])) * v;
                    const double endMR = -0.001 * static_cast<double>(Oval::msDiff(endR, tr.crossMs[k])) * v;
                    (k == 0 ? lap1 : later).push_back(endM - before);
                    (k == 0 ? a1 : aL).push_back(std::fabs(endM - before));
                    (k == 0 ? lap1R : laterR).push_back(endMR - beforeR);
                    (k == 0 ? a1R : aLR).push_back(std::fabs(endMR - beforeR));
                    if (k == 0 && ap.target) {
                        (nz < 3.0f ? bandLo : bandHi).push_back(std::fabs(endMR));
                    }
                    before  = endM;
                    beforeR = endMR;
                }
                // The join's time against the true entry.
                for (size_t k = 0; k < r.changes.size(); ++k) {
                    if (r.changes[k].segment == Oval::Session::Segment::Track && !tr.visits.empty()) {
                        joinS.push_back(0.001 * Oval::msDiff(r.changes[k].tMs, tr.visits[0].joinMs));
                        joinAbs.push_back(std::fabs(joinS.back()));
                        break;
                    }
                }
            }
            std::printf("[ join7c   ] %-7s %-20s %zu runs: lap 1 credit less the metres run from the entry, provisional "
                        "mean %+.2f m, |95th| %.2f m, worst %.2f m (laps 2 to 4: |95th| %.2f m); refined mean %+.2f m, "
                        "|95th| %.2f m, worst %.2f m (laps 2 to 4: mean %+.2f m, |95th| %.2f m); join from the true "
                        "entry median %+.1f s, |95th| %.1f s; the entry's station behind the true one mean %+.2f m, "
                        "|95th| %.2f m; runs or laps wrong %zu\n",
                        stored ? "stored" : "runner", ap.name, lap1.size(), meanOf(lap1), percentile(a1, 0.95),
                        percentile(a1, 1.0), percentile(aL, 0.95), meanOf(lap1R), percentile(a1R, 0.95),
                        percentile(a1R, 1.0), meanOf(laterR), percentile(aLR, 0.95), percentile(joinS, 0.5),
                        percentile(joinAbs, 0.95), meanOf(entryM), percentile(entryAbs, 0.95), wrong);
            wrongAll += wrong;
            if (ap.target) {
                lap1All.insert(lap1All.end(), lap1.begin(), lap1.end());
                aAll.insert(aAll.end(), a1.begin(), a1.end());
                lap1AllR.insert(lap1AllR.end(), lap1R.begin(), lap1R.end());
                aAllR.insert(aAllR.end(), a1R.begin(), a1R.end());
            }
        }
        std::printf("[ join7c   ] %-7s slanting approaches and car park starts, %zu runs: lap 1 provisional mean %+.2f m, "
                    "|95th| %.2f m; refined mean %+.2f m, |95th| %.2f m; refined |95th| at noise 2 to 3 m %.2f m (%zu "
                    "runs), 3 to 4 m %.2f m (%zu)\n",
                    stored ? "stored" : "runner", lap1All.size(), meanOf(lap1All), percentile(aAll, 0.95),
                    meanOf(lap1AllR), percentile(aAllR, 0.95), percentile(bandLo, 0.95), bandLo.size(),
                    percentile(bandHi, 0.95), bandHi.size());
        out.wrong[stored] = wrongAll;
        out.lap1[stored]  = lap1All;
        out.abs1[stored]  = aAll;
    }
    return out;
}

} // namespace

TEST(StartSweep, JoinLap1AtEveryApproach)
{
    // Lap 1 at a join is credited from where the runner came onto their line:
    // a stored track the run notices from Session::entryAt, a lane the runner
    // sets from the latest reading. On average within 2 m of the metres run;
    // at the 95th percentile within 12 m, GPS noise on the few readings
    // there (the target of 5 m is Targets.JoinLap1Within5mAt95th). At most
    // one run with a lap wrong.
    const JoinSweep s = joinApproachSweep();
    for (int stored = 0; stored < 2; ++stored) {
        ASSERT_FALSE(s.lap1[stored].empty()) << (stored ? "stored" : "runner") << ": no lap 1 measured";
        EXPECT_LE(s.wrong[stored], 1u) << (stored ? "stored" : "runner");
        EXPECT_LE(std::fabs(meanOf(s.lap1[stored])), 2.0) << (stored ? "stored" : "runner");
        EXPECT_LE(percentile(s.abs1[stored], 0.95), 12.0) << (stored ? "stored" : "runner");
    }
}

// Target not yet reached (ctest label "targets", not in the default run):
// lap 1 at a join within 5 m at the 95th percentile.
TEST(Targets, JoinLap1Within5mAt95th)
{
    const JoinSweep s = joinApproachSweep();
    for (int stored = 0; stored < 2; ++stored) {
        ASSERT_FALSE(s.abs1[stored].empty()) << (stored ? "stored" : "runner") << ": no lap 1 measured";
        EXPECT_LE(percentile(s.abs1[stored], 0.95), 5.0) << (stored ? "stored" : "runner");
    }
}
