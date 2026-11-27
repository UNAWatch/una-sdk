// Robustness sweep for Oval::Tracker: 200 seeded randomised runs.
//
// Half calibrate from scratch; half start with the true track supplied as a
// stored model, from a random point on the lap. Each run picks a lane, speed
// (3 to 7 m/s), noise (2 to 5 m white per axis, with drift), GPS distance
// over-read, heading, bend radius (35 to 38 m), location, clock start and lap
// count (8 to 12). Hard assertions: no false laps, no missed laps, the total
// never decreases, every lap credited its exact lane length. Part-lap error
// and lap time error (provisional and refined) are reported by noise band.
//
// TRACKRUN_SWEEP_SEED and TRACKRUN_SWEEP_RUNS change the master seed and the
// number of runs, as for the LapCounter sweep (Sweep_test.cpp).

#include "OvalTestUtil.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/Tracker.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace OvalTest;

TEST(TrackerSweep, TwoHundredRandomisedRuns)
{
    const Oval::LaneTable table   = Oval::LaneTable::outdoor();
    const int             total   = static_cast<int>(envNumber("TRACKRUN_SWEEP_RUNS", 200));
    std::mt19937 rng(PortableRandom::seeded(envSeed("TRACKRUN_SWEEP_SEED", 20261009u)));
    auto uni  = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
    auto pick = [&](int a, int b) { return PortableRandom::UniformInt(a, b)(rng); };

    std::vector<double> part[3], prov[3], ref[3], partAll, provAll, refAll;
    size_t laps = 0, missedCredited = 0, failedRuns = 0, locked = 0, rejected = 0, neverCalibrated = 0, droppedOut = 0;
    float  worstHold = 0.0f;

    for (int run = 0; run < total; ++run) {
        TraceGen::Track track;
        track.headingDeg  = static_cast<float>(uni(0.0, 360.0));
        track.kerbRadiusM = static_cast<float>(uni(35.0, 38.0));
        track.centreLat   = uni(-60.0, 65.0);
        track.centreLon   = uni(-180.0, 180.0);

        const bool    supplied = (run % 2) == 1;
        const uint8_t lane     = static_cast<uint8_t>(pick(1, 8));
        const float   speed    = static_cast<float>(uni(3.0, 7.0));
        const float   sigma    = static_cast<float>(uni(2.0, 5.0));
        const int     lapCount = pick(8, 12);

        TraceGen::Noise n = noisy(sigma, static_cast<float>(uni(0.05, 0.15)));
        n.distJitterM = static_cast<float>(speed * std::sqrt(uni(0.0, 0.15)));
        n.distScale   = static_cast<float>(uni(0.95, 1.10));

        const uint32_t seed = rng();
        TraceGen::Generator g(track, n, seed);
        const uint32_t t0 = rng();
        g.begin(static_cast<float>(uni(0.0, 1.0)), lane, t0);
        g.runLaps(lapCount, runner(speed));
        g.run(40.0, runner(speed));
        const TraceGen::Trace tr = g.finish();

        const Oval::TrackModel        stored = Replay::truthModel(track);
        const Replay::TrackerResult   r = Replay::runTracker(tr, table, Oval::Config(), supplied ? &stored : nullptr);

        bool ok = r.decreases == 0 && r.laps.size() == tr.crossMs.size();
        size_t badLap = SIZE_MAX;
        for (size_t k = 0; ok && k < r.laps.size(); ++k) {
            ok = std::fabs(r.laps[k].lapLengthM - table.lapLengthM(lane)) <= 0.01f
                && std::abs(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])) <= 15000;
            if (!ok) {
                badLap = k;
            }
        }
        if (badLap != SIZE_MAX) {
            std::printf("[ tsweep   ] run %d lap %zu: %s, credited %.2f m, ends %.1f s from the truth, switch at reading %zu, lap at reading %zu\n",
                        run, badLap + 1, r.laps[badLap].type == Oval::Tracker::EventType::Lap ? "lap" : "missed lap",
                        r.laps[badLap].lapLengthM, Oval::msDiff(r.laps[badLap].tMs, tr.crossMs[badLap]) / 1000.0,
                        r.calibratedAt, r.lapSample[badLap]);
        }
        EXPECT_TRUE(ok) << "run " << run << (supplied ? " stored" : " scratch") << " lane " << int(lane) << " speed "
                        << speed << " sigma " << sigma << ": " << r.laps.size() << " laps, truth " << tr.crossMs.size()
                        << ", decreases " << r.decreases;
        if (!ok) {
            ++failedRuns;
            continue;
        }
        laps += r.laps.size();
        missedCredited += r.missed;
        locked += r.lockedAt != SIZE_MAX ? 1 : 0;
        rejected += r.rejectedAt != SIZE_MAX ? 1 : 0;
        neverCalibrated += r.calibratedAt == SIZE_MAX ? 1 : 0;
        worstHold = std::max(worstHold, r.largestHold);
        for (const auto& e : r.events) {
            if (e.type == Oval::Tracker::EventType::ModeChanged && e.mode == Oval::Tracker::Mode::Counting) {
                ++droppedOut; // the runner never leaves the track in these runs
                break;
            }
        }

        const int band = sigma < 3.0f ? 0 : (sigma < 4.0f ? 1 : 2);
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            if (r.mode[i] == static_cast<uint8_t>(Oval::Tracker::Mode::Calibrated)) {
                const double e = std::fabs(r.total[i] - tr.truePhysM[i]);
                part[band].push_back(e);
                partAll.push_back(e);
            }
        }
        const Replay::TimeErrors te = Replay::lapTimeErrors(tr, r);
        for (double e : te.provisional) {
            prov[band].push_back(std::fabs(e) / 1000.0);
            provAll.push_back(std::fabs(e) / 1000.0);
        }
        for (double e : te.refined) {
            ref[band].push_back(std::fabs(e) / 1000.0);
            refAll.push_back(std::fabs(e) / 1000.0);
        }
    }

    std::printf("[ tsweep   ] %d runs, %zu failed, %zu laps, %zu missed laps credited\n", total, failedRuns, laps,
                missedCredited);
    std::printf("[ tsweep   ] from scratch: %zu locked, %zu rejected, %zu never calibrated; largest hold %.1f m\n",
                locked, rejected, neverCalibrated, worstHold);
    const char* bands[3] = {"2 to 3 m", "3 to 4 m", "4 to 5 m"};
    char label[64];
    printStats("part-lap |error| (m), all", partAll);
    for (int b = 0; b < 3; ++b) {
        std::snprintf(label, sizeof label, "part-lap |error| (m), %s", bands[b]);
        printStats(label, part[b]);
    }
    printStats("lap time provisional (s), all", provAll);
    printStats("lap time refined (s), all", refAll);
    for (int b = 0; b < 3; ++b) {
        std::snprintf(label, sizeof label, "lap time provisional (s), %s", bands[b]);
        printStats(label, prov[b]);
        std::snprintf(label, sizeof label, "lap time refined (s), %s", bands[b]);
        printStats(label, ref[b]);
    }
    std::printf("[ tsweep   ] runs wrongly dropped to counting mode as off the track: %zu\n", droppedOut);
    EXPECT_EQ(failedRuns, 0u);
    EXPECT_EQ(droppedOut, 0u);
    EXPECT_EQ(rejected, 0u) << "standard tracks are never rejected";
}

