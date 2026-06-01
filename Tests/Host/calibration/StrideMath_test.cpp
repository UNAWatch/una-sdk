#include <gtest/gtest.h>

#include <limits>

#include "SDK/Calibration/StrideMath.hpp"

using SDK::Calibration::StrideMath::impliedStepLengthM;
using SDK::Calibration::StrideMath::impliedStrideLengthM;
namespace SM = SDK::Calibration::StrideMath;

TEST(StrideMath, StepLengthMatchesFormulaWhenValid)
{
    // 3 m/s at 160 SPM → 3*60/160 = 1.125 m, within [0.15, 2.50].
    const auto r = impliedStepLengthM(3.0f, true, 160.0f, true);
    EXPECT_TRUE(r.valid);
    EXPECT_NEAR(r.meters, 1.125f, 1e-4f);
}

TEST(StrideMath, StepLengthInvalidWhenSpeedInvalid)
{
    const auto r = impliedStepLengthM(3.0f, false, 160.0f, true);
    EXPECT_FALSE(r.valid);
}

TEST(StrideMath, StepLengthInvalidWhenCadenceInvalid)
{
    const auto r = impliedStepLengthM(3.0f, true, 160.0f, false);
    EXPECT_FALSE(r.valid);
}

TEST(StrideMath, StepLengthInvalidWhenCadenceNonPositive)
{
    const auto r = impliedStepLengthM(3.0f, true, 0.0f, true);
    EXPECT_FALSE(r.valid);
}

TEST(StrideMath, StepLengthGatedAboveMax)
{
    // 8 m/s at 160 SPM → 3.0 m > kMaxStepLengthM (2.50) → invalid.
    const auto r = impliedStepLengthM(8.0f, true, 160.0f, true);
    EXPECT_FALSE(r.valid);
    EXPECT_NEAR(r.meters, 3.0f, 1e-4f);
}

TEST(StrideMath, StepLengthInvalidForNonFiniteSpeed)
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    EXPECT_FALSE(impliedStepLengthM(nan, true, 160.0f, true).valid);
    EXPECT_FALSE(impliedStepLengthM(inf, true, 160.0f, true).valid);
}

TEST(StrideMath, StepLengthGatedBelowMin)
{
    // 0.5 m/s at 220 SPM → 0.136 m < kMinStepLengthM (0.15) → invalid.
    const auto r = impliedStepLengthM(0.5f, true, 220.0f, true);
    EXPECT_FALSE(r.valid);
}

TEST(StrideMath, StepLengthBoundaryValuesAreInclusive)
{
    // step = v*60/cad. For cad=60, step = v, so v=kMin/kMax lands exactly on
    // the inclusive emit-gate boundaries.
    EXPECT_TRUE(impliedStepLengthM(SM::kMinStepLengthM, true, 60.0f, true).valid);
    EXPECT_TRUE(impliedStepLengthM(SM::kMaxStepLengthM, true, 60.0f, true).valid);
}

TEST(StrideMath, StrideLengthIsTwiceTheStep)
{
    // stride = v*120/cad = 2 * step.
    EXPECT_NEAR(impliedStrideLengthM(3.0f, 160.0f), 2.25f, 1e-4f);
}

TEST(StrideMath, StrideLengthGuardsNonPositiveCadence)
{
    EXPECT_FLOAT_EQ(impliedStrideLengthM(3.0f, 0.0f), 0.0f);
    EXPECT_FLOAT_EQ(impliedStrideLengthM(3.0f, -10.0f), 0.0f);
}
