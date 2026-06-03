/**
 * @file CadenceSpikeFilter_test.cpp
 * @brief Unit tests for the app-side cadence median spike-filter.
 */

#include <gtest/gtest.h>

#include "SDK/Calibration/CadenceSpikeFilter.hpp"

using SDK::Calibration::CadenceSpikeFilter;

namespace
{

TEST(CadenceSpikeFilter, PassesThroughSteadyCadence)
{
    CadenceSpikeFilter f(5);
    // Once the window is primed, a steady stream is returned unchanged.
    for (int i = 0; i < 10; ++i) {
        EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);
    }
}

TEST(CadenceSpikeFilter, RemovesIsolatedDownwardSpike)
{
    CadenceSpikeFilter f(5);
    for (int i = 0; i < 5; ++i) {
        f.filter(170.0f);                 // prime the window at 170
    }
    // A single 140 sample is outvoted by the 170 neighbours -> stays 170.
    EXPECT_FLOAT_EQ(f.filter(140.0f), 170.0f);
    // And the next steady sample is also clean.
    EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);
}

TEST(CadenceSpikeFilter, RemovesIsolatedUpwardSpike)
{
    CadenceSpikeFilter f(5);
    for (int i = 0; i < 5; ++i) {
        f.filter(110.0f);
    }
    EXPECT_FLOAT_EQ(f.filter(200.0f), 110.0f);  // lone high outvoted
}

TEST(CadenceSpikeFilter, TracksGenuineStepWithBoundedLag)
{
    CadenceSpikeFilter f(5);
    for (int i = 0; i < 5; ++i) {
        f.filter(110.0f);                 // steady walk
    }
    // Genuine sustained step up to 170: median catches up once a majority of
    // the window (3 of 5) are the new value -> within window/2 + 1 samples.
    f.filter(170.0f);                     // [110 110 110 110 170] -> 110
    f.filter(170.0f);                     // [110 110 110 170 170] -> 110
    EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);  // [110 110 170 170 170] -> 170
    EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);
}

TEST(CadenceSpikeFilter, WindowOfOneIsPassThrough)
{
    CadenceSpikeFilter f(1);
    EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);
    EXPECT_FLOAT_EQ(f.filter(140.0f), 140.0f);  // no neighbours -> unchanged
}

TEST(CadenceSpikeFilter, ResetClearsHistory)
{
    CadenceSpikeFilter f(5);
    for (int i = 0; i < 5; ++i) {
        f.filter(170.0f);
    }
    f.reset();
    // After reset the first sample defines the median (median of one).
    EXPECT_FLOAT_EQ(f.filter(120.0f), 120.0f);
}

TEST(CadenceSpikeFilter, WindowIsClampedToMax)
{
    CadenceSpikeFilter f(99);
    EXPECT_EQ(f.window(), CadenceSpikeFilter::kMaxWindow);
}

TEST(CadenceSpikeFilter, EarlySamplesBeforeWindowFilled)
{
    CadenceSpikeFilter f(5);
    EXPECT_FLOAT_EQ(f.filter(170.0f), 170.0f);          // median of {170}
    EXPECT_FLOAT_EQ(f.filter(172.0f), 172.0f);          // median of {170,172} -> upper-middle
    EXPECT_FLOAT_EQ(f.filter(168.0f), 170.0f);          // median of {168,170,172}
}

} // namespace