// Lap time error before and after refinement, in both modes, on exactly the
// 200 traces of the LapCounter sweep (Sweep_test.cpp, same seed and draws). Counting
// mode is the Tracker with calibration switched off (no provisional fit can
// happen); calibrated is the Tracker as it is, so laps from about the second
// on are timed on the lane line. The LapCounter's figures on these traces
// are 0.72 s mean, 2.04 s 95th percentile and 5.85 s worst.
TEST(TrackerSweep, LapTimesOnTheLapCounterSweepTraces)
{
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    std::mt19937 rng(PortableRandom::seeded(20261006u));
    auto uni  = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
    auto pick = [&](int a, int b) { return PortableRandom::UniformInt(a, b)(rng); };

    Oval::Config countingOnly;
    countingOnly.calProvisionalLaps = 255;

    // [mode: 0 counting, 1 calibrated][noise band]
    std::vector<double> prov[2][4], ref[2][4], twoReading[4];
    for (int run = 0; run < 200; ++run) {
        TraceGen::Track track;
        track.headingDeg = static_cast<float>(uni(0.0, 360.0));
        track.centreLat  = uni(-60.0, 65.0);
        track.centreLon  = uni(-180.0, 180.0);
        const uint8_t lane     = static_cast<uint8_t>(pick(1, 8));
        const float   speed    = static_cast<float>(uni(3.0, 7.0));
        const float   sigma    = static_cast<float>(uni(2.0, 5.0));
        const int     lapCount = pick(8, 12);
        TraceGen::Noise n = noisy(sigma, static_cast<float>(uni(0.05, 0.15)));
        n.distJitterM = static_cast<float>(speed * std::sqrt(uni(0.0, 0.15)));
        n.distScale   = static_cast<float>(uni(0.95, 1.10));
        const uint32_t seed = rng();
        TraceGen::Generator g(track, n, seed);
        const uint32_t t0 = rng();
        g.begin(static_cast<float>(uni(0.0, 1.0)), lane, t0);
        g.runLaps(lapCount, runner(speed));
        g.run(40.0, runner(speed));
        const TraceGen::Trace tr = g.finish();
        const int band = sigma < 3.0f ? 1 : (sigma < 4.0f ? 2 : 3);

        // The LapCounter alone on the same trace, for comparison.
        const Replay::Result lc = Replay::run(tr, table);
        for (double e : Replay::match(tr, lc, 15000).lapTimeErrMs) {
            twoReading[0].push_back(std::fabs(e) / 1000.0);
            twoReading[band].push_back(std::fabs(e) / 1000.0);
        }

        for (int mode = 0; mode < 2; ++mode) {
            const Replay::TrackerResult r = Replay::runTracker(tr, table, mode == 0 ? countingOnly : Oval::Config());
            ASSERT_EQ(r.laps.size(), tr.crossMs.size()) << "run " << run << " mode " << mode;
            if (mode == 0) {
                EXPECT_EQ(r.finalMode, Oval::Tracker::Mode::Calibrating);
            }
            const Replay::TimeErrors te = Replay::lapTimeErrors(tr, r);
            for (size_t k = 0; k < te.provisional.size(); ++k) {
                // In the calibrated pass, only laps that ended in calibrated mode.
                if (mode == 1 && te.mode[k] != Oval::Tracker::Mode::Calibrated) {
                    continue;
                }
                for (int b : {0, band}) {
                    prov[mode][b].push_back(std::fabs(te.provisional[k]) / 1000.0);
                    ref[mode][b].push_back(std::fabs(te.refined[k]) / 1000.0);
                }
            }
        }
    }
    const char* bands[4] = {"all", "2 to 3 m", "3 to 4 m", "4 to 5 m"};
    char label[64];
    for (int b = 0; b < 4; ++b) {
        std::snprintf(label, sizeof label, "LapCounter (s), %s", bands[b]);
        printStats(label, twoReading[b]);
        std::snprintf(label, sizeof label, "counting provisional (s), %s", bands[b]);
        printStats(label, prov[0][b]);
        std::snprintf(label, sizeof label, "counting refined (s), %s", bands[b]);
        printStats(label, ref[0][b]);
        std::snprintf(label, sizeof label, "calibrated provisional (s), %s", bands[b]);
        printStats(label, prov[1][b]);
        std::snprintf(label, sizeof label, "calibrated refined (s), %s", bands[b]);
        printStats(label, ref[1][b]);
    }
    for (int mode = 0; mode < 2; ++mode) {
        EXPECT_LT(percentile(ref[mode][0], 0.5), percentile(prov[mode][0], 0.5)) << "mode " << mode;
        EXPECT_LT(percentile(ref[mode][0], 0.95), percentile(prov[mode][0], 0.95)) << "mode " << mode;
    }
}

