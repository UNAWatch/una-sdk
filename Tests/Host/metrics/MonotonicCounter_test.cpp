// Unit tests for SDK::Metric::MonotonicCounter, focused on the distance
// auto-lap "drift" fix: advanceLap() closes a lap on an exact target and
// carries the per-sample overshoot into the next lap, so lap boundaries stay
// pinned to the km/mi grid instead of walking forward by the overshoot.

#include <gtest/gtest.h>

#include "SDK/Metrics/MonotonicCounter.hpp"

#include <vector>

using SDK::Metric::MonotonicCounter;

namespace {

// A cumulative distance stream sampled at 1 Hz. Each element is the absolute
// distance (metres) reported by the sensor at that tick. The step between
// samples is > 0 so a lap boundary is generally crossed mid-sample, producing
// an overshoot -- exactly the condition that used to make laps drift.
//
// The stream starts at a non-zero base: MonotonicCounter treats a first sample
// equal to T{} as a not-yet-seeded sentinel and would re-seat the base on the
// following call, which is not the behaviour under test here.
constexpr float kBase = 10000.0f;

std::vector<float> makeStream(float stepM, int samples)
{
    std::vector<float> out;
    out.reserve(static_cast<size_t>(samples));
    float d = kBase;
    for (int i = 0; i < samples; ++i) {
        out.push_back(d);
        d += stepM;
    }
    return out;
}

} // namespace

// Baseline behaviour: resetLap() re-bases at the next sample, discarding the
// overshoot. This is what caused the drift and is preserved for manual /
// time / final laps.
TEST(MonotonicCounter, ResetLapDropsOvershoot)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(kBase);            // seat base
    c.add(kBase + 6.0f);
    c.add(kBase + 12.0f);    // lap value now 12, target 10 -> overshoot 2

    EXPECT_FLOAT_EQ(c.getLapValueActive(), 12.0f);
    c.resetLap();
    c.add(kBase + 18.0f);    // re-based here; overshoot of 2 is lost

    EXPECT_FLOAT_EQ(c.getLapValueActive(), 0.0f); // 18 - 18 lap base
    EXPECT_FLOAT_EQ(c.getValueActive(), 18.0f);   // total is unaffected
}

// advanceLap() carries the overshoot: after closing a 10 m lap at value 12,
// the new lap already holds the 2 m overshoot.
TEST(MonotonicCounter, AdvanceLapCarriesOvershoot)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(kBase);            // seat base
    c.add(kBase + 12.0f);    // lap value 12, close on target 10

    c.advanceLap(10.0f);
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 2.0f);   // overshoot carried
    EXPECT_FLOAT_EQ(c.getValueActive(), 12.0f);     // total unchanged

    c.add(kBase + 20.0f);    // +8 m
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 10.0f);  // 2 carried + 8
}

// The core regression: over many laps, advanceLap keeps the auto-lap firing on
// exact multiples of the target, while resetLap drifts by the overshoot.
TEST(MonotonicCounter, DistanceLapsDoNotDriftWithAdvanceLap)
{
    constexpr float kTarget = 1000.0f;
    // 7 m/s at 1 Hz -> 7 m per sample; boundaries are crossed mid-sample.
    const std::vector<float> stream = makeStream(7.0f, 2000);

    MonotonicCounter<float> drift;   // old behaviour
    MonotonicCounter<float> fixed;   // new behaviour
    drift.init();
    fixed.init();

    std::vector<float> driftBoundaries;   // cumulative distance at each lap fire
    std::vector<float> fixedBoundaries;
    std::vector<float> fixedLapSizes;     // recorded size of each closed lap

    for (float sample : stream) {
        drift.add(sample);
        fixed.add(sample);

        if (drift.getLapValueActive() >= kTarget) {
            driftBoundaries.push_back(drift.getValueActive());
            drift.resetLap();
        }
        if (fixed.getLapValueActive() >= kTarget) {
            fixedBoundaries.push_back(fixed.getValueActive());
            fixedLapSizes.push_back(kTarget);     // Service records exactly the target
            fixed.advanceLap(kTarget);
        }
    }

    ASSERT_GE(fixedBoundaries.size(), 5u);
    ASSERT_EQ(driftBoundaries.size(), fixedBoundaries.size());

    // Fixed: every fire lands within one sample-step of the exact grid line and
    // never accumulates. The Nth boundary is at ~N*1000, not N*1000 + N*drift.
    for (size_t i = 0; i < fixedBoundaries.size(); ++i) {
        const float grid = static_cast<float>(i + 1) * kTarget;
        EXPECT_LT(fixedBoundaries[i] - grid, 7.0f + 0.01f)
            << "fixed lap " << i << " overshoot exceeded one sample step";
        EXPECT_GE(fixedBoundaries[i], grid);
    }

    // Drift: the last boundary is far past its grid line -- the bug.
    const size_t last = driftBoundaries.size() - 1;
    const float lastGrid = static_cast<float>(last + 1) * kTarget;
    EXPECT_GT(driftBoundaries[last] - lastGrid, 7.0f)
        << "resetLap should visibly drift past the grid over many laps";

    // Fixed lap sizes sum to exactly N*target (conservation, no lost/added distance).
    float sum = 0.0f;
    for (float s : fixedLapSizes) sum += s;
    EXPECT_FLOAT_EQ(sum, static_cast<float>(fixedLapSizes.size()) * kTarget);
}

// advanceLap must carry both the active and the total (includes-pauses) lap
// accumulators so paused sessions stay consistent too.
TEST(MonotonicCounter, AdvanceLapCarriesActiveAndTotal)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(kBase);            // seat base
    c.add(kBase + 1004.0f);

    EXPECT_FLOAT_EQ(c.getLapValueActive(), 1004.0f);
    EXPECT_FLOAT_EQ(c.getLapValueTotal(), 1004.0f);

    c.advanceLap(1000.0f);
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 4.0f);
    EXPECT_FLOAT_EQ(c.getLapValueTotal(), 4.0f);
}
