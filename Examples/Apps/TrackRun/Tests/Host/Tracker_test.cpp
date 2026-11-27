// Oval::Tracker: calibrated mode, the switches between modes and lap timing,
// and drift tracking.
//
// Every run goes through Replay::runTracker(), which checks after every
// reading that the total never decreased.

#include "OvalTestUtil.hpp"

#include "Oval/Tracker.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace OvalTest;
using Oval::LaneTable;
using Oval::Tracker;
using TraceGen::Generator;
using TraceGen::Trace;

namespace {

constexpr float    kTailM   = 40.0f;
constexpr uint32_t kSeeds[] = {11, 23, 37, 41, 59};

using Mode = Tracker::Mode;

// Shown total minus the physical truth, at every reading in calibrated mode
// from reading `from` on.
std::vector<double> partLapErrors(const Trace& tr, const Replay::TrackerResult& r, size_t from = 0)
{
    std::vector<double> e;
    for (size_t i = std::max(from, tr.startIndex); i < tr.samples.size(); ++i) {
        if (r.mode[i] == static_cast<uint8_t>(Mode::Calibrated)) {
            e.push_back(std::fabs(r.total[i] - tr.truePhysM[i]));
        }
    }
    return e;
}

size_t countType(const Replay::TrackerResult& r, Tracker::EventType t)
{
    size_t n = 0;
    for (const auto& e : r.events) {
        n += e.type == t ? 1 : 0;
    }
    return n;
}

// Laps counted one for one against the truth, each ending near the true
// pass of the start line, and each credited its exact lane length (or the
// physical length when lane changes are allowed).
::testing::AssertionResult lapsRight(const Trace& tr, const Replay::TrackerResult& r, uint32_t endTolMs,
                                     bool physical = false, double tolM = 0.01)
{
    if (r.decreases != 0) {
        return ::testing::AssertionFailure() << "total went down " << r.decreases << " times, worst by "
                                             << r.worstDecrease << " m";
    }
    if (r.laps.size() != tr.crossMs.size()) {
        return ::testing::AssertionFailure() << "counted " << r.laps.size() << " laps, truth " << tr.crossMs.size();
    }
    for (size_t k = 0; k < r.laps.size(); ++k) {
        const double endErr = std::fabs(static_cast<double>(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])));
        if (endErr > endTolMs) {
            return ::testing::AssertionFailure() << "lap " << k + 1 << " ends " << endErr << " ms from the truth";
        }
        const double expected = physical ? tr.lapPhysM[k] : tr.lapCreditM[k];
        if (std::fabs(r.laps[k].lapLengthM - expected) > tolM) {
            return ::testing::AssertionFailure() << "lap " << k + 1 << " credited " << r.laps[k].lapLengthM
                                                 << " m, expected " << expected << " m";
        }
    }
    return ::testing::AssertionSuccess();
}

Oval::Config defaultCfg()
{
    return Oval::Config();
}

} // namespace

// ---------------------------------------------------------------------------
// From scratch: counting, then calibrated after a lap or so
// ---------------------------------------------------------------------------

TEST(Tracker, CleanRunCalibratesAndLocks)
{
    TraceGen::Track track;
    track.headingDeg = 40.0f;
    Generator g(track, clean(), 1);
    g.begin(0.1f, 1);
    g.runLaps(6, steady(5.0f));
    g.run(kTailM, steady(5.0f));
    const Trace tr = g.finish();
    const Replay::TrackerResult r = Replay::runTracker(tr);

    EXPECT_TRUE(lapsRight(tr, r, 500));
    EXPECT_NE(r.provisionalAt, SIZE_MAX);
    EXPECT_NE(r.lockedAt, SIZE_MAX);
    EXPECT_NE(r.calibratedAt, SIZE_MAX);
    EXPECT_EQ(r.finalMode, Mode::Calibrated);
    EXPECT_EQ(r.finalStage, Tracker::CalStage::Locked);
    EXPECT_NEAR(r.laps.back().totalM, 2400.0f, 0.01f);
    EXPECT_NEAR(r.lockedModel.kerbRadiusM(), 36.5f, 0.1f);
    EXPECT_TRUE(r.lockedModel.hasGeo());
    EXPECT_NEAR(r.lockedModel.centreLat(), track.centreLat, 1e-6);
    EXPECT_NEAR(r.lockedModel.altitudeM(), 50.0f, 0.2f);
    const std::vector<double> e = partLapErrors(tr, r);
    EXPECT_LT(percentile(e, 1.0), 1.5) << "clean part-lap error";
}

