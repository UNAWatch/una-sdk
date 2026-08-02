/**
 ******************************************************************************
 * @file    Utils_test.cpp
 * @brief   Tests for the SDK::Utils unit conversions and time decomposition.
 *
 * The static_asserts are the point. constexpr here buys the right to be called
 * from a constexpr caller, nothing at runtime; breaking that must fail a build.
 ******************************************************************************
 */

#include "SDK/Utils/Utils.hpp"

#include <gtest/gtest.h>

#include <cstdint>

using SDK::Utils::feetToMeters;
using SDK::Utils::kmToMiles;
using SDK::Utils::metersToFeet;
using SDK::Utils::milesToKm;
using SDK::Utils::toHMS;

namespace
{

/** @brief constexpr magnitude comparison; std::abs is not constexpr in C++17. */
constexpr bool withinTolerance(float a, float b, float tolerance)
{
    return ((a > b) ? (a - b) : (b - a)) <= tolerance;
}

} // namespace

// =============================================================================
// Compile-time coverage
// =============================================================================

// Each helper is a single multiply or divide by its factor, so feeding it the
// factor is exact in float and pins the constant itself.
static_assert(kmToMiles(1.0f) == 0.621371f, "kmToMiles must scale by 0.621371");
static_assert(metersToFeet(1.0f) == 3.28084f, "metersToFeet must scale by 3.28084");
static_assert(milesToKm(0.621371f) == 1.0f, "milesToKm must invert kmToMiles' factor");
static_assert(feetToMeters(3.28084f) == 1.0f, "feetToMeters must invert metersToFeet' factor");

static_assert(kmToMiles(0.0f) == 0.0f, "");
static_assert(milesToKm(0.0f) == 0.0f, "");
static_assert(metersToFeet(0.0f) == 0.0f, "");
static_assert(feetToMeters(0.0f) == 0.0f, "");

static_assert(kmToMiles(-1.0f) == -0.621371f, "conversions must carry sign");
static_assert(metersToFeet(-1.0f) == -3.28084f, "conversions must carry sign");

// Against the defined values of the units, not against the implementation.
static_assert(withinTolerance(milesToKm(1.0f), 1.609344f, 1.0e-5f), "1 mile is 1609.344 m");
static_assert(withinTolerance(feetToMeters(5280.0f), 1609.344f, 1.0e-2f), "5280 ft is 1 mile");
static_assert(withinTolerance(metersToFeet(100.0f), 328.084f, 1.0e-2f), "");
static_assert(withinTolerance(kmToMiles(42.195f), 26.21875f, 1.0e-4f), "a marathon in miles");

// Round trips, which the shared factor makes exact to well under a display digit.
static_assert(withinTolerance(milesToKm(kmToMiles(42.195f)), 42.195f, 1.0e-3f), "");
static_assert(withinTolerance(feetToMeters(metersToFeet(1000.0f)), 1000.0f, 1.0e-3f), "");

static_assert(toHMS(3661).h == 1, "");
static_assert(toHMS(3661).m == 1, "");
static_assert(toHMS(3661).s == 1, "");
static_assert(toHMS(0).h == 0 && toHMS(0).m == 0 && toHMS(0).s == 0, "");
static_assert(toHMS(59).h == 0 && toHMS(59).m == 0 && toHMS(59).s == 59, "");
static_assert(toHMS(3600).h == 1 && toHMS(3600).m == 0 && toHMS(3600).s == 0, "");
static_assert(toHMS(86399).h == 23 && toHMS(86399).m == 59 && toHMS(86399).s == 59, "");
static_assert(toHMS(360000).h == 100, "hours are not wrapped at 24");

// =============================================================================
// Runtime coverage
// =============================================================================

TEST(Utils, ConversionsPinTheirFactors)
{
    EXPECT_FLOAT_EQ(kmToMiles(1.0f), 0.621371f);
    EXPECT_FLOAT_EQ(metersToFeet(1.0f), 3.28084f);
    EXPECT_FLOAT_EQ(milesToKm(0.621371f), 1.0f);
    EXPECT_FLOAT_EQ(feetToMeters(3.28084f), 1.0f);
}

TEST(Utils, ConversionsMatchTheDefinedUnits)
{
    EXPECT_NEAR(milesToKm(1.0f), 1.609344f, 1.0e-5f);
    EXPECT_NEAR(feetToMeters(5280.0f), 1609.344f, 1.0e-2f);
    EXPECT_NEAR(metersToFeet(100.0f), 328.084f, 1.0e-2f);
    EXPECT_NEAR(kmToMiles(42.195f), 26.21875f, 1.0e-4f);
}

TEST(Utils, ConversionsRoundTrip)
{
    for (const float km : {0.0f, 0.4f, 5.0f, 42.195f, 250.0f}) {
        EXPECT_NEAR(milesToKm(kmToMiles(km)), km, 1.0e-3f) << "km = " << km;
    }
    for (const float m : {0.0f, 1.0f, 100.0f, 1000.0f, 8848.0f}) {
        EXPECT_NEAR(feetToMeters(metersToFeet(m)), m, 1.0e-2f) << "m = " << m;
    }
}

TEST(Utils, ConversionsCarrySign)
{
    EXPECT_FLOAT_EQ(kmToMiles(-1.0f), -0.621371f);
    EXPECT_FLOAT_EQ(metersToFeet(-1.0f), -3.28084f);
    EXPECT_NEAR(feetToMeters(metersToFeet(-12.0f)), -12.0f, 1.0e-3f);
}

TEST(Utils, ToHMSDecomposesAndDoesNotWrapAtADay)
{
    const auto t = toHMS(3661);
    EXPECT_EQ(t.h, 1u);
    EXPECT_EQ(t.m, 1u);
    EXPECT_EQ(t.s, 1u);

    EXPECT_EQ(toHMS(86399).h, 23u);
    EXPECT_EQ(toHMS(86399).m, 59u);
    EXPECT_EQ(toHMS(86399).s, 59u);

    // A stopwatch past a day keeps counting hours rather than rolling over.
    EXPECT_EQ(toHMS(360000).h, 100u);
    EXPECT_EQ(toHMS(360000).m, 0u);
}

TEST(Utils, ToHMSRecomposes)
{
    for (const std::time_t sec : {std::time_t{0}, std::time_t{1}, std::time_t{59},
                                  std::time_t{60}, std::time_t{3599}, std::time_t{3600},
                                  std::time_t{3661}, std::time_t{86399}, std::time_t{123456}}) {
        const auto hms = toHMS(sec);
        EXPECT_LT(hms.m, 60u) << "sec = " << sec;
        EXPECT_LT(hms.s, 60u) << "sec = " << sec;
        EXPECT_EQ(static_cast<std::time_t>(hms.h) * 3600 + hms.m * 60 + hms.s, sec);
    }
}
