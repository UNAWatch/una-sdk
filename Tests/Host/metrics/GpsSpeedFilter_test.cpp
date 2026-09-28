/**
 * @file GpsSpeedFilter_test.cpp
 * @brief Unit tests for SDK::Metric::GpsSpeedFilter
 */

#include <gtest/gtest.h>

#include "SDK/Metrics/GpsSpeedFilter.hpp"

#include <cmath>

namespace {

constexpr float kMinValid = 0.5f;
constexpr float kMaxValid = 300.0f;

constexpr float kEarthR   = 6371000.0f;
constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;

/**
 * @brief A walker heading due north from a fixed start, at a chosen true speed.
 *
 * The position is accumulated in double and only narrowed to float at the
 * boundary, which is how the real thing behaves: the receiver solves precisely
 * and we are handed a float. Accumulating in float instead biases the distance
 * LOW, because each addition of a small step to a large latitude loses its low
 * bits -- which is a property of the test rig, not of the filter.
 *
 * The default start latitude is near the equator so that float quantisation is
 * negligible and the tests measure the algorithm. PositionQuantisation below
 * covers the precision cost at a realistic latitude.
 */
class Track {
public:
    explicit Track(double startLat = 0.0, double startLon = 0.0)
        : mLat(startLat), mLon(startLon) {}

    /** @brief Advance one second at @p speedMs. */
    void advance(float speedMs)
    {
        mLat += static_cast<double>(speedMs) / (static_cast<double>(kEarthR) *
                                                static_cast<double>(kDegToRad));
    }
    float lat() const { return static_cast<float>(mLat); }
    float lon() const { return static_cast<float>(mLon); }

private:
    double mLat;
    double mLon;
};

using Filter = SDK::Metric::GpsSpeedFilter<5, 60, 5>;

Filter makeFilter()
{
    Filter f;
    EXPECT_TRUE(f.init(kMinValid, kMaxValid));
    return f;
}

/**
 * @brief Run @p seconds of a track at @p trueSpeed while the Doppler reports
 *        @p reportedSpeed, i.e. a receiver whose speed is off by a known factor.
 */
void feed(Filter &f, Track &t, float trueSpeed, float reportedSpeed, int seconds,
          bool speedValid = true, bool posValid = true)
{
    for (int i = 0; i < seconds; i++) {
        t.advance(trueSpeed);
        f.tick(reportedSpeed, speedValid, t.lat(), t.lon(), posValid);
    }
}

}  // namespace

TEST(GpsSpeedFilter, InitRejectsInvertedRange)
{
    Filter f;
    EXPECT_FALSE(f.init(10.0f, 10.0f));
    EXPECT_FALSE(f.init(20.0f, 10.0f));
    EXPECT_TRUE(f.init(kMinValid, kMaxValid));
}

TEST(GpsSpeedFilter, ReportsNothingBeforeAnySample)
{
    Filter f = makeFilter();
    EXPECT_FALSE(f.isValid());
    EXPECT_FLOAT_EQ(0.0f, f.getSpeed());
    EXPECT_FLOAT_EQ(0.0f, f.getPace());
    EXPECT_FLOAT_EQ(1.0f, f.getScale()) << "scale starts neutral";
    EXPECT_FALSE(f.isScaleMeasured());
}

TEST(GpsSpeedFilter, IgnoredWithoutInit)
{
    Filter f;  // no init()
    Track t;
    t.advance(3.0f);
    f.tick(3.0f, true, t.lat(), t.lon(), true);
    EXPECT_FALSE(f.isValid());
    EXPECT_FLOAT_EQ(0.0f, f.getSpeed());
}

TEST(GpsSpeedFilter, BehavesAsPlainSmoothingUntilScaleIsMeasured)
{
    Filter f = makeFilter();
    Track t;
    // Only a couple of ticks: nowhere near the third of the scale window needed.
    feed(f, t, 3.0f, 3.0f, 3);
    EXPECT_FALSE(f.isScaleMeasured());
    EXPECT_FLOAT_EQ(1.0f, f.getScale());
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.01f);
}

TEST(GpsSpeedFilter, CorrectsASpeedThatReadsLow)
{
    Filter f = makeFilter();
    Track t;
    // The receiver reports 2.85 m/s while the position track covers 3.00 m/s.
    feed(f, t, 3.0f, 2.85f, 120);

    EXPECT_TRUE(f.isScaleMeasured());
    EXPECT_NEAR(3.0f / 2.85f, f.getScale(), 0.01f);
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.03f) << "the corrected speed tracks the position";
    EXPECT_NEAR(1.0f / 3.0f, f.getPace(), 0.005f);
}

TEST(GpsSpeedFilter, CorrectsASpeedThatReadsHigh)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.20f, 120);
    EXPECT_NEAR(3.0f / 3.20f, f.getScale(), 0.01f);
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.03f);
}