// ---------------------------------------------------------------------------
// A lane change during the calibration laps
// ---------------------------------------------------------------------------

TEST(Tracker, LaneChangeDuringCalibrationStillLocks)
{
    for (uint32_t seed : kSeeds) {
        TraceGen::Track track;
        track.headingDeg  = 11.0f * seed;
        track.kerbRadiusM = 35.5f + 0.05f * seed;
        Generator g(track, noisy(3.0f), seed);
        g.begin(0.2f, 1);
        g.runLaps(1.6, runner());
        g.changeLane(4);
        g.runLaps(5.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr);
        EXPECT_NE(r.lockedAt, SIZE_MAX) << "seed " << seed;
        EXPECT_TRUE(lapsRight(tr, r, 5000, true, 3.0)) << "seed " << seed;
        const Oval::Vec2 c = r.frame.toLocal(track.centreLat, track.centreLon);
        // Where the drifting GPS put the track over the calibration points
        // (Calibrator_test's centreGpsM): reported beside the true centre.
        double offE = 0.0, offN = 0.0;
        size_t offK = 0;
        for (size_t i = tr.startIndex; i < tr.samples.size() && i <= r.lockedAt; ++i, ++offK) {
            offE += tr.offsetE[i];
            offN += tr.offsetN[i];
        }
        const Oval::Vec2 cGps{c.x + static_cast<float>(offK ? offE / offK : 0.0),
                              c.y + static_cast<float>(offK ? offN / offK : 0.0)};
        std::printf("[ lanecal  ] seed %u: locked centre %.2f m from the true centre, %.2f m from where the GPS put it\n",
                    seed, Oval::length(r.lockedModel.centre() - c), Oval::length(r.lockedModel.centre() - cGps));
        // Against where the GPS put the track, as the calibrator's own sweeps
        // assert it: with drift on, no fit can recover a position
        // the GPS drifted away from, and the true centre was a lucky draw.
        EXPECT_LT(Oval::length(r.lockedModel.centre() - cGps), 1.5f) << "seed " << seed;
        EXPECT_LT(std::fabs(r.lockedModel.kerbRadiusM() - track.kerbRadiusM), 1.2f) << "seed " << seed;
    }
}

// ---------------------------------------------------------------------------
// Non-standard shapes fall back to counting mode
// ---------------------------------------------------------------------------

