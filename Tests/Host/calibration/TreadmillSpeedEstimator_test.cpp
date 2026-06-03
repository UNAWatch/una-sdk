#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "FakeFileSystem.hpp"

#include "SDK/Calibration/CadenceStrideModel.hpp"
#include "SDK/Calibration/CadenceStrideModelConfig.hpp"
#include "SDK/Calibration/StrideLut.hpp"
#include "SDK/Calibration/TreadmillSpeedEstimator.hpp"

using SDK::Calibration::CadenceStrideModel;
using SDK::Calibration::Phase;
using SDK::Calibration::StrideLut;
using SDK::Calibration::TreadmillSpeedEstimator;
namespace Cfg = SDK::Calibration::Config;

namespace
{

constexpr const char *kOutPath  = "../SharedData/stride.json";
constexpr const char *kDeltaPath = "treadmill_delta.json";

struct SeedBin {
    float centre;
    float distance;
    float steps;
    float count;
};

std::string makeStoreJson(int version, const std::vector<SeedBin> &bins)
{
    std::ostringstream os;
    os << "{\"version\":" << version << ",\"bins\":[";
    for (size_t i = 0; i < bins.size(); ++i) {
        if (i != 0) {
            os << ",";
        }
        os << "{\"centre_spm\":" << bins[i].centre
           << ",\"total_distance_m\":" << bins[i].distance
           << ",\"total_steps\":" << bins[i].steps
           << ",\"sample_count\":" << bins[i].count << "}";
    }
    os << "]}";
    return os.str();
}

// A phase-1 model: demographic stride is constant, so the estimator math is
// easy to check by hand. height 1.75 → SL = 0.685 * 1.75 = 1.19875 m.
constexpr float kDemoStride = 0.685f * 1.75f;

} // namespace

// --- Tick integration --------------------------------------------------------

TEST(TreadmillSpeedEstimator, SteadyCadenceIntegratesDistanceAndHistogram)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);  // phase 1, demographic stride
    ASSERT_EQ(m.phase(), Phase::UNCALIBRATED);

    TreadmillSpeedEstimator est(m);
    est.startSession();

    const float cadence = 160.0f;
    const float stride  = m.treadmillStrideLengthM(cadence);  // == kDemoStride
    const float speed   = (cadence / 120.0f) * stride;

    for (int i = 0; i < 10; ++i) {
        est.tick(cadence, true, 1.0f);
    }

    EXPECT_TRUE(est.speedValid());
    EXPECT_NEAR(est.speedMps(), speed, 1e-4f);
    EXPECT_NEAR(est.distanceM(), speed * 10.0f, 1e-3f);

    // Steps land in bin 20 (centre 162 covers 160): 160 spm * 10 s / 60 = 26.67.
    const float *hist = est.stepHistogram();
    EXPECT_NEAR(hist[StrideLut::binIndexForCadence(cadence)],
                cadence * 10.0f / 60.0f, 1e-3f);
    EXPECT_NEAR(hist[0], 0.0f, 1e-6f);
}

TEST(TreadmillSpeedEstimator, SpeedEqualsCadenceOver120TimesStride)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    est.tick(180.0f, true, 1.0f);
    EXPECT_NEAR(est.speedMps(), (180.0f / 120.0f) * kDemoStride, 1e-4f);
}

// --- Hold-forward ------------------------------------------------------------

TEST(TreadmillSpeedEstimator, HoldForwardWithinWindowKeepsIntegrating)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    const float cadence = 160.0f;
    const float speed   = (cadence / 120.0f) * kDemoStride;

    est.tick(cadence, true, 1.0f);  // 1 valid tick latches the held value
    const float distAfterValid = est.distanceM();
    const float stepsAfterValid =
        est.stepHistogram()[StrideLut::binIndexForCadence(cadence)];

    // 5 invalid ticks: all within the 5 s window → keep integrating at held.
    for (int i = 0; i < 5; ++i) {
        est.tick(0.0f, false, 1.0f);
        EXPECT_TRUE(est.speedValid());
        EXPECT_NEAR(est.speedMps(), speed, 1e-4f);
    }
    EXPECT_NEAR(est.distanceM(), distAfterValid + speed * 5.0f, 1e-3f);
    // Held S_i accrues at the held cadence's bin, consistent with held distance.
    EXPECT_NEAR(est.stepHistogram()[StrideLut::binIndexForCadence(cadence)],
                stepsAfterValid + cadence * 5.0f / 60.0f, 1e-3f);

    // 6th invalid tick exceeds 5 s → speed 0, no further integration.
    const float distBefore = est.distanceM();
    est.tick(0.0f, false, 1.0f);
    EXPECT_FALSE(est.speedValid());
    EXPECT_FLOAT_EQ(est.speedMps(), 0.0f);
    EXPECT_NEAR(est.distanceM(), distBefore, 1e-6f);
}