// The same randomised runs, all on a stored track, each with a
// day's GPS offset of up to 20 m in a random direction (the stored model is
// the true track moved by minus the offset). Every run must be lined up
// (never mistaken for a wrong pick), with no false or missed laps, every lap
// credited exactly, and no false off-track drop. TRACKRUN_SWEEP_RUNS and
// TRACKRUN_SWEEP_SEED as above.
TEST(TrackerSweep, StoredTracksWithDailyOffsets)
{
    const Oval::LaneTable table   = Oval::LaneTable::outdoor();
    const int             total   = static_cast<int>(envNumber("TRACKRUN_SWEEP_RUNS", 200));
    const char*           seedEnv = std::getenv("TRACKRUN_SWEEP_SEED");
    std::mt19937 rng(PortableRandom::seeded(seedEnv ? envSeed("TRACKRUN_SWEEP_SEED", 0u) + 4u : 20261013u));
    auto uni  = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
    auto pick = [&](int a, int b) { return PortableRandom::UniformInt(a, b)(rng); };

    std::vector<double> part[3], partAll, alignM, offErr;
    size_t laps = 0, failedRuns = 0, droppedOut = 0, abandoned = 0, flips = 0;
    for (int run = 0; run < total; ++run) {
        TraceGen::Track track;
        track.headingDeg  = static_cast<float>(uni(0.0, 360.0));
        track.kerbRadiusM = static_cast<float>(uni(35.0, 38.0));
        track.centreLat   = uni(-60.0, 65.0);
        track.centreLon   = uni(-180.0, 180.0);

        const uint8_t lane     = static_cast<uint8_t>(pick(1, 8));
        const float   speed    = static_cast<float>(uni(3.0, 7.0));
        const float   sigma    = static_cast<float>(uni(2.0, 5.0));
        const int     lapCount = pick(8, 12);
        const double  offset   = uni(0.0, 20.0);
        const double  ang      = uni(0.0, 6.283185307179586);

        TraceGen::Noise n = noisy(sigma, static_cast<float>(uni(0.05, 0.15)));
        n.distJitterM = static_cast<float>(speed * std::sqrt(uni(0.0, 0.15)));
        n.distScale   = static_cast<float>(uni(0.95, 1.10));

        const uint32_t seed = rng();
        TraceGen::Generator g(track, n, seed);
        const uint32_t t0 = rng();
        g.begin(static_cast<float>(uni(0.0, 1.0)), lane, t0);
        g.runLaps(lapCount, runner(speed));
        g.run(40.0, runner(speed));
        const TraceGen::Trace tr = g.finish();

        const double e = offset * PortableMath::cos(ang), nn = offset * PortableMath::sin(ang);
        const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(track), -e, -nn);
        const Replay::TrackerResult r = Replay::runTracker(tr, table, Oval::Config(), &stored);

        bool ok = r.decreases == 0 && r.laps.size() == tr.crossMs.size();
        for (size_t k = 0; ok && k < r.laps.size(); ++k) {
            ok = std::fabs(r.laps[k].lapLengthM - table.lapLengthM(lane)) <= 0.01f
                && std::abs(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])) <= 15000;
        }
        EXPECT_TRUE(ok) << "run " << run << " offset " << offset << " m lane " << int(lane) << " speed " << speed
                        << " sigma " << sigma << ": " << r.laps.size() << " laps, truth " << tr.crossMs.size()
                        << ", decreases " << r.decreases;
        abandoned += r.alignment.abandoned ? 1 : 0;
        flips += r.alignment.flips;
        if (r.alignment.abandoned || r.alignment.flips > 0) {
            std::printf("[ tsweep4  ] run %d: offset %.1f m at %.0f deg, start %.2f of the lap, lane %d, noise %.1f m, speed %.1f: %s, %d flip(s), lined up after %.0f m with offset %.1f, %.1f (scale %.1f)\n",
                        run, offset, ang * 57.29578, 0.0, int(lane), sigma, speed,
                        r.alignment.abandoned ? "abandoned" : "kept", r.alignment.flips, r.alignment.endM,
                        r.alignment.offset.x, r.alignment.offset.y, r.alignment.scaleM);
        }
        for (const auto& ev : r.events) {
            if (ev.type == Oval::Tracker::EventType::ModeChanged) {
                ++droppedOut;
                break;
            }
        }
        if (!ok) {
            ++failedRuns;
            continue;
        }
        laps += r.laps.size();
        alignM.push_back(r.alignment.endM);
        if (r.alignedAt != SIZE_MAX) {
            offErr.push_back(std::hypot(r.alignment.offset.x - (e + tr.offsetE[r.alignedAt]),
                                        r.alignment.offset.y - (nn + tr.offsetN[r.alignedAt])));
        }
        const int band = sigma < 3.0f ? 0 : (sigma < 4.0f ? 1 : 2);
        for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
            if (r.mode[i] == static_cast<uint8_t>(Oval::Tracker::Mode::Calibrated)) {
                const double err = std::fabs(r.total[i] - tr.truePhysM[i]);
                part[band].push_back(err);
                partAll.push_back(err);
            }
        }
    }
    std::printf("[ tsweep4  ] %d stored-track runs with offsets up to 20 m: %zu failed, %zu laps, %zu wrongly abandoned, %zu direction flips, %zu dropped to counting mode\n",
                total, failedRuns, laps, abandoned, flips, droppedOut);
    printStats("lined up after (m)", alignM);
    printStats("offset error when lined up (m)", offErr);
    printStats("part-lap |error| (m), all", partAll);
    const char* bands[3] = {"2 to 3 m", "3 to 4 m", "4 to 5 m"};
    char label[64];
    for (int b = 0; b < 3; ++b) {
        std::snprintf(label, sizeof label, "part-lap |error| (m), %s", bands[b]);
        printStats(label, part[b]);
    }
    EXPECT_EQ(failedRuns, 0u);
    EXPECT_EQ(abandoned, 0u);
    EXPECT_EQ(flips, 0u);
    EXPECT_EQ(droppedOut, 0u);
}