TEST(Tracker, NonStandardShapesFallBackToCountingMode)
{
    struct Case {
        const char*     name;
        TraceGen::Track track;
        bool            lap400;
    };
    const Case cases[] = {{"300 m track", TraceGen::shortTrack(300.0), false},
                          {"park loop", TraceGen::parkLoopTrack(), true},
                          {"400 m circle", TraceGen::circleTrack(), true},
                          {"double bend", TraceGen::doubleBendTrack(), true}};
    for (const Case& c : cases) {
        for (uint32_t seed : kSeeds) {
            for (float sigma : {2.0f, 4.0f}) {
                TraceGen::Track track = c.track;
                track.headingDeg = 13.0f * seed;
                Generator g(track, noisy(sigma), seed);
                g.begin(0.05f * seed, 1);
                g.runLaps(8, runner());
                g.run(kTailM, runner());
                const Trace tr = g.finish();
                const Replay::TrackerResult r = Replay::runTracker(tr);
                EXPECT_NE(r.rejectedAt, SIZE_MAX) << c.name << " seed " << seed;
                EXPECT_EQ(r.finalMode, Mode::NotStandard) << c.name << " seed " << seed;
                EXPECT_EQ(r.finalStage, Tracker::CalStage::Rejected) << c.name;
                EXPECT_EQ(r.decreases, 0u) << c.name;
                ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << c.name << " seed " << seed;
                // Whole laps are counting mode's: one lane-1 lap each.
                EXPECT_NEAR(r.laps.back().totalM, 400.0f * r.laps.size(), 0.01f) << c.name;
                if (c.lap400) {
                    // On these the lap really is 400 m, so the total is exact.
                    EXPECT_NEAR(r.laps.back().totalM, Replay::creditedAfter(tr, r.laps.size()), 0.01) << c.name;
                }
                if (r.calibratedAt != SIZE_MAX) {
                    std::printf("[ shapes   ] %s seed %u: calibrated for a while before the rejection\n", c.name, seed);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Starting anywhere with a stored model
// ---------------------------------------------------------------------------

TEST(Tracker, StartingAnywhereWithASuppliedModel)
{
    TraceGen::Track track;
    track.headingDeg  = 63.0f;
    track.kerbRadiusM = 37.2f;
    const Oval::TrackModel stored = Replay::truthModel(track);
    // 0, 100, 200 and 300 m into the lap, and the middle of the first bend.
    const double starts[] = {0.0, 100.0, 200.0, 300.0, 142.0};
    std::vector<double> finalErr;
    for (double startM : starts) {
        for (uint32_t seed : kSeeds) {
            for (int laneNo : {1, 5}) {
                const uint8_t lane = static_cast<uint8_t>(laneNo);
                Generator g(track, metresAccuracy(noisy(3.0f)), seed);
                g.begin(static_cast<float>(startM / g.laneLengthM(lane)), lane);
                g.runLaps(3, runner());
                g.run(150.0, runner());
                const Trace tr = g.finish();
                const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
                EXPECT_TRUE(lapsRight(tr, r, 3000)) << "start " << startM << " seed " << seed << " lane " << int(lane);
                EXPECT_EQ(r.finalMode, Mode::Calibrated);
                EXPECT_EQ(r.finalStage, Tracker::CalStage::Supplied);
                EXPECT_EQ(countType(r, Tracker::EventType::ModeChanged), 0u);
                // Each run's final total is one part-lap reading: within twice
                // the 5 m target, and the runs' 95th percentile within 6 m.
                EXPECT_NEAR(r.finalTotal, tr.truePhysM.back(), 10.0)
                    << "start " << startM << " seed " << seed << " lane " << int(lane);
                finalErr.push_back(std::fabs(r.finalTotal - tr.truePhysM.back()));
            }
        }
    }
    printStats("final total |error| (m), 50 runs", finalErr);
    EXPECT_LE(percentile(finalErr, 0.95), 6.0) << "final total, 95th percentile";
}

// ---------------------------------------------------------------------------
// Part-lap accuracy in calibrated mode
// ---------------------------------------------------------------------------

// 20 runs at one noise level, each with the true track supplied and from
// scratch: the part-lap errors of both.
static void partLapSweep(float sigma, std::vector<double>& supplied, std::vector<double>& scratch)
{
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        TraceGen::Track track;
        track.headingDeg  = 17.0f * seed;
        track.kerbRadiusM = 35.0f + 0.15f * seed;
        const uint8_t lane = static_cast<uint8_t>(1 + seed % 8);
        const Oval::TrackModel stored = Replay::truthModel(track);
        for (int fromScratch = 0; fromScratch < 2; ++fromScratch) {
            Generator g(track, noisy(sigma), seed * 7 + fromScratch);
            g.begin(0.03f * seed, lane);
            g.runLaps(8, runner(3.0f + 0.2f * seed));
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(),
                                                               fromScratch ? nullptr : &stored);
            EXPECT_TRUE(lapsRight(tr, r, 5000)) << "sigma " << sigma << " seed " << seed;
            const std::vector<double> e = partLapErrors(tr, r);
            (fromScratch ? scratch : supplied).insert((fromScratch ? scratch : supplied).end(), e.begin(), e.end());
        }
    }
}

TEST(Tracker, PartLapAccuracyAcrossNoise)
{
    // Laps exact on every run (partLapSweep), and the part-lap's 95th
    // percentile within a limit at each noise: the suite's draws come out
    // about 15% inside them.
    const float  sigmas[]       = {2.0f, 3.0f, 4.0f, 5.0f};
    const double storedLimit[]  = {4.3, 5.9, 7.6, 9.4};
    const double scratchLimit[] = {5.0, 6.4, 8.0, 14.5};
    for (size_t i = 0; i < sizeof(sigmas) / sizeof(sigmas[0]); ++i) {
        const float sigma = sigmas[i];
        std::vector<double> supplied, scratch;
        partLapSweep(sigma, supplied, scratch);
        char label[64];
        std::snprintf(label, sizeof label, "part-lap, stored model, %.0f m", sigma);
        printStats(label, supplied);
        std::snprintf(label, sizeof label, "part-lap, from scratch, %.0f m", sigma);
        printStats(label, scratch);
        EXPECT_LE(percentile(supplied, 0.95), storedLimit[i]) << "stored model, noise " << sigma << " m";
        EXPECT_LE(percentile(scratch, 0.95), scratchLimit[i]) << "from scratch, noise " << sigma << " m";
    }
}

// Targets not yet reached (ctest label "targets", not in the default run;
// docs/OVAL.md): part-lap within 5 m at the 95th percentile at 3 m noise
// when calibrating from scratch, on the same runs as above; and the same with
// the stored model, which comes out at 4.6 to 6.3 m over four seed sets of these 20 runs.
TEST(Targets, PartLapStoredModelWithin5mAt3mNoise)
{
    std::vector<double> supplied, scratch;
    partLapSweep(3.0f, supplied, scratch);
    printStats("part-lap, stored model, 3 m", supplied);
    EXPECT_LE(percentile(supplied, 0.95), 5.0) << "95th percentile at 3 m noise, stored model";
}

TEST(Targets, PartLapFromScratchWithin5mAt3mNoise)
{
    std::vector<double> supplied, scratch;
    partLapSweep(3.0f, supplied, scratch);
    printStats("part-lap, from scratch, 3 m", scratch);
    EXPECT_LE(percentile(scratch, 0.95), 5.0) << "95th percentile at 3 m noise, from scratch";
}

// ---------------------------------------------------------------------------
// Rep endings
// ---------------------------------------------------------------------------

TEST(Tracker, RepEndingsMidLap)
{
    std::vector<double> err200, err300;
    for (uint32_t seed = 1; seed <= 30; ++seed) {
        TraceGen::Track track;
        track.headingDeg = 23.0f * seed;
        const Oval::TrackModel stored = Replay::truthModel(track);
        Generator g(track, noisy(3.0f), seed);
        g.begin(0.37f, 1 + seed % 4);
        g.runLaps(2, runner(4.0f + 0.1f * seed));
        const Trace tr = g.finish();

        // Reps of 200 m and 300 m from Start, then 600 m and 700 m (the
        // second lap): each asked a few seconds after it was reached, as
        // the interval engine would.
        const double targets[] = {200.0, 300.0, 600.0, 700.0};
        bool         asked[4]  = {};
        double       got[4]    = {};
        Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored,
                           [&](size_t i, const Tracker& t) {
                               for (int k = 0; k < 4; ++k) {
                                   if (asked[k] || t.totalDistanceM() < targets[k] + 15.0) {
                                       continue;
                                   }
                                   uint32_t at = 0;
                                   asked[k] = true;
                                   got[k] = t.timeOfDistance(static_cast<float>(targets[k]), at)
                                       ? static_cast<double>(Oval::msDiff(at, tr.startMs)) : -1.0;
                                   (void)i;
                               }
                           });
        for (int k = 0; k < 4; ++k) {
            ASSERT_TRUE(asked[k]) << "seed " << seed;
            ASSERT_GE(got[k], 0.0) << "seed " << seed << " target " << targets[k];
            const double truth = Replay::trueTimeOfPhys(tr, targets[k]);
            ((k % 2 == 0) ? err200 : err300).push_back(std::fabs(got[k] - truth) / 1000.0);
        }
    }
    printStats("rep end 200 m, |error| (s)", err200);
    printStats("rep end 300 m, |error| (s)", err300);
    EXPECT_LT(percentile(err200, 0.95), 1.0);
    EXPECT_LT(percentile(err300, 0.95), 1.0);
}

// ---------------------------------------------------------------------------
// A physical lane change in calibrated mode
// ---------------------------------------------------------------------------

TEST(Tracker, LaneChange1To8AtHalfALapCreditsThePhysicalDistance)
{
    TraceGen::Track track;
    track.headingDeg = 101.0f;
    const Oval::TrackModel stored = Replay::truthModel(track);
    std::vector<double> straightErr;
    for (double startFrac : {0.0, 0.1, 0.3, 0.6}) {
        for (uint32_t seed : kSeeds) {
            Generator g(track, noisy(3.0f), seed);
            g.begin(static_cast<float>(startFrac), 1);
            g.runLaps(1, runner());
            g.runLaps(0.5, runner());
            g.changeLane(8);
            g.runLaps(0.5 * 453.03 / 400.0 + 1.0, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
            ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << "start " << startFrac << " seed " << seed;
            EXPECT_EQ(r.decreases, 0u);
            // The lap with the change: physical, not the fraction formula.
            std::printf("[ lane 1-8 ] start %.1f seed %2u: lap 2 credited %.2f m, physical %.2f m, formula %.2f m\n",
                        startFrac, seed, r.laps[1].lapLengthM, tr.lapPhysM[1], tr.lapCreditM[1]);
            // Half a lap from a start at 0.1 is on the back straight; from 0
            // it is the exit of the first bend, where the lane takes effect at
            // a reading, up to a second early, on lane 8's bend (0.24 m more a
            // metre than lane 1's). From 0.3 and 0.6 the change is on a bend,
            // where the runner's 6 s drift across is counted on lane 8's line
            // from the moment it is set: reported, not asserted. Within 1.5 m
            // a run (one reading of lane 8's bend at 6 m/s), and within 0.5 m
            // on average, so a bias still shows (1.0 m held on one draw only).
            if (startFrac < 0.2) {
                EXPECT_NEAR(r.laps[1].lapLengthM, tr.lapPhysM[1], 1.5) << "start " << startFrac << " seed " << seed;
                straightErr.push_back(r.laps[1].lapLengthM - tr.lapPhysM[1]);
            }
            EXPECT_NEAR(r.laps[0].lapLengthM, 400.0f, 0.01f);
            EXPECT_NEAR(r.laps[2].lapLengthM, 453.03f, 0.01f);
        }
    }
    double sum = 0.0;
    for (double e : straightErr) {
        sum += e;
    }
    const double mean = straightErr.empty() ? 0.0 : sum / static_cast<double>(straightErr.size());
    std::printf("[ lane 1-8 ] start 0 and 0.1: credited - physical, mean %+.2f m over %zu runs\n", mean,
                straightErr.size());
    EXPECT_LE(std::fabs(mean), 0.5) << "mean of credited - physical";
}

// ---------------------------------------------------------------------------
// The switch from counting to calibrated mode mid-run
// ---------------------------------------------------------------------------

TEST(Tracker, MidRunSwitchNeverDoubleCountsOrGoesBack)
{
    float worstHold = 0.0f;
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        TraceGen::Track track;
        track.headingDeg  = 9.0f * seed;
        track.kerbRadiusM = 35.0f + 0.075f * seed;
        TraceGen::Noise n = noisy(2.0f + 0.075f * seed);
        n.distScale = 1.0f + 0.005f * static_cast<float>(seed % 20); // GPS distance over-reads up to 10%
        Generator g(track, n, seed);
        g.begin(0.025f * seed, 1 + seed % 8);
        g.runLaps(6, runner(3.0f + 0.1f * seed));
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr);
        EXPECT_TRUE(lapsRight(tr, r, 5000)) << "seed " << seed;
        EXPECT_NE(r.calibratedAt, SIZE_MAX) << "seed " << seed;
        worstHold = std::max(worstHold, r.largestHold);
    }
    std::printf("[ switch   ] largest hold over 40 runs: %.1f m\n", worstHold);
}

// ---------------------------------------------------------------------------
// Leaving the track and rejoining it
// ---------------------------------------------------------------------------

TEST(Tracker, LeavingAndRejoiningDoesNotCreditTheDetour)
{
    TraceGen::Track track;
    track.headingDeg = 31.0f;
    const Oval::TrackModel stored = Replay::truthModel(track);
    for (uint32_t seed : kSeeds) {
        for (double ahead : {0.0, 60.0}) {
            Generator g(track, metresHdop(noisy(3.0f)), seed);
            g.begin(0.05f, 2);
            g.runLaps(1.3, runner());
            g.leaveTrack(80.0f, 3.0f, ahead); // over 40 s more than 15 m out
            g.runLaps(2.0, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
            EXPECT_EQ(r.decreases, 0u);
            ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << "seed " << seed << " ahead " << ahead;
            EXPECT_EQ(countType(r, Tracker::EventType::ModeChanged), 2u) << "to counting and back";
            EXPECT_EQ(r.finalMode, Mode::Calibrated);
            // The detour, and the track it skipped, are not credited: the
            // total follows the metres really run on the track. Leaving is
            // judged beyond the lanes, so a lane 2 runner turning off along
            // the track is credited a little of the way out to their edge.
            EXPECT_NEAR(r.finalTotal, tr.truePhysM.back(), 9.0) << "seed " << seed << " ahead " << ahead;
            EXPECT_NEAR(r.laps.back().lapLengthM, 407.04f, 0.01f) << "the lap after rejoining is exact";
        }
    }
}

TEST(Tracker, ShortDetourIsFrozenAndNotCredited)
{
    TraceGen::Track track;
    const Oval::TrackModel stored = Replay::truthModel(track);
    for (uint32_t seed : kSeeds) {
        Generator g(track, noisy(3.0f), seed);
        g.begin(0.05f, 1);
        g.runLaps(1.3, runner());
        g.detour(40.0f, 3.0f); // about 17 s beyond 15 m: frozen, still calibrated
        g.runLaps(1.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
        EXPECT_TRUE(lapsRight(tr, r, 3000)) << "seed " << seed;
        EXPECT_EQ(countType(r, Tracker::EventType::ModeChanged), 0u);
        EXPECT_NEAR(r.finalTotal, tr.truePhysM.back(), 6.0) << "seed " << seed;
    }
}

TEST(Tracker, ALaneChangeOnTheLineBeforeCalibrationAddsNoLap)
{
    // From scratch: lap 1 in lane 1, a pause on the line with a lane change
    // to lane 2 from the pause menu, then on. Calibrated mode takes over in
    // lap 2 and re-measures it lane by lane from the marks: a mark a little
    // behind the line must count as none, not as nearly a lap.
    TraceGen::Track track;
    size_t worst = 0;
    double worstErr = 0.0;
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        Generator g(track, noisy(3.0f), 9100u + seed);
        g.begin(0.0f, 1);
        g.runLaps(1.0, runner());
        g.pause();
        g.stand(20.0);
        g.changeLane(2, 1.5f, true);
        g.resume();
        g.runLaps(2.5, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), nullptr);
        const double err = std::fabs(r.finalTotal - tr.truePhysM.back());
        if (err > worstErr) {
            worstErr = err;
            worst    = seed;
        }
    }
    EXPECT_LT(worstErr, 60.0) << "seed " << worst << ": the total is off by about a lap";
}

TEST(Tracker, ALapPressOffTheTrackLeavesTheTotal)
{
    // Calibrated on a stored track, the runner steps 60 m off it mid-lap,
    // stands, and presses Lap: the press restarts the lap and moves no
    // distance (the part-lap was counted once, not again from counting mode).
    TraceGen::Track track;
    const Oval::TrackModel stored = Replay::truthModel(track);
    for (uint32_t seed : kSeeds) {
        Generator g(track, noisy(2.0f), seed);
        g.begin(0.05f, 1);
        g.runLaps(1.3, runner());
        g.leaveToRoad();
        g.road({g.trackPointAt(g.tauNow(), 1, 60.0)}, runner(2.0f));
        g.stand(20.0);
        g.lapPress();
        g.stand(20.0);
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
        size_t press = SIZE_MAX;
        for (const TraceGen::Command& c : tr.commands) {
            if (c.kind == TraceGen::CmdKind::LapPress) {
                press = c.beforeSample;
            }
        }
        ASSERT_NE(press, SIZE_MAX);
        ASSERT_GT(press, tr.startIndex + 1u);
        ASSERT_LT(press + 5u, r.total.size());
        // Standing: a few metres of GPS noise at most either side.
        EXPECT_NEAR(r.total[press + 5u], r.total[press - 1u], 5.0)
            << "seed " << seed << ": the press moved the total";
    }
}

// ---------------------------------------------------------------------------
// Lap time refinement
// ---------------------------------------------------------------------------

TEST(Tracker, RefinedLapTimesBeatProvisionalOnes)
{
    std::vector<double> prov[2], ref[2], delay, delayReported;
    for (int fromScratch = 0; fromScratch < 2; ++fromScratch) {
        for (uint32_t seed = 1; seed <= 20; ++seed) {
            TraceGen::Track track;
            track.headingDeg = 19.0f * seed;
            const Oval::TrackModel stored = Replay::truthModel(track);
            Generator g(track, noisy(3.0f + 0.1f * (seed % 10)), seed);
            g.begin(0.04f * seed, 1 + seed % 8);
            g.runLaps(8, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::TrackerResult r =
                Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), fromScratch ? nullptr : &stored);
            ASSERT_TRUE(lapsRight(tr, r, 5000)) << "seed " << seed;
            const Replay::TimeErrors te = Replay::lapTimeErrors(tr, r);
            for (double e : te.provisional) {
                prov[fromScratch].push_back(std::fabs(e) / 1000.0);
            }
            for (double e : te.refined) {
                ref[fromScratch].push_back(std::fabs(e) / 1000.0);
            }
            for (double d : te.refineDelayMs) {
                delay.push_back(d / 1000.0);
            }
            for (double d : te.refineAfterLapMs) {
                delayReported.push_back(d / 1000.0);
            }
        }
    }
    printStats("lap time, stored, provisional (s)", prov[0]);
    printStats("lap time, stored, refined (s)", ref[0]);
    printStats("lap time, scratch, provisional (s)", prov[1]);
    printStats("lap time, scratch, refined (s)", ref[1]);
    printStats("refined event after true crossing (s)", delay);
    printStats("refined event after crossing as reported (s)", delayReported);
    for (int k = 0; k < 2; ++k) {
        double mp = 0.0, mr = 0.0;
        for (double v : prov[k]) mp += v;
        for (double v : ref[k]) mr += v;
        EXPECT_LT(mr, mp) << "mean refined error below provisional";
        EXPECT_LT(percentile(ref[k], 0.95), percentile(prov[k], 0.95));
    }
    // "Within about 4 s of the crossing": at most 4 s after the crossing the
    // watch detected (readings come once a second, the fit waits 3 s). The
    // delay after the true crossing is printed above; it adds the error of
    // the first, two-reading time.
    EXPECT_LE(percentile(delayReported, 1.0), 4.0);
    EXPECT_LE(percentile(delay, 0.95), 4.6);
}

