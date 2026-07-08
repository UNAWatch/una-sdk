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
// The stream starts at a non-zero base to mimic an absolute odometer; the
// lap-drift maths is independent of the base value.
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

// The base is seated on the very first sample, even when that sample is 0, so
// the first real movement after a zero start is counted rather than consumed
// as a second base point.
TEST(MonotonicCounter, FirstMovementFromZeroNotDropped)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(0.0f);   // seat base at 0
    c.add(5.0f);   // first movement -- must count, not re-seat the base

    EXPECT_FLOAT_EQ(c.getValueActive(), 5.0f);
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 5.0f);

    c.add(11.0f);
    EXPECT_FLOAT_EQ(c.getValueActive(), 11.0f);
}

// resetLap() closes the lap where it stands and starts the next lap there with
// no gap: the distance travelled between the lap event and the next sample is
// counted in the new lap, not dropped. (Used for manual / time / final laps.)
TEST(MonotonicCounter, ResetLapIsGapFree)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(kBase);            // seat base
    c.add(kBase + 6.0f);
    c.add(kBase + 12.0f);    // lap value now 12

    EXPECT_FLOAT_EQ(c.getLapValueActive(), 12.0f);
    c.resetLap();            // close the lap exactly here
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 0.0f);

    c.add(kBase + 18.0f);    // +6 m since the lap event
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 6.0f); // sliver counted, not dropped
    EXPECT_FLOAT_EQ(c.getValueActive(), 18.0f);   // total is unaffected
}

// resetLap() called mid-pause must stay consistent through resume(): the closed
// lap keeps its pre-pause active distance, and the new lap starts from the reset
// point with the paused distance correctly excluded from active. The old
// re-seat path re-based to a mid-pause sample and then had resume() add the
// pause offset on top, double-counting it into a NEGATIVE lap distance.
TEST(MonotonicCounter, ResetLapDuringPauseStaysConsistent)
{
    MonotonicCounter<float> c;
    c.init();

    c.add(0.0f);
    c.add(100.0f);          // 100 m of active distance this lap

    c.pause();              // pause at 100
    c.add(150.0f);          // sensor keeps moving while paused (not active)

    // Manual lap while paused: closes at the frozen active distance and the
    // includes-pauses total.
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 100.0f);
    EXPECT_FLOAT_EQ(c.getLapValueTotal(), 150.0f);
    c.resetLap();

    c.add(170.0f);          // still paused after the reset -- the old bug trigger
    c.resume();             // resume at 170; 70 m elapsed during the pause

    c.add(200.0f);          // 30 m of real movement after resuming

    // New lap counts only post-resume active movement; never negative.
    EXPECT_GE(c.getLapValueActive(), 0.0f);
    EXPECT_FLOAT_EQ(c.getLapValueActive(), 30.0f);
    // New lap's includes-pauses total spans the reset point (150) to 200 = 50
    // (20 m during the pause tail + 30 m active).
    EXPECT_FLOAT_EQ(c.getLapValueTotal(), 50.0f);
    // Session active distance excludes the 70 m paused: 100 + 30.
    EXPECT_FLOAT_EQ(c.getValueActive(), 130.0f);
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
// exact multiples of the target. resetLap is gap-free but does not carry the
// overshoot, so a resetLap-driven auto-lap still drifts by the accumulated
// overshoot -- which is exactly why the distance auto-lap uses advanceLap.
TEST(MonotonicCounter, DistanceLapsDoNotDriftWithAdvanceLap)
{
    constexpr float kTarget = 1000.0f;
    // 7 m/s at 1 Hz -> 7 m per sample; boundaries are crossed mid-sample.
    const std::vector<float> stream = makeStream(7.0f, 2000);

    MonotonicCounter<float> drift;   // resetLap: drifts by accumulated overshoot
    MonotonicCounter<float> fixed;   // advanceLap: pinned to the grid
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

// End-to-end mix of distance auto-laps and manual laps, mirroring the Service
// lap logic: auto-laps record exactly the target and carry (advanceLap); manual
// laps record the actual distance and reset (resetLap). Verifies (a) a manual
// lap re-syncs the auto grid to the manual point -- the next auto fires one full
// target later, not on the original grid -- and (b) every metre is accounted
// for: recorded laps + final partial sum to the total distance (no gap).
TEST(MonotonicCounter, MixedAutoAndManualLaps)
{
    constexpr float kTarget    = 1000.0f;
    constexpr float kManualAt  = 2500.0f;   // press lap here (getValueActive terms)
    // 7 m/s at 1 Hz; enough samples for autos at ~1k/2k, a manual at 2.5k, then
    // more autos out past 4.5k.
    const std::vector<float> stream = makeStream(7.0f, 800);

    MonotonicCounter<float> c;
    c.init();

    std::vector<float> recorded;        // every closed lap's recorded distance
    std::vector<float> autoFires;       // cumulative distance at each auto fire
    float manualFireDist = 0.0f;
    bool  manualDone      = false;

    for (float sample : stream) {
        c.add(sample);
        const float d = c.getValueActive();

        if (!manualDone && d >= kManualAt) {
            recorded.push_back(c.getLapValueActive());   // manual: actual distance
            manualFireDist = d;
            manualDone = true;
            c.resetLap();
        } else if (c.getLapValueActive() >= kTarget) {
            recorded.push_back(kTarget);                 // auto: exactly the target
            autoFires.push_back(d);
            c.advanceLap(kTarget);
        }
    }
    recorded.push_back(c.getLapValueActive());           // final partial lap
    const float total = c.getValueActive();

    ASSERT_TRUE(manualDone);

    // Two autos land on the grid before the manual lap.
    ASSERT_GE(autoFires.size(), 3u);
    EXPECT_NEAR(autoFires[0], 1000.0f, 7.0f);
    EXPECT_NEAR(autoFires[1], 2000.0f, 7.0f);

    // The first auto after the manual lap fires ~1 target past the manual point
    // (grid re-synced to the manual lap), NOT on the original 3000 m grid line.
    float firstAutoAfterManual = 0.0f;
    for (float f : autoFires) {
        if (f > manualFireDist) { firstAutoAfterManual = f; break; }
    }
    ASSERT_GT(firstAutoAfterManual, 0.0f);
    EXPECT_NEAR(firstAutoAfterManual, manualFireDist + kTarget, 7.0f);

    // Conservation / gap-free: recorded laps + final partial == total distance.
    // A dropped sub-sample sliver at the manual lap would make this short by
    // ~one sample step (7 m), far above the 0.5 m float-noise tolerance.
    float sum = 0.0f;
    for (float r : recorded) sum += r;
    EXPECT_NEAR(sum, total, 0.5f);
}
