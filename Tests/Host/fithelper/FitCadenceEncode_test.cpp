#include <gtest/gtest.h>

#include "SDK/FitHelper/FitRecordCadence.hpp"

#include <limits>

TEST(FitCadenceEncodeTest, SplitsWholeAndFractionalCadence)
{
    const auto fields = SDK::FitRecordCadence::encodeCadenceSpm(175.5f);
    EXPECT_EQ(fields.cadence, 87u);
    EXPECT_EQ(fields.fractionalCadence, 96u);
}

TEST(FitCadenceEncodeTest, HalvesStepsPerMinForFitStridesPerMin)
{
    const auto fields = SDK::FitRecordCadence::encodeCadenceSpm(170.0f);
    EXPECT_EQ(fields.cadence, 85u);
    EXPECT_EQ(fields.fractionalCadence, 0u);
}

TEST(FitCadenceEncodeTest, ClampsFractionalCadenceToFitSevenBitRange)
{
    const auto fields = SDK::FitRecordCadence::encodeCadenceSpm(175.999f);
    EXPECT_EQ(fields.cadence, 87u);
    EXPECT_EQ(fields.fractionalCadence, 127u);
}

TEST(FitCadenceEncodeTest, ClampsCadenceToFitUint8Range)
{
    const auto fields = SDK::FitRecordCadence::encodeCadenceSpm(1000.0f);
    EXPECT_EQ(fields.cadence, 255u);
    EXPECT_EQ(fields.fractionalCadence, 0u);
}

TEST(FitCadenceEncodeTest, NonFiniteCadenceEncodesAsZero)
{
    const auto fields = SDK::FitRecordCadence::encodeCadenceSpm(
        std::numeric_limits<float>::infinity());
    EXPECT_EQ(fields.cadence, 0u);
    EXPECT_EQ(fields.fractionalCadence, 0u);
}

TEST(FitCadenceEncodeTest, EncodesStepLengthMToTenthMillimeterUnits)
{
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(0.0f), 0u);
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(0.01f), 100u);
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(0.15f), 1500u);
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(0.75f), 7500u);
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(1.0f), 10000u);
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(2.5f), 25000u);
}

TEST(FitCadenceEncodeTest, ClampsStepLengthToFitUint16Range)
{
    EXPECT_EQ(SDK::FitRecordCadence::encodeStepLengthM(10.0f), 65535u);
}

TEST(FitCadenceEncodeTest, NonFiniteStepLengthEncodesAsZero)
{
    EXPECT_EQ(
        SDK::FitRecordCadence::encodeStepLengthM(std::numeric_limits<float>::quiet_NaN()),
        0u);
}