// ---------------------------------------------------------------------------
// Drift tracking
// ---------------------------------------------------------------------------

TEST(Tracker, GrowingDriftAfterLockIsTracked)
{
    std::vector<double> early, late;
    for (uint32_t seed : kSeeds) {
        TraceGen::Track track;
        track.headingDeg = 29.0f * seed;
        TraceGen::Noise n = noisy(3.0f, 0.03f);
        // 0.012 m/s: about 18 m in 25 minutes, more than the 15 m off-track limit.
        n.driftRateEMps = 0.010f;
        n.driftRateNMps = -0.0066f;
        Generator g(track, n, seed);
        g.begin(0.1f, 3);
        g.runLaps(20, runner(5.0f)); // about 27 minutes
        g.leaveTrack(80.0f, 3.0f, 0.0);
        g.runLaps(1.5, runner(5.0f));
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr);
        ASSERT_NE(r.lockedAt, SIZE_MAX) << "seed " << seed;
        EXPECT_EQ(r.decreases, 0u);
        ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << "seed " << seed;
        // Never frozen as off the track while running round it: the first
        // drop to counting mode is the deliberate detour at the end.
        size_t firstCounting = SIZE_MAX;
        for (size_t i = r.lockedAt; i < tr.samples.size(); ++i) {
            if (r.mode[i] == static_cast<uint8_t>(Mode::Counting)) {
                firstCounting = i;
                break;
            }
        }
        const double driftAtEnd = std::hypot(tr.offsetE.back(), tr.offsetN.back());
        EXPECT_GT(driftAtEnd, 15.0) << "the drift outgrows the off-track limit";
        ASSERT_NE(firstCounting, SIZE_MAX) << "the detour was caught, seed " << seed;
        EXPECT_GT(Oval::msDiff(tr.samples[firstCounting].tMs, tr.startMs), 20 * 75 * 1000)
            << "not before the detour, seed " << seed;
        // Part-lap error stays at its level just after lock.
        for (size_t i = r.lockedAt; i < firstCounting; ++i) {
            const double e   = std::fabs(r.total[i] - tr.truePhysM[i]);
            const double tSec = Oval::msDiff(tr.samples[i].tMs, tr.startMs) / 1000.0;
            (tSec < 600.0 ? early : late).push_back(e);
        }
        EXPECT_EQ(r.finalMode, Mode::Calibrated) << "rejoined after the detour";
    }
    printStats("part-lap, first 10 min after lock", early);
    printStats("part-lap, 10 min after lock on", late);
    EXPECT_LE(percentile(late, 0.95), percentile(early, 0.95) + 1.0);
}

