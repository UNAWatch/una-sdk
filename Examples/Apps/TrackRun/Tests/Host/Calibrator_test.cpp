// Oval::Calibrator: the template fit, on several track shapes. The trace is fed straight to the calibrator here, in a
// local frame around the start point as the Tracker does; Tracker_test runs
// the same through the Tracker.

#include "OvalTestUtil.hpp"

#include "Oval/Calibrator.hpp"
#include "Oval/LaneTable.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace OvalTest;
using Oval::Calibrator;
using Oval::LaneTable;
using Oval::TrackModel;
using TraceGen::Generator;
using TraceGen::Trace;

namespace {

struct FitError {
    float centreM  = 0.0f;
    float centreGpsM = 0.0f; // against the truth moved by the mean GPS drift so far
    float headingDeg = 0.0f;
    float radiusM  = 0.0f;
    float radiusSignedM = 0.0f;
};

struct CalRun {
    bool       provisional = false;
    bool       locked      = false;
    bool       rejected    = false;
    Calibrator::Reject reason = Calibrator::Reject::None;
    uint8_t    lockLaps    = 0;
    FitError   atProvisional;
    FitError   atLock;
    TrackModel lockModel;
    Calibrator::FitInfo lastFit;
    std::vector<Calibrator::FitInfo> fits;
};

FitError errorOf(const TrackModel& m, const TraceGen::Track& truth, const Oval::LocalFrame& f,
                 Oval::Vec2 meanDrift)
{
    FitError e;
    const Oval::Vec2 c = f.toLocal(truth.centreLat, truth.centreLon);
    e.centreM    = Oval::length(m.centre() - c);
    e.centreGpsM = Oval::length(m.centre() - (c + meanDrift));
    float dh = std::fmod(std::fabs(m.headingDeg() - truth.headingDeg), 180.0f);
    if (dh > 90.0f) {
        dh = 180.0f - dh;
    }
    e.headingDeg = dh;
    e.radiusM    = std::fabs(m.kerbRadiusM() - truth.kerbRadiusM);
    e.radiusSignedM = m.kerbRadiusM() - truth.kerbRadiusM;
    return e;
}

// Feeds every good reading after Start to a calibrator, in a frame around the
// true start point, with the lane the runner had set.
CalRun calibrate(const Trace& tr, const TraceGen::Track& truth, const Oval::Config& cfg = Oval::Config())
{
    CalRun out;
    Oval::LocalFrame f;
    f.setOrigin(tr.startLat, tr.startLon);
    Calibrator cal(cfg);
    cal.reset(LaneTable::outdoor());
    uint8_t lane = tr.startLane;
    size_t  nextCmd = 0;
    double  dx = 0.0, dy = 0.0;
    size_t  dn = 0;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        for (; nextCmd < tr.commands.size() && tr.commands[nextCmd].beforeSample <= i; ++nextCmd) {
            if (tr.commands[nextCmd].kind == TraceGen::CmdKind::SetLane) {
                lane = tr.commands[nextCmd].lane;
            }
        }
        const Oval::Sample& s = tr.samples[i];
        if (!s.fix || s.hdop > cfg.maxHdop) {
            continue;
        }
        dx += tr.offsetE[i];
        dy += tr.offsetN[i];
        ++dn;
        const Oval::Vec2 drift{static_cast<float>(dx / dn), static_cast<float>(dy / dn)};
        const Calibrator::Change c = cal.add(f.toLocal(s.lat, s.lon), lane, s.altM, s.hdop);
        if (c != Calibrator::Change::None) {
            out.fits.push_back(cal.lastFit());
        }
        if (c == Calibrator::Change::Provisional) {
            out.provisional   = true;
            out.atProvisional = errorOf(cal.model(), truth, f, drift);
        } else if (c == Calibrator::Change::Locked) {
            out.locked   = true;
            out.lockLaps = cal.lapsCovered();
            out.lockModel = cal.model();
            out.atLock   = errorOf(cal.model(), truth, f, drift);
        } else if (c == Calibrator::Change::Rejected) {
            out.rejected = true;
            out.reason   = cal.rejectReason();
        }
    }
    out.lastFit = cal.lastFit();
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// Calibration accuracy
// ---------------------------------------------------------------------------

TEST(Calibrator, CleanTrackLocksOnTheTruth)
{
    TraceGen::Track track;
    track.headingDeg  = 33.0f;
    track.kerbRadiusM = 37.1f;
    Generator g(track, clean(), 3);
    g.begin(0.3f, 1);
    g.runLaps(5.2, steady(5.0f));
    const Trace tr = g.finish();
    const CalRun r = calibrate(tr, track);
    ASSERT_TRUE(r.provisional);
    ASSERT_TRUE(r.locked);
    EXPECT_EQ(r.lockLaps, 4);
    EXPECT_LT(r.atLock.centreM, 0.1f);
    EXPECT_LT(r.atLock.headingDeg, 0.1f);
    EXPECT_LT(r.atLock.radiusM, 0.1f);
    EXPECT_LT(r.lastFit.shapeM, 0.1f);
}

// 200 random runs: noise 2 to 5 m, a random heading, location, start point,
// lane, speed and bend radius 35 to 38 m. Prints the distributions after one
// lap and at lock. The centre is asserted against where the GPS put the
// track (the true centre moved by the mean GPS drift over the points used):
// no fit can recover a position the GPS has drifted away from. The heading
// at lock is asserted as a rate: its 95th percentile under 1.5
// degrees, and runs at 1.5 or more no more than a Poisson count at 0.5% of
// the runs allows for 99% of seed sets. Check::Radius and Check::Heading
// assert only the targets not yet reached (the "targets" run): R within 0.5 m,
// and the heading within a degree (95th percentile).
enum class Check { Default, Radius, Heading };

static void accuracySweep(float driftLo, float driftHi, uint32_t seed, Check check)
{
    std::mt19937 rng(PortableRandom::seeded(seed));
    auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
    std::vector<double> c1, h1, r1, c4, h4, r4, g1, g4, shape, scale, rs, rsLo, rsMid, rsHi;
    int runs = 0, locked = 0, overC = 0, overCg = 0, overH = 0, overR = 0, lockLap[8] = {};
    std::vector<double> moveC, moveH;
    for (int run = 0; run < 200; ++run) {
        TraceGen::Track track;
        track.headingDeg  = static_cast<float>(uni(0.0, 360.0));
        track.kerbRadiusM = static_cast<float>(uni(35.0, 38.0));
        track.centreLat   = uni(-60.0, 65.0);
        track.centreLon   = uni(-180.0, 180.0);
        const float   sigma = static_cast<float>(uni(2.0, 5.0));
        const uint8_t lane  = static_cast<uint8_t>(1 + rng() % 8);
        const float   speed = static_cast<float>(uni(3.0, 7.0));
        // One draw a statement: the order of a call's arguments is the compiler's.
        const float    drift   = static_cast<float>(uni(driftLo, driftHi));
        const uint32_t genSeed = rng();
        Generator g(track, metresHdop(noisy(sigma, drift)), genSeed);
        const float    startAt = static_cast<float>(uni(0.0, 1.0));
        const uint32_t t0      = rng();
        g.begin(startAt, lane, t0);
        g.runLaps(7.0, runner(speed));
        const Trace  tr = g.finish();
        const CalRun r  = calibrate(tr, track);
        ++runs;
        EXPECT_TRUE(r.provisional) << "run " << run;
        EXPECT_TRUE(r.locked) << "run " << run << " sigma " << sigma << " reason " << int(r.reason)
                              << " shape " << r.lastFit.shapeM << " scale " << r.lastFit.scaleM;
        if (r.provisional) {
            c1.push_back(r.atProvisional.centreM);
            g1.push_back(r.atProvisional.centreGpsM);
            h1.push_back(r.atProvisional.headingDeg);
            r1.push_back(r.atProvisional.radiusM);
        }
        if (r.locked) {
            ++locked;
            c4.push_back(r.atLock.centreM);
            g4.push_back(r.atLock.centreGpsM);
            h4.push_back(r.atLock.headingDeg);
            r4.push_back(r.atLock.radiusM);
            rs.push_back(r.atLock.radiusSignedM);
            (sigma < 3.0f ? rsLo : (sigma < 4.0f ? rsMid : rsHi)).push_back(r.atLock.radiusM);
            overC += r.atLock.centreM >= 1.5f ? 1 : 0;
            overCg += r.atLock.centreGpsM >= 1.5f ? 1 : 0;
            overH += r.atLock.headingDeg >= 1.5f ? 1 : 0;
            overR += r.atLock.radiusM >= 0.5f ? 1 : 0;
            ++lockLap[std::min<int>(r.lockLaps, 7)];
            if (check == Check::Radius) {
                EXPECT_LT(r.atLock.radiusM, 0.5f) << "run " << run << " sigma " << sigma;
            } else if (check == Check::Default) {
                EXPECT_LT(r.atLock.centreGpsM, 1.5f) << "run " << run << " sigma " << sigma;
            }
        }
        for (const auto& f : r.fits) {
            shape.push_back(f.shapeM);
            scale.push_back(f.scaleM);
            if (f.laps >= 4) {
                moveC.push_back(f.centreMoveM);
                moveH.push_back(f.headingMoveDeg);
            }
        }
    }
    std::printf("[ calib    ] drift %.2f to %.2f m a reading: %d runs, %d locked\n", driftLo, driftHi, runs, locked);
    printStats("centre after 1 lap (m)", c1);
    printStats("centre after 1 lap, vs GPS (m)", g1);
    printStats("heading after 1 lap (deg)", h1);
    printStats("R after 1 lap (m)", r1);
    printStats("centre at lock (m)", c4);
    printStats("centre at lock, vs GPS (m)", g4);
    printStats("heading at lock (deg)", h4);
    printStats("R at lock (m)", r4);
    printStats("R at lock, signed (m)", rs);
    printStats("R at lock, noise 2 to 3 m", rsLo);
    printStats("R at lock, noise 3 to 4 m", rsMid);
    printStats("R at lock, noise 4 to 5 m", rsHi);
    printStats("shape, every fit (m)", shape);
    printStats("scale, every fit (m)", scale);
    printStats("centre move between fits, lap 4+ (m)", moveC);
    printStats("heading move between fits, lap 4+", moveH);
    std::printf("[ calib    ] at lock, 1.5 m or more from the true centre: %d runs; from where the GPS put it: %d\n",
                overC, overCg);
    std::printf("[ calib    ] at lock, heading 1.5 degrees or more: %d runs; R 0.5 m or more: %d; "
                "locked at lap 4: %d, 5: %d, 6: %d\n",
                overH, overR, lockLap[4], lockLap[5], lockLap[6]);
    if (check == Check::Default) {
        const size_t allowed = poissonQuantile(0.005 * locked, 0.99);
        EXPECT_LT(percentile(h4, 0.95), 1.5) << "heading at lock, 95th percentile";
        EXPECT_LE(static_cast<size_t>(overH), allowed) << "runs with the heading 1.5 degrees or more at lock";
    } else if (check == Check::Heading) {
        EXPECT_LE(percentile(h4, 0.95), 1.0) << "heading at lock, 95th percentile (target: within a degree)";
    }
}

// With drift: a random walk of 0.05 to 0.15 m per axis a reading.
TEST(Calibrator, AccuracyAcrossNoiseHeadingsAndRadii)
{
    accuracySweep(0.05f, 0.15f, 20261009u, Check::Default);
}

// The same without drift, to show what the fit itself achieves.
TEST(Calibrator, AccuracyWithoutDrift)
{
    accuracySweep(0.0f, 0.0f, 20261010u, Check::Default);
}

// ---------------------------------------------------------------------------
// Targets not yet reached (ctest label "targets", not in the default run;
// docs/OVAL.md): R within 0.5 m at lock, on the same runs, and the
// heading within a degree at the 95th
// percentile.
// ---------------------------------------------------------------------------

TEST(Targets, CalibrationRadiusWithin0_5mAtLock)
{
    accuracySweep(0.05f, 0.15f, 20261009u, Check::Radius);
}

TEST(Targets, CalibrationRadiusWithin0_5mAtLockWithoutDrift)
{
    accuracySweep(0.0f, 0.0f, 20261010u, Check::Radius);
}

TEST(Targets, CalibrationHeadingWithin1DegreeAtLock)
{
    accuracySweep(0.05f, 0.15f, 20261009u, Check::Heading);
}

TEST(Targets, CalibrationHeadingWithin1DegreeAtLockWithoutDrift)
{
    accuracySweep(0.0f, 0.0f, 20261010u, Check::Heading);
}

// ---------------------------------------------------------------------------
// non-standard shapes are rejected
// ---------------------------------------------------------------------------

namespace {

struct Shape {
    const char*     name;
    TraceGen::Track track;
};

std::vector<Shape> badShapes()
{
    return {{"300 m track", TraceGen::shortTrack(300.0)},
            {"park loop", TraceGen::parkLoopTrack()},
            {"400 m circle", TraceGen::circleTrack()},
            {"double bend", TraceGen::doubleBendTrack()}};
}

const char* reasonName(Calibrator::Reject r)
{
    switch (r) {
    case Calibrator::Reject::None:       return "none";
    case Calibrator::Reject::Shape:      return "shape";
    case Calibrator::Reject::Residual:   return "residual";
    case Calibrator::Reject::RadiusEdge: return "radius edge";
    case Calibrator::Reject::Coverage:   return "coverage";
    case Calibrator::Reject::Disagree:   return "disagree";
    }
    return "?";
}

} // namespace

TEST(Calibrator, NonStandardShapesAreRejected)
{
    const float sigmas[] = {0.0f, 2.0f, 3.0f, 5.0f};
    for (const Shape& sh : badShapes()) {
        double minShape = 1e9, minScale = 1e9, maxShape = 0.0;
        int    rejected = 0, runs = 0, atLap[8] = {};
        for (float sigma : sigmas) {
            for (uint32_t seed : {1u, 2u, 3u, 4u, 5u}) {
                TraceGen::Track track = sh.track;
                track.headingDeg = 17.0f * seed;
                Generator g(track, sigma > 0.0f ? noisy(sigma) : clean(), seed);
                g.begin(0.1f * seed, 1);
                g.runLaps(8.0, runner(5.0f));
                const Trace  tr = g.finish();
                const CalRun r  = calibrate(tr, track);
                ++runs;
                EXPECT_TRUE(r.rejected) << sh.name << " sigma " << sigma << " seed " << seed;
                EXPECT_FALSE(r.locked) << sh.name << " sigma " << sigma << " seed " << seed;
                if (r.rejected) {
                    ++rejected;
                    ++atLap[std::min<int>(r.lastFit.laps, 7)];
                }
                for (const auto& f : r.fits) {
                    minShape = std::min<double>(minShape, f.shapeM);
                    maxShape = std::max<double>(maxShape, f.shapeM);
                    minScale = std::min<double>(minScale, f.scaleM);
                }
                std::printf("[ reject   ] %-12s sigma %.0f seed %u: %s at lap %d, shape %.2f m, scale %.2f m, R %.2f m\n",
                            sh.name, sigma, seed, r.rejected ? reasonName(r.reason) : "NOT REJECTED",
                            r.lastFit.laps, r.lastFit.shapeM, r.lastFit.scaleM, r.lastFit.kerbRadiusM);
            }
        }
        std::printf("[ reject   ] %-12s %d of %d rejected; shape %.2f to %.2f m; lowest scale %.2f m\n", sh.name,
                    rejected, runs, minShape, maxShape, minScale);
    }
}

// ---------------------------------------------------------------------------
// A lane change during the calibration laps
// ---------------------------------------------------------------------------

// The same runs with and without a declared change from lane 1 to lane 4
// half way through lap 2 (the noise is the same reading for reading). The
// points are tagged with the lane the runner had set, so the change should
// move the fit by far less than the accuracy targets: on average under 0.3 m
// for the centre and under 0.1 m for R (a fifth of the targets).
TEST(Calibrator, LaneChangeDuringCalibrationDoesNotBiasTheFit)
{
    std::vector<double> dCentre, dR, dHeading, silentCentre, silentR;
    double sumDR = 0.0, sumDx = 0.0, sumDy = 0.0;
    int    pairs = 0;
    for (uint32_t seed = 1; seed <= 100; ++seed) {
        TraceGen::Track track;
        track.headingDeg  = 3.7f * seed;
        track.kerbRadiusM = 35.0f + 0.03f * seed;
        const float sigma = 2.0f + 0.03f * seed;
        CalRun runs[3];
        for (int variant = 0; variant < 3; ++variant) {
            Generator g(track, noisy(sigma, 0.0f), seed);
            g.begin(0.0f, 1);
            g.runLaps(1.5, runner());
            if (variant > 0) {
                g.changeLane(4, 1.5f, variant == 1); // 1: declared, 2: silent
            }
            g.runLaps(5.0, runner());
            runs[variant] = calibrate(g.finish(), track);
        }
        ASSERT_TRUE(runs[0].locked) << "seed " << seed;
        EXPECT_TRUE(runs[1].locked) << "declared change, seed " << seed;
        if (!runs[1].locked) {
            continue;
        }
        // The same start point, so the same local frame: centres compare directly.
        ++pairs;
        const Oval::Vec2 shift = runs[1].lockModel.centre() - runs[0].lockModel.centre();
        const float      rDiff = runs[1].lockModel.kerbRadiusM() - runs[0].lockModel.kerbRadiusM();
        sumDx += shift.x;
        sumDy += shift.y;
        sumDR += rDiff;
        dCentre.push_back(Oval::length(shift));
        dR.push_back(std::fabs(rDiff));
        float dh = std::fabs(runs[1].lockModel.headingDeg() - runs[0].lockModel.headingDeg());
        dHeading.push_back(dh > 90.0f ? 180.0f - dh : dh);
        if (runs[2].locked) {
            silentCentre.push_back(Oval::length(runs[2].lockModel.centre() - runs[0].lockModel.centre()));
            silentR.push_back(std::fabs(runs[2].lockModel.kerbRadiusM() - runs[0].lockModel.kerbRadiusM()));
        }
    }
    ASSERT_GT(pairs, 90);
    const double meanShift = std::hypot(sumDx / pairs, sumDy / pairs);
    std::printf("[ calib    ] lane change 1 to 4 in lap 2: %d pairs; mean centre shift %.3f m, mean R shift %+.3f m\n",
                pairs, meanShift, sumDR / pairs);
    printStats("declared: centre moved by (m)", dCentre);
    printStats("declared: R moved by (m)", dR);
    printStats("declared: heading moved by (deg)", dHeading);
    printStats("silent: centre moved by (m)", silentCentre);
    printStats("silent: R moved by (m)", silentR);
    EXPECT_LT(meanShift, 0.3) << "mean shift of the centre";
    EXPECT_LT(std::fabs(sumDR / pairs), 0.1) << "mean shift of R";
}

// ---------------------------------------------------------------------------
// The rejection thresholds, and the margin both ways
// ---------------------------------------------------------------------------

// The quality checks that can reject from calRejectLaps on, against standard
// tracks (100 runs at 2 to 5 m noise, R 35 to 38 m, lanes 1 to 8) and the four
// shapes (5 runs each at 2 to 5 m noise). R is fitted here with the search
// range opened up, so the figures show where each would land.
TEST(Calibrator, RejectionMargins)
{
    const Oval::Config def;
    Oval::Config open = def;
    open.fitRadiusMinM    = 20.0f;
    open.fitRadiusMaxM    = 55.0f;
    open.maxShapeM        = 100.0f;
    open.maxScaleM        = 100.0f;
    open.calMaxLaps       = 8;
    open.lockCentreM      = 0.0f; // never lock: fit every lap
    const float rHigh = def.fitRadiusMaxM - 0.05f; // the edge test
    const float rLow  = def.fitRadiusMinM + 0.05f;

    // Standard tracks: how close each check comes to rejecting.
    float stdMaxShape = 0.0f, stdMaxScale = 0.0f, stdMaxR = 0.0f, stdMinR = 99.0f;
    for (uint32_t seed = 1; seed <= 100; ++seed) {
        TraceGen::Track track;
        track.kerbRadiusM = (seed % 2) ? 35.0f : 38.0f; // the edges of the allowed range
        track.headingDeg  = 7.0f * seed;
        Generator g(track, noisy(2.0f + 0.03f * seed), seed);
        g.begin(0.01f * seed, static_cast<uint8_t>(1 + seed % 8));
        g.runLaps(6.5, runner(3.0f + 0.04f * seed));
        const CalRun r = calibrate(g.finish(), track, open);
        for (const auto& f : r.fits) {
            if (f.laps < def.calRejectLaps) {
                continue;
            }
            stdMaxShape = std::max(stdMaxShape, f.shapeM);
            stdMaxScale = std::max(stdMaxScale, f.scaleM);
            stdMaxR     = std::max(stdMaxR, f.kerbRadiusM);
            stdMinR     = std::min(stdMinR, f.kerbRadiusM);
        }
    }
    std::printf("[ margins  ] standard tracks from lap %d: R %.2f to %.2f m (edges %.2f, %.2f), shape up to %.2f m "
                "(limit %.2f), scale up to %.2f m (limit %.2f)\n",
                def.calRejectLaps, stdMinR, stdMaxR, rLow, rHigh, stdMaxShape, def.maxShapeM, stdMaxScale, def.maxScaleM);
    std::printf("[ margins  ] standard margins: R %.2f m below the top edge, %.2f m above the bottom; shape %.2f m; "
                "scale %.2f m\n",
                rHigh - stdMaxR, stdMinR - rLow, def.maxShapeM - stdMaxShape, def.maxScaleM - stdMaxScale);
    EXPECT_LT(stdMaxR, rHigh);
    EXPECT_GT(stdMinR, rLow);
    EXPECT_LT(stdMaxShape, def.maxShapeM);
    EXPECT_LT(stdMaxScale, def.maxScaleM);

    // The shapes: the strongest reason each fit fails, as a margin beyond
    // its threshold (in metres of R, shape or scale).
    for (const Shape& sh : badShapes()) {
        float worstR = 99.0f, worstShape = 99.0f;
        for (uint32_t seed = 1; seed <= 5; ++seed) {
            TraceGen::Track track = sh.track;
            track.headingDeg = 17.0f * seed;
            Generator g(track, noisy(1.25f + 0.75f * seed), seed);
            g.begin(0.1f * seed, 1);
            g.runLaps(6.5, runner(5.0f));
            const CalRun r = calibrate(g.finish(), track, open);
            for (const auto& f : r.fits) {
                if (f.laps < def.calRejectLaps) {
                    continue;
                }
                worstR     = std::min(worstR, f.kerbRadiusM - rHigh);
                worstShape = std::min(worstShape, f.shapeM - def.maxShapeM);
            }
        }
        std::printf("[ margins  ] %-12s from lap %d: R at least %.2f m beyond the edge; shape %+.2f m against its "
                    "limit\n",
                    sh.name, def.calRejectLaps, worstR, worstShape);
        EXPECT_TRUE(worstR > 0.0f || worstShape > 0.0f) << sh.name;
    }
}

// What an error in R costs in part-lap distance: the true track, supplied
// with R off by up to 1 m (beyond the 95th percentile at lock), no noise.
TEST(Calibrator, PartLapCostOfARadiusError)
{
    for (float dR : {0.0f, 0.5f, 0.75f, 1.0f}) {
        std::vector<double> err;
        for (uint32_t seed = 1; seed <= 10; ++seed) {
            TraceGen::Track track;
            track.headingDeg  = 31.0f * seed;
            track.kerbRadiusM = 36.5f;
            TraceGen::Track wrong = track;
            wrong.kerbRadiusM = track.kerbRadiusM + ((seed % 2) ? dR : -dR);
            const TrackModel stored = Replay::truthModel(wrong);
            Generator g(track, clean(), seed);
            g.begin(0.07f * seed, static_cast<uint8_t>(1 + seed % 8));
            g.runLaps(3, steady(5.0f));
            g.run(40.0, steady(5.0f));
            const Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, LaneTable::outdoor(), Oval::Config(), &stored);
            EXPECT_EQ(r.laps.size(), tr.crossMs.size());
            for (size_t i = tr.startIndex + 20; i < tr.samples.size(); ++i) {
                err.push_back(std::fabs(r.total[i] - tr.truePhysM[i]));
            }
        }
        char label[64];
        std::snprintf(label, sizeof label, "R off by %.2f m: part-lap |error| (m)", dR);
        printStats(label, err);
    }
}
