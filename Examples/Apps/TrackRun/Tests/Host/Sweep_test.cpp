// Robustness sweep: 200 seeded randomised runs through Oval::LapCounter.
//
// Each run picks a lane, speed (3 to 7 m/s), position noise (2 to 5 m white
// per axis, with drift), GPS distance over-read, track orientation, location,
// start point, clock start and lap count (8 to 12). Hard assertions: no false
// laps, no missed laps left uncredited, the total never decreases, and the
// total at the final crossing is exact to 0.01 m. Lap time errors are only
// reported: synthetic white noise is harsher than real GPS, whose errors
// drift slowly, so these figures are a ceiling.
//
// For tuning, TRACKRUN_SWEEP_SEED and TRACKRUN_SWEEP_RUNS change the master
// seed and the number of runs.

#include "OvalTestUtil.hpp"

#include "Oval/LaneTable.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

namespace {

struct Stats {
    std::vector<double> absErrS;
    void add(double errMs) { absErrS.push_back(std::fabs(errMs) / 1000.0); }
    void print(const char* label) const
    {
        if (absErrS.empty()) {
            std::printf("[ sweep    ] %-22s no laps\n", label);
            return;
        }
        std::vector<double> v = absErrS;
        std::sort(v.begin(), v.end());
        double sum = 0.0;
        for (double x : v) {
            sum += x;
        }
        const size_t p95 = static_cast<size_t>(std::ceil(0.95 * v.size())) - 1;
        std::printf("[ sweep    ] %-22s %5zu laps: mean %.2f s, 95th percentile %.2f s, worst %.2f s\n",
                    label, v.size(), sum / v.size(), v[p95], v.back());
    }
};

} // namespace

TEST(Sweep, TwoHundredRandomisedRuns)
{
    const Oval::LaneTable table   = Oval::LaneTable::outdoor();
    const char*           seedEnv = std::getenv("TRACKRUN_SWEEP_SEED");
    const char*           runsEnv = std::getenv("TRACKRUN_SWEEP_RUNS");
    const int             total   = runsEnv ? std::atoi(runsEnv) : 200;
    std::mt19937 rng(PortableRandom::seeded(seedEnv ? static_cast<uint32_t>(std::strtoul(seedEnv, nullptr, 10)) : 20261006u));
    auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
    auto pick = [&](int a, int b) { return PortableRandom::UniformInt(a, b)(rng); };

    Stats all, byNoise[3];
    size_t runs = 0, laps = 0, missedCredited = 0, failedRuns = 0;
    double minOver = 9.0, maxOver = 0.0, worstEnd = 0.0;

    for (int run = 0; run < total; ++run) {
        TraceGen::Track track;
        track.headingDeg = static_cast<float>(uni(0.0, 360.0));
        track.centreLat  = uni(-60.0, 65.0);
        track.centreLon  = uni(-180.0, 180.0);

        const uint8_t lane     = static_cast<uint8_t>(pick(1, 8));
        const float   speed    = static_cast<float>(uni(3.0, 7.0));
        const float   sigma    = static_cast<float>(uni(2.0, 5.0));
        const int     lapCount = pick(8, 12);

        TraceGen::Noise n = OvalTest::noisy(sigma, static_cast<float>(uni(0.05, 0.15)));
        // GPS distance: summed white jitter sized for an over-read of 0 to
        // 15% at this pace, times a systematic 0.95 to 1.10. Overall about
        // -5% to +27%, around the simulator's 12%.
        n.distJitterM = static_cast<float>(speed * std::sqrt(uni(0.0, 0.15)));
        n.distScale   = static_cast<float>(uni(0.95, 1.10));

        const uint32_t seed = rng();
        TraceGen::Generator g(track, n, seed);
        const uint32_t t0 = rng();
        g.begin(static_cast<float>(uni(0.0, 1.0)), lane, t0);
        g.runLaps(lapCount, OvalTest::runner(speed));
        g.run(40.0, OvalTest::runner(speed));
        const TraceGen::Trace tr = g.finish();
        ASSERT_EQ(tr.crossMs.size(), static_cast<size_t>(lapCount));

        const double over = tr.gpsDistM / tr.movedM;
        minOver = std::min(minOver, over);
        maxOver = std::max(maxOver, over);

        const Replay::Result r = Replay::run(tr, table);
        const ::testing::AssertionResult ok = OvalTest::wholeLapsRight(tr, r, 15000);
        EXPECT_TRUE(ok) << "run " << run << " lane " << int(lane) << " speed " << speed << " sigma " << sigma;
        if (!ok) {
            ++failedRuns;
            continue;
        }
        ASSERT_FALSE(r.events.empty());
        EXPECT_NEAR(r.events.back().totalM, lapCount * table.lapLengthM(lane), 0.01f) << "run " << run;

        const Replay::Match m = Replay::match(tr, r, 15000);
        ++runs;
        laps += m.countedLaps;
        missedCredited += m.missedLaps;
        worstEnd = std::max(worstEnd, m.worstEndErrMs);
        const int band = sigma < 3.0f ? 0 : (sigma < 4.0f ? 1 : 2);
        for (double e : m.lapTimeErrMs) {
            all.add(e);
            byNoise[band].add(e);
        }
    }

    std::printf("[ sweep    ] %zu runs passed of %d, %zu laps, %zu missed laps credited, %zu runs failed\n",
                runs, total, laps, missedCredited, failedRuns);
    std::printf("[ sweep    ] GPS distance over-read across runs: %.1f%% to %.1f%%\n",
                100.0 * (minOver - 1.0), 100.0 * (maxOver - 1.0));
    std::printf("[ sweep    ] worst lap end against the true pass of the start point: %.2f s\n", worstEnd / 1000.0);
    std::printf("[ sweep    ] lap time error (|counted lap duration - true duration|):\n");
    all.print("all runs");
    byNoise[0].print("noise 2 to 3 m");
    byNoise[1].print("noise 3 to 4 m");
    byNoise[2].print("noise 4 to 5 m");
    EXPECT_EQ(failedRuns, 0u);
}