// ---------------------------------------------------------------------------
// The everyday cases in calibrated mode
// ---------------------------------------------------------------------------

TEST(Tracker, StandingAndJoggingOnTheLineNeverCountsALap)
{
    TraceGen::Track track;
    const Oval::TrackModel stored = Replay::truthModel(track);
    for (uint32_t seed : kSeeds) {
        // A rest after easing up past the line, as in the LapCounter tests.
        for (double restAt : {10.0, 20.0}) {
            Generator g(track, noisy(4.0f), seed);
            g.begin(0.0f, 1);
            g.stand(120.0);
            g.shuttle(120.0, 15.0f, 2.0f);
            g.run(2.0 * 400.0 + restAt, runner());
            g.stand(180.0); // a rest at the line between reps
            g.runLaps(1, runner());
            g.run(kTailM, runner());
            const Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
            EXPECT_TRUE(lapsRight(tr, r, 3000)) << "seed " << seed << " rest at " << restAt;
        }
    }
}

// Resting 4 m short of the line, within the GPS noise: the lap count stays
// right, but the lap can end during the rest. Reported, not asserted (see
// docs/OVAL.md, known limits).
TEST(Tracker, RestingJustShortOfTheLine)
{
    TraceGen::Track track;
    const Oval::TrackModel stored = Replay::truthModel(track);
    std::vector<double> endErr;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        Generator g(track, noisy(4.0f), seed);
        g.begin(0.0f, 1);
        g.runLaps(1, runner());
        g.run(400.0 - 4.0, runner());
        g.stand(180.0);
        g.runLaps(1, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
        EXPECT_EQ(r.decreases, 0u);
        ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << "seed " << seed;
        endErr.push_back(std::fabs(static_cast<double>(Oval::msDiff(r.laps[1].tMs, tr.crossMs[1]))) / 1000.0);
    }
    printStats("rest 4 m short: lap 2 end |error| (s)", endErr);
}