TEST(GpsSpeedFilter, LeavesAnAccurateSpeedAlone)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.0f, 120);
    EXPECT_NEAR(1.0f, f.getScale(), 0.01f) << "nothing to correct, so k stays near 1";
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.03f);
}

TEST(GpsSpeedFilter, ScaleDoesNotDecayTowardOneWhenHeld)
{
    // THE FEEDBACK TRAP. k must be measured against the RAW Doppler. If it were
    // ever measured against this filter's own corrected output, each update would
    // find the "error" already removed, k would walk back to 1.0, and the
    // correction would silently vanish while still looking plausible.
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 2.85f, 120);
    const float early = f.getScale();
    ASSERT_NEAR(3.0f / 2.85f, early, 0.01f);

    feed(f, t, 3.0f, 2.85f, 600);          // ten more scale windows of the same
    EXPECT_NEAR(early, f.getScale(), 0.005f) << "k must hold, not decay to 1.0";
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.03f);
}

TEST(GpsSpeedFilter, ScaleIsClampedBothWays)
{
    {   // position says we are going far faster than the Doppler claims
        Filter f = makeFilter();
        Track t;
        feed(f, t, 6.0f, 2.0f, 120);
        EXPECT_FLOAT_EQ(Filter::getScaleMax(), f.getScale());
    }
    {   // and far slower
        Filter f = makeFilter();
        Track t;
        feed(f, t, 2.0f, 6.0f, 120);
        EXPECT_FLOAT_EQ(Filter::getScaleMin(), f.getScale());
    }
}

TEST(GpsSpeedFilter, SmoothingRespondsWithinItsWindow)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.0f, 120);
    ASSERT_NEAR(3.0f, f.getSpeed(), 0.03f);

    // A step up. The 5-tick window must have fully taken it after 5 ticks.
    const float before = f.getSpeed();
    feed(f, t, 4.0f, 4.0f, 5);
    EXPECT_GT(f.getSpeed(), before);
    EXPECT_NEAR(4.0f * f.getScale(), f.getSpeed(), 0.03f);
}

TEST(GpsSpeedFilter, InstantSpeedIsScaledButNotSmoothed)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 2.85f, 120);
    const float k = f.getScale();

    // One fast sample: the smoothed value moves a fifth of the way, the instant
    // value all of it. This is why a lap maximum should use the instant value.
    t.advance(3.0f);
    f.tick(5.0f, true, t.lat(), t.lon(), true);
    EXPECT_NEAR(5.0f * k, f.getInstantSpeed(), 0.02f);
    EXPECT_LT(f.getSpeed(), 4.0f * k) << "the smoothed value must lag a single spike";
}

TEST(GpsSpeedFilter, LostFixAgesOutOfTheSmoothingWindow)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.0f, 60);
    ASSERT_TRUE(f.isValid());

    for (int i = 0; i < 5; i++) {
        f.tick(0.0f, false, 0.0f, 0.0f, false);
    }
    EXPECT_FALSE(f.isValid());
    EXPECT_FLOAT_EQ(0.0f, f.getSpeed());
    EXPECT_FLOAT_EQ(0.0f, f.getPace());
}

TEST(GpsSpeedFilter, ScaleSurvivesAShortFixLoss)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 2.85f, 120);
    const float k = f.getScale();

    for (int i = 0; i < 5; i++) {
        f.tick(0.0f, false, 0.0f, 0.0f, false);
    }
    EXPECT_NEAR(k, f.getScale(), 0.01f) << "a brief dropout must not reset the correction";

    feed(f, t, 3.0f, 2.85f, 5);
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.05f) << "and the readout returns corrected";
}

TEST(GpsSpeedFilter, SpeedWithoutPositionStillSmoothsAtTheLastScale)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 2.85f, 120);
    const float k = f.getScale();

    // Speed keeps arriving but position does not: k holds, smoothing continues.
    for (int i = 0; i < 30; i++) {
        f.tick(2.85f, true, 0.0f, 0.0f, false);
    }
    EXPECT_NEAR(k, f.getScale(), 0.02f);
    EXPECT_NEAR(2.85f * k, f.getSpeed(), 0.02f);
}

TEST(GpsSpeedFilter, RejectsOutOfRangeSpeedSamples)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.0f, 60);
    const std::size_t before = f.getSampleCount();

    t.advance(3.0f);
    f.tick(kMaxValid + 10.0f, true, t.lat(), t.lon(), true);   // bogus
    t.advance(3.0f);
    f.tick(-1.0f, true, t.lat(), t.lon(), true);               // impossible
    EXPECT_LT(f.getSampleCount(), before + 1);
    EXPECT_NEAR(3.0f * f.getScale(), f.getSpeed(), 0.1f);
}

TEST(GpsSpeedFilter, NoPaceWhileStopped)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 0.2f, 0.2f, 60);
    EXPECT_TRUE(f.isValid());
    EXPECT_FLOAT_EQ(0.0f, f.getPace()) << "a stopped runner has no pace to show";
}