TEST(TreadmillSpeedEstimator, RevalidationReLatchesAndResumes)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    est.tick(160.0f, true, 1.0f);
    for (int i = 0; i < 6; ++i) {  // hold expires
        est.tick(0.0f, false, 1.0f);
    }
    ASSERT_FALSE(est.speedValid());

    // Re-acquire at a new cadence → re-latches and resumes.
    est.tick(180.0f, true, 1.0f);
    EXPECT_TRUE(est.speedValid());
    EXPECT_NEAR(est.speedMps(), (180.0f / 120.0f) * kDemoStride, 1e-4f);
}

TEST(TreadmillSpeedEstimator, NoHeldSampleMeansNoIntegration)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    // Invalid cadence from the very first tick → nothing to hold.
    est.tick(0.0f, false, 1.0f);
    EXPECT_FALSE(est.speedValid());
    EXPECT_FLOAT_EQ(est.distanceM(), 0.0f);
}

// --- Pause / resume ----------------------------------------------------------

TEST(TreadmillSpeedEstimator, PauseHaltsIntegrationEvenWithinHold)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    est.tick(160.0f, true, 1.0f);   // latch a held value
    const float dist = est.distanceM();

    est.pause();
    EXPECT_FALSE(est.speedValid());
    EXPECT_FLOAT_EQ(est.speedMps(), 0.0f);

    // Ticks during pause do not integrate, even though a hold value exists.
    est.tick(0.0f, false, 1.0f);
    est.tick(160.0f, true, 1.0f);
    EXPECT_FLOAT_EQ(est.distanceM(), dist);
    EXPECT_FALSE(est.speedValid());

    // Resume: integration continues on the next valid tick.
    est.resume();
    est.tick(160.0f, true, 1.0f);
    EXPECT_TRUE(est.speedValid());
    EXPECT_GT(est.distanceM(), dist);
}

TEST(TreadmillSpeedEstimator, ResumeClearsHoldState)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    est.tick(160.0f, true, 1.0f);
    est.pause();
    est.resume();

    // After resume the hold is cleared: an invalid tick cannot integrate.
    const float dist = est.distanceM();
    est.tick(0.0f, false, 1.0f);
    EXPECT_FALSE(est.speedValid());
    EXPECT_FLOAT_EQ(est.distanceM(), dist);
}

// --- dt clamp ----------------------------------------------------------------

TEST(TreadmillSpeedEstimator, AnomalousDtClampedToMaxTickGap)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    const float cadence = 160.0f;
    const float speed   = (cadence / 120.0f) * kDemoStride;

    est.tick(cadence, true, 100.0f);  // huge dt → clamped to kMaxTickGapS
    EXPECT_NEAR(est.distanceM(), speed * Cfg::kMaxTickGapS, 1e-3f);
}

TEST(TreadmillSpeedEstimator, NonPositiveDtDoesNotIntegrate)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    TreadmillSpeedEstimator est(m);
    est.startSession();

    est.tick(160.0f, true, 0.0f);
    EXPECT_FLOAT_EQ(est.distanceM(), 0.0f);
    est.tick(160.0f, true, -5.0f);
    EXPECT_FLOAT_EQ(est.distanceM(), 0.0f);
}

// --- End-to-end convergence --------------------------------------------------

TEST(TreadmillSpeedEstimator, EndToEndDeltaConvergesTowardActual)
{
    SDK::Test::FakeFileSystem fs;
    // Phase-2 LUT: 8 valid bins, SL 1.3 each.
    std::vector<SeedBin> bins;
    for (int i = 0; i < 8; ++i) {
        bins.push_back({82.0f + 4.0f * i, 650.0f, 1000.0f, 50.0f});
    }
    fs.seedFile(kOutPath, makeStoreJson(1, bins));

    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);

    TreadmillSpeedEstimator est(m);
    est.startSession();

    // Drive a steady cadence trace (within the valid bins, e.g. 100 spm).
    const float cadence = 100.0f;
    for (int i = 0; i < 600; ++i) {
        est.tick(cadence, true, 1.0f);
    }
    const float dEst = est.distanceM();
    ASSERT_GT(dEst, 0.0f);

    // Console says we actually went 10% further. Within both gates → accepted.
    const float dActual = dEst * 1.10f;
    auto r = m.applyPostRunCalibration(est.stepHistogram(), dEst, dActual);
    ASSERT_TRUE(r.dActualAccepted);
    ASSERT_TRUE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, dActual);

    // A second identical session now estimates a longer distance (Δ nudged the
    // stride upward), converging toward the actual.
    TreadmillSpeedEstimator est2(m);
    est2.startSession();
    for (int i = 0; i < 600; ++i) {
        est2.tick(cadence, true, 1.0f);
    }
    EXPECT_GT(est2.distanceM(), dEst);
    EXPECT_LE(est2.distanceM(), dActual + 1e-3f);
}