TEST(Tracker, PauseMidLapKeepsTheLapExact)
{
    TraceGen::Track track;
    const Oval::TrackModel stored = Replay::truthModel(track);
    for (uint32_t seed : kSeeds) {
        Generator g(track, noisy(3.0f), seed);
        g.begin(0.0f, 1);
        g.runLaps(1.4, runner());
        g.pause();
        g.stand(60.0);
        g.resume();
        g.runLaps(1.0, runner());
        g.run(kTailM, runner());
        const Trace tr = g.finish();
        const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), defaultCfg(), &stored);
        EXPECT_TRUE(lapsRight(tr, r, 3000)) << "seed " << seed;
        // The paused minute is not in the lap time.
        const Replay::TimeErrors te = Replay::lapTimeErrors(tr, r);
        EXPECT_LT(worstAbs(te.refined), 2000.0);
    }
}

TEST(Tracker, RejectsInvalidLanesAndIdlesBeforeStart)
{
    Tracker t;
    EXPECT_FALSE(t.start(1000, 0, LaneTable::outdoor()));
    EXPECT_FALSE(t.start(1000, 9, LaneTable::outdoor()));
    EXPECT_FALSE(t.isRunning());
    Oval::Sample s;
    s.tMs = 2000;
    s.fix = true;
    s.lat = 55.95;
    s.lon = -3.19;
    EXPECT_EQ(t.update(s).count, 0);
    EXPECT_EQ(t.totalDistanceM(), 0.0f);
    EXPECT_TRUE(t.start(1000, 3, LaneTable::outdoor()));
    EXPECT_EQ(t.mode(), Mode::Calibrating);
    EXPECT_FALSE(t.setLane(9));
    EXPECT_TRUE(t.setLane(2));
    EXPECT_EQ(t.lane(), 2);
}