TEST(GpsSpeedFilter, ResetClearsHistoryAndScale)
{
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 2.85f, 120);
    ASSERT_TRUE(f.isScaleMeasured());

    f.reset();
    EXPECT_FALSE(f.isValid());
    EXPECT_FALSE(f.isScaleMeasured());
    EXPECT_FLOAT_EQ(1.0f, f.getScale()) << "back to neutral, not the old factor";
    EXPECT_FLOAT_EQ(0.0f, f.getSpeed());
    EXPECT_FLOAT_EQ(kMinValid, f.getMinValid());
    EXPECT_FLOAT_EQ(kMaxValid, f.getMaxValid());
}

TEST(GpsSpeedFilter, ChordAveragingBeatsPerSecondAgainstPositionNoise)
{
    // Per-second position noise inflates a summed path; a chord averages it out
    // before it reaches k. With a jittered track the chord filter must stay much
    // closer to the truth than a 1-tick chord does.
    auto run = [](auto &filter) {
        Track t;
        float wobble = 0.0f;
        for (int i = 0; i < 600; i++) {
            t.advance(3.0f);
            wobble = (i % 2 == 0) ? 4.0e-5f : -4.0e-5f;   // ~4 m of side-to-side
            filter.tick(3.0f, true, t.lat(), t.lon() + wobble, true);
        }
        return filter.getScale();
    };
    SDK::Metric::GpsSpeedFilter<5, 60, 5> chord5;
    SDK::Metric::GpsSpeedFilter<5, 60, 1> chord1;
    ASSERT_TRUE(chord5.init(kMinValid, kMaxValid));
    ASSERT_TRUE(chord1.init(kMinValid, kMaxValid));
    const float k5 = run(chord5);
    const float k1 = run(chord1);
    EXPECT_LT(std::fabs(k5 - 1.0f), std::fabs(k1 - 1.0f))
        << "5-tick chord k=" << k5 << " vs 1-tick chord k=" << k1;
}

TEST(GpsSpeedFilter, DefaultTemplateArgumentsAreTheShippedConfiguration)
{
    SDK::Metric::GpsSpeedFilter<> f;
    EXPECT_EQ(5u,   f.getSmoothTicks());
    EXPECT_EQ(180u, f.getScaleTauTicks());
    EXPECT_EQ(5u,   f.getChordTicks());
    ASSERT_TRUE(f.init(kMinValid, kMaxValid));
    Track t;
    for (int i = 0; i < 400; i++) {
        t.advance(3.0f);
        f.tick(2.85f, true, t.lat(), t.lon(), true);
    }
    EXPECT_NEAR(3.0f / 2.85f, f.getScale(), 0.02f);
    EXPECT_NEAR(3.0f, f.getSpeed(), 0.05f);
}

TEST(GpsSpeedFilter, PositionQuantisationAtRealLatitudeStaysWithinAPercent)
{
    // Positions reach us as float. At UK latitudes one ulp is about 0.7 m, so a
    // 5 s chord of ~15 m carries a couple of percent of noise -- zero-mean, so it
    // averages down across the scale window rather than biasing k. This pins that
    // the residual stays small enough not to matter beside the error being fixed.
    SDK::Metric::GpsSpeedFilter<5, 180, 5> f;
    ASSERT_TRUE(f.init(kMinValid, kMaxValid));
    Track t(51.5, -1.5);
    for (int i = 0; i < 400; i++) {
        t.advance(3.0f);
        f.tick(3.0f, true, t.lat(), t.lon(), true);
    }
    EXPECT_NEAR(1.0f, f.getScale(), 0.01f)
        << "float position quantisation must not move k by more than a percent";
}

TEST(GpsSpeedFilter, StandingStillDoesNotWalkTheScaleUpward)
{
    // Stopped at a crossing the Doppler correctly reads about zero, but the
    // position still wanders by a few metres, so the chord reads a speed that is
    // not there. Admitted into the average, such a tick adds far more to the
    // numerator than to the denominator and teaches the filter that the receiver
    // reads low. Ticks at or below the floor are therefore excluded entirely.
    Filter f = makeFilter();
    Track t;
    feed(f, t, 3.0f, 3.0f, 120);                  // honest running: k settles at 1
    const float k = f.getScale();
    ASSERT_NEAR(1.0f, k, 0.02f);

    // Two minutes stationary, the position drifting back and forth a few metres.
    for (int i = 0; i < 120; i++) {
        const float wobble = (i % 2 == 0) ? 3.0e-5f : -3.0e-5f;
        f.tick(0.05f, true, t.lat() + wobble, t.lon(), true);
    }
    EXPECT_NEAR(k, f.getScale(), 0.01f)
        << "a stop must not move the scale factor; got " << f.getScale();
}