// ---------------------------------------------------------------------------
// Tuning aid, not part of the suite: part-lap and lap time error against the
// alpha-beta constants, with and without the random-walk drift. Run with
//   --gtest_also_run_disabled_tests --gtest_filter=*TuneFilter*
// ---------------------------------------------------------------------------

TEST(Tracker, DISABLED_TuneFilter)
{
    const float alphas[] = {0.3f, 0.4f, 0.5f, 0.6f};
    const float betas[]  = {0.05f, 0.12f, 0.2f};
    for (float drift : {0.1f, 0.0f}) {
        for (float a : alphas) {
            for (float b : betas) {
                Oval::Config cfg;
                cfg.abAlpha = a;
                cfg.abBeta  = b;
                std::vector<double> sup, scr, lap;
                for (uint32_t seed = 1; seed <= 20; ++seed) {
                    TraceGen::Track track;
                    track.headingDeg  = 17.0f * seed;
                    track.kerbRadiusM = 35.0f + 0.15f * seed;
                    const Oval::TrackModel stored = Replay::truthModel(track);
                    for (int fromScratch = 0; fromScratch < 2; ++fromScratch) {
                        Generator g(track, noisy(3.0f, drift), seed * 7 + fromScratch);
                        g.begin(0.03f * seed, static_cast<uint8_t>(1 + seed % 8));
                        g.runLaps(8, runner(3.0f + 0.2f * seed));
                        g.run(kTailM, runner());
                        const Trace tr = g.finish();
                        const Replay::TrackerResult r =
                            Replay::runTracker(tr, LaneTable::outdoor(), cfg, fromScratch ? nullptr : &stored);
                        const std::vector<double> e = partLapErrors(tr, r);
                        (fromScratch ? scr : sup).insert((fromScratch ? scr : sup).end(), e.begin(), e.end());
                        for (double x : Replay::lapTimeErrors(tr, r).refined) {
                            lap.push_back(std::fabs(x) / 1000.0);
                        }
                    }
                }
                std::printf("[ tune     ] drift %.1f alpha %.2f beta %.2f: part-lap p95 stored %.2f m, scratch %.2f m; "
                            "lap time p95 %.2f s\n",
                            drift, a, b, percentile(sup, 0.95), percentile(scr, 0.95), percentile(lap, 0.95));
            }
        }
    }
}
