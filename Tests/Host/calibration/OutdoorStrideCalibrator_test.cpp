#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "FakeFileSystem.hpp"

#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"
#include "SDK/Calibration/OutdoorStrideCalibrator.hpp"

using SDK::Calibration::CalibratorSample;
using SDK::Calibration::OutdoorStrideCalibrator;
using SDK::Calibration::StrideBin;
namespace Cfg = SDK::Calibration::Config;

namespace
{

constexpr const char *kPath = "../SharedData/stride.json";
constexpr const char *kBakPath = "../SharedData/stride.json.bak";

// A sample that passes every non-steady-state gate. Defaults: 3 m/s, 160 SPM,
// flat. SL_implied = 3 * 120 / 160 = 2.25 m (in [0.3, 5.0]); bin index 20.
CalibratorSample goodSample(float speed = 3.0f, float cadence = 160.0f,
                            float grade = 0.0f, float dt = 1.0f)
{
    CalibratorSample s;
    s.gps_speed_ms           = speed;
    s.gps_speed_valid        = true;
    s.gps_fix_dead_reckoning = false;
    s.cadence_spm            = cadence;
    s.cadence_valid          = true;
    s.grade_pct              = grade;
    s.grade_valid            = true;
    s.delta_t_s              = dt;
    return s;
}

void feed(OutdoorStrideCalibrator &c, const CalibratorSample &s, int n)
{
    for (int i = 0; i < n; ++i) {
        c.ingestSample(s);
    }
}

struct SeedBin {
    float centre;
    float distance;
    float steps;
    float count;
};

std::string makeStoreJson(int version, const std::vector<SeedBin> &bins,
                          bool withUnknownKeys = false)
{
    std::ostringstream os;
    os << "{\"version\":" << version;
    if (withUnknownKeys) {
        os << ",\"future_top_key\":123";
    }
    os << ",\"bins\":[";
    for (size_t i = 0; i < bins.size(); ++i) {
        if (i != 0) {
            os << ",";
        }
        os << "{\"centre_spm\":" << bins[i].centre
           << ",\"total_distance_m\":" << bins[i].distance
           << ",\"total_steps\":" << bins[i].steps
           << ",\"sample_count\":" << bins[i].count;
        if (withUnknownKeys) {
            os << ",\"future_bin_key\":7";
        }
        os << "}";
    }
    os << "]}";
    return os.str();
}

} // namespace

// --- Bin layout --------------------------------------------------------------

TEST(OutdoorStrideCalibrator, BinIndexFormula)
{
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(80.0f), 0u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(83.9f), 0u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(84.0f), 1u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(160.0f), 20u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(219.9f), 34u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(220.0f), 34u);
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(60.0f), 0u);  // clamp low
    EXPECT_EQ(OutdoorStrideCalibrator::binIndexForCadence(300.0f), 34u); // clamp high
}

TEST(OutdoorStrideCalibrator, BinCentres)
{
    EXPECT_FLOAT_EQ(OutdoorStrideCalibrator::binCentreSpm(0), 82.0f);
    EXPECT_FLOAT_EQ(OutdoorStrideCalibrator::binCentreSpm(20), 162.0f);
    EXPECT_FLOAT_EQ(OutdoorStrideCalibrator::binCentreSpm(34), 218.0f);
}

TEST(OutdoorStrideCalibrator, BinValidityAndStrideFormula)
{
    StrideBin under;
    under.total_steps = Cfg::kBinValidMinSteps - 1.0f;
    EXPECT_FALSE(under.isValid());

    StrideBin at;
    at.total_steps = Cfg::kBinValidMinSteps;
    EXPECT_TRUE(at.isValid());

    StrideBin b;
    b.total_distance_m = 100.0f;
    b.total_steps      = 80.0f;  // 40 strides
    EXPECT_FLOAT_EQ(b.strideLengthM(), 2.5f);  // 100 / (80/2)
}

// --- State machine -----------------------------------------------------------

TEST(OutdoorStrideCalibrator, QualifiedBlockAcceptsAtFifteenAndContinues)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);

    feed(c, goodSample(), 14);
    EXPECT_EQ(c.acceptedThisSession(), 0u);

    c.ingestSample(goodSample());  // 15th
    EXPECT_EQ(c.acceptedThisSession(), 1u);

    c.ingestSample(goodSample());  // 16th — keeps accepting
    EXPECT_EQ(c.acceptedThisSession(), 2u);
}

TEST(OutdoorStrideCalibrator, GpsValidityGateResetsCounter)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 5);
    ASSERT_FLOAT_EQ(c.steadySeconds(), 5.0f);

    CalibratorSample bad = goodSample();
    bad.gps_speed_valid = false;
    c.ingestSample(bad);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
}

TEST(OutdoorStrideCalibrator, FixModeGateRejectsDeadReckoning)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 5);

    CalibratorSample dr = goodSample();
    dr.gps_fix_dead_reckoning = true;
    c.ingestSample(dr);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);

    // Autonomous fix resumes qualifying.
    c.ingestSample(goodSample());
    EXPECT_FLOAT_EQ(c.steadySeconds(), 1.0f);
}

TEST(OutdoorStrideCalibrator, EachBoundGateResetsCounter)
{
    SDK::Test::FakeFileSystem fs;

    const std::vector<CalibratorSample> failing = {
        goodSample(Cfg::kGpsSpeedMinMs - 0.1f),       // speed too low
        goodSample(Cfg::kGpsSpeedMaxMs + 0.1f),       // speed too high
        goodSample(3.0f, Cfg::kCadenceMinSpm - 1.0f), // cadence too low
        goodSample(3.0f, Cfg::kCadenceMaxSpm + 1.0f), // cadence too high
        goodSample(3.0f, 160.0f, Cfg::kGradeMaxPct + 0.5f),  // grade too steep
        goodSample(3.0f, 160.0f, -(Cfg::kGradeMaxPct + 0.5f)), // grade too steep (down)
    };

    for (const auto &bad : failing) {
        OutdoorStrideCalibrator c(fs, kPath);
        feed(c, goodSample(), 5);
        ASSERT_FLOAT_EQ(c.steadySeconds(), 5.0f);
        c.ingestSample(bad);
        EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
    }
}

TEST(OutdoorStrideCalibrator, GradeValidityGateResets)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 5);

    CalibratorSample s = goodSample();
    s.grade_valid = false;
    c.ingestSample(s);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
}

TEST(OutdoorStrideCalibrator, SteadyStateBandResetsOnSpeedJump)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(3.0f), 5);

    // > 5% deviation from the latched 3.0 m/s reference.
    c.ingestSample(goodSample(3.0f * 1.10f));
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
}

TEST(OutdoorStrideCalibrator, SteadyStateBandAcceptsWithinBand)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(3.0f), 5);

    // 3% deviation — within the 5% band, counter keeps advancing.
    c.ingestSample(goodSample(3.0f * 1.03f));
    EXPECT_FLOAT_EQ(c.steadySeconds(), 6.0f);
}

TEST(OutdoorStrideCalibrator, PauseResetsResumeRestartsFromZero)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 16);
    ASSERT_GE(c.acceptedThisSession(), 1u);

    c.pause();
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);

    // Ingestion is ignored while paused.
    feed(c, goodSample(), 20);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);

    c.resume();
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
    feed(c, goodSample(), 14);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 14.0f);  // still re-qualifying
}

TEST(OutdoorStrideCalibrator, StreamGapResetsCounter)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 10);
    ASSERT_FLOAT_EQ(c.steadySeconds(), 10.0f);

    // A delta_t jump beyond MAX_TICK_GAP_S breaks the run; the gap tick is
    // excluded from the window (its delta_t spans the gap), so the counter is 0.
    CalibratorSample gap = goodSample();
    gap.delta_t_s = Cfg::kMaxTickGapS + 0.5f;
    c.ingestSample(gap);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
    EXPECT_EQ(c.acceptedThisSession(), 0u);

    // The next contiguous tick starts the fresh window from its own delta_t.
    c.ingestSample(goodSample());
    EXPECT_FLOAT_EQ(c.steadySeconds(), 1.0f);
}

TEST(OutdoorStrideCalibrator, StreamGapDoesNotInstantAcceptOrIntegrateDistance)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);

    // A single tick with a huge delta_t (e.g. the service resumed after a long
    // suspension): exceeds both the gap threshold and the 15 s window. It must
    // not instant-accept nor inject speed*gap_dt of bogus distance (§4.4).
    CalibratorSample s = goodSample(3.0f, 160.0f);
    s.delta_t_s = 30.0f;
    c.ingestSample(s);

    EXPECT_EQ(c.acceptedThisSession(), 0u);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
    EXPECT_FALSE(c.bin(20).hasData());
}

TEST(OutdoorStrideCalibrator, NonPositiveDeltaTBreaksTheRun)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 10);
    ASSERT_FLOAT_EQ(c.steadySeconds(), 10.0f);

    // A zero or backward delta_t (repeated/adjusted clock) must break the run,
    // never shrinking the counter or subtracting bin distance/steps.
    CalibratorSample zero = goodSample();
    zero.delta_t_s = 0.0f;
    c.ingestSample(zero);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);

    feed(c, goodSample(), 5);
    ASSERT_FLOAT_EQ(c.steadySeconds(), 5.0f);
    CalibratorSample back = goodSample();
    back.delta_t_s = -2.0f;
    c.ingestSample(back);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
    EXPECT_EQ(c.acceptedThisSession(), 0u);
}

TEST(OutdoorStrideCalibrator, GpsLossRecovery)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(), 16);
    const size_t acceptedBefore = c.acceptedThisSession();
    ASSERT_GE(acceptedBefore, 1u);

    CalibratorSample lost = goodSample();
    lost.gps_speed_valid = false;
    c.ingestSample(lost);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);

    // Re-acquire + 15 steady ticks → accepting again.
    feed(c, goodSample(), 15);
    EXPECT_EQ(c.acceptedThisSession(), acceptedBefore + 1u);
}

// --- Plausibility (post-gate, no reset) -------------------------------------

TEST(OutdoorStrideCalibrator, PlausibilityRejectsWithoutResettingCounter)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);

    // speed 8, cadence 160 → SL_implied = 6.0 m (> STRIDE_MAX_M) but all gates
    // pass (speed 8 is the upper bound).
    feed(c, goodSample(8.0f, 160.0f), 20);

    EXPECT_EQ(c.acceptedThisSession(), 0u);     // every sample rejected
    EXPECT_GE(c.steadySeconds(), Cfg::kSteadyStateMinSeconds);  // counter NOT reset
    EXPECT_FALSE(c.bin(20).hasData());
}

// --- Bin aggregation ---------------------------------------------------------

TEST(OutdoorStrideCalibrator, PerBinAccumulation)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);

    // 17 ticks → ticks 15,16,17 accepted (3 accepted samples) into bin 20.
    feed(c, goodSample(3.0f, 160.0f), 17);
    ASSERT_EQ(c.acceptedThisSession(), 3u);

    const StrideBin &b = c.bin(20);
    EXPECT_FLOAT_EQ(b.total_distance_m, 9.0f);          // 3 * (3.0 * 1.0)
    EXPECT_NEAR(b.total_steps, 8.0f, 1e-4f);            // 3 * (160/60)
    EXPECT_FLOAT_EQ(b.sample_count, 3.0f);
    EXPECT_NEAR(b.strideLengthM(), 2.25f, 1e-3f);       // 3*120/160
}

TEST(OutdoorStrideCalibrator, AgingBelowCapDoesNotScale)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    feed(c, goodSample(3.0f, 160.0f), 30);  // far below the 20 km cap

    const StrideBin &b = c.bin(20);
    EXPECT_FLOAT_EQ(b.total_distance_m, 3.0f * static_cast<float>(c.acceptedThisSession()));
    EXPECT_LT(b.total_distance_m, Cfg::kDistanceCapM);
}

TEST(OutdoorStrideCalibrator, AgingAtCapPreservesStrideLength)
{
    SDK::Test::FakeFileSystem fs;
    // Seed bin 20 (centre 162) just under the cap with SL = 2.25.
    const float dist  = Cfg::kDistanceCapM - 1.0f;  // 19999
    const float steps = dist * 2.0f / 2.25f;        // SL = dist/(steps/2) = 2.25
    fs.seedFile(kPath, makeStoreJson(1, {{162.0f, dist, steps, 5000.0f}}));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    ASSERT_NEAR(c.bin(20).strideLengthM(), 2.25f, 1e-3f);

    feed(c, goodSample(3.0f, 160.0f), 15);  // one accepted sample, d = 3.0 m
    ASSERT_EQ(c.acceptedThisSession(), 1u);

    const StrideBin &b = c.bin(20);
    EXPECT_NEAR(b.total_distance_m, Cfg::kDistanceCapM, 0.5f);  // pinned at cap
    EXPECT_NEAR(b.strideLengthM(), 2.25f, 1e-3f);              // SL preserved
}

TEST(OutdoorStrideCalibrator, AgingLongSeriesConvergesToNewStrideLength)
{
    SDK::Test::FakeFileSystem fs;
    const float dist  = Cfg::kDistanceCapM - 1.0f;
    const float steps = dist * 2.0f / 2.25f;  // old SL = 2.25
    fs.seedFile(kPath, makeStoreJson(1, {{162.0f, dist, steps, 5000.0f}}));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    // New steady run at SL = 3.3 * 120 / 160 = 2.475. Enough distance at the cap
    // (~60 km >> 20 km cap) washes out the old data (exponential decay).
    feed(c, goodSample(3.3f, 160.0f), 20000);

    const StrideBin &b = c.bin(20);
    EXPECT_NEAR(b.total_distance_m, Cfg::kDistanceCapM, 1.0f);
    EXPECT_NEAR(b.strideLengthM(), 2.475f, 0.05f);
}

// --- Phase-2 gate ------------------------------------------------------------

TEST(OutdoorStrideCalibrator, Phase2GateReady)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 8; ++i) {
        bins.push_back({82.0f + 4.0f * i, 700.0f, 300.0f, 100.0f});  // valid bins
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_EQ(c.validBinCount(), 8u);
    EXPECT_NEAR(c.totalCalibrationDistanceM(), 5600.0f, 1.0f);
    EXPECT_TRUE(c.readyForPhase2());
}

TEST(OutdoorStrideCalibrator, Phase2GateReadyAtLowDistance)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 8; ++i) {
        bins.push_back({82.0f + 4.0f * i, 500.0f, 300.0f, 100.0f});  // total 4000
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    // 8 valid bins is the sole gate; no distance floor, so 4000 m is ready.
    EXPECT_EQ(c.validBinCount(), 8u);
    EXPECT_TRUE(c.readyForPhase2());
}

TEST(OutdoorStrideCalibrator, Phase2GateNotReadyOnBinCount)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 7; ++i) {
        bins.push_back({82.0f + 4.0f * i, 1000.0f, 300.0f, 100.0f});  // 7000 total
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    EXPECT_EQ(c.validBinCount(), 7u);
    EXPECT_FALSE(c.readyForPhase2());
}

// --- Persistence -------------------------------------------------------------

TEST(OutdoorStrideCalibrator, FinaliseSkipsWriteWhenNothingAccepted)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, "PRIOR_CONTENT");

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    feed(c, goodSample(), 5);  // never reaches 15 → nothing accepted

    EXPECT_FALSE(c.finalise());
    EXPECT_EQ(fs.fileContents(kPath), "PRIOR_CONTENT");  // untouched
}

TEST(OutdoorStrideCalibrator, FinaliseWritesAndRoundTrips)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    feed(c, goodSample(3.0f, 160.0f), 20);
    ASSERT_GE(c.acceptedThisSession(), 1u);

    EXPECT_TRUE(c.finalise());
    ASSERT_TRUE(fs.hasFile(kPath));

    OutdoorStrideCalibrator reloaded(fs, kPath);
    reloaded.load();
    for (size_t i = 0; i < OutdoorStrideCalibrator::binCount(); ++i) {
        EXPECT_NEAR(reloaded.bin(i).total_distance_m, c.bin(i).total_distance_m, 0.05f);
        EXPECT_NEAR(reloaded.bin(i).total_steps, c.bin(i).total_steps, 0.05f);
        EXPECT_NEAR(reloaded.bin(i).sample_count, c.bin(i).sample_count, 0.05f);
    }
}

TEST(OutdoorStrideCalibrator, FinaliseCreatesSharedDataDirectory)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    feed(c, goodSample(), 16);

    EXPECT_TRUE(c.finalise());
    EXPECT_TRUE(fs.dirCreated("../SharedData"));

    // Second finalise: directory already exists → still succeeds.
    feed(c, goodSample(), 1);
    EXPECT_TRUE(c.finalise());
}

TEST(OutdoorStrideCalibrator, CorruptFileBackedUpAndStartsFresh)
{
    SDK::Test::FakeFileSystem fs;
    const std::string corrupt = "{ this is not valid json";
    fs.seedFile(kPath, corrupt);

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_TRUE(fs.hasFile(kBakPath));
    EXPECT_EQ(fs.fileContents(kBakPath), corrupt);
    for (size_t i = 0; i < OutdoorStrideCalibrator::binCount(); ++i) {
        EXPECT_FALSE(c.bin(i).hasData());
    }
}

TEST(OutdoorStrideCalibrator, OversizedStoreBackedUpAndStartsFresh)
{
    SDK::Test::FakeFileSystem fs;
    const std::string big(OutdoorStrideCalibrator::kMaxStoreBytes + 1, 'x');
    fs.seedFile(kPath, big);

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_TRUE(fs.hasFile(kBakPath));  // suspicious oversized file preserved
    for (size_t i = 0; i < OutdoorStrideCalibrator::binCount(); ++i) {
        EXPECT_FALSE(c.bin(i).hasData());
    }
}

TEST(OutdoorStrideCalibrator, MissingFileStartsFreshNoBackup)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_FALSE(fs.hasFile(kBakPath));
    for (size_t i = 0; i < OutdoorStrideCalibrator::binCount(); ++i) {
        EXPECT_FALSE(c.bin(i).hasData());
    }
}

TEST(OutdoorStrideCalibrator, OlderVersionTreatedAsMissingNoBackup)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(0, {{162.0f, 1000.0f, 300.0f, 100.0f}}));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_FALSE(fs.hasFile(kBakPath));  // valid JSON, just old → no backup
    EXPECT_FALSE(c.bin(20).hasData());   // data ignored, fresh start
}

TEST(OutdoorStrideCalibrator, NewerVersionLoadsKnownFieldsIgnoringUnknown)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(99, {{162.0f, 1234.5f, 300.0f, 100.0f}},
                                     /*withUnknownKeys=*/true));

    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_FALSE(fs.hasFile(kBakPath));
    EXPECT_NEAR(c.bin(20).total_distance_m, 1234.5f, 0.05f);
    EXPECT_TRUE(c.bin(20).isValid());
}

TEST(OutdoorStrideCalibrator, RejectsImplausiblePersistedBinValues)
{
    SDK::Test::FakeFileSystem fs;
    // Bin 20 (centre 162) has a negative distance (corrupt); bin 21 (166) valid.
    fs.seedFile(kPath, makeStoreJson(1, {{162.0f, -5.0f, 300.0f, 10.0f},
                                         {166.0f, 700.0f, 300.0f, 10.0f}}));
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_FALSE(c.bin(20).hasData());  // corrupt bin rejected -> empty
    EXPECT_TRUE(c.bin(21).isValid());   // valid bin loaded normally
}

TEST(OutdoorStrideCalibrator, RejectsOffGridCentre)
{
    SDK::Test::FakeFileSystem fs;
    // 163 is off-grid (valid centres are 82, 86, ... 162, 166, ...). It rounds
    // toward bin 20 (centre 162) but must NOT be aliased into it; 166 is exact.
    fs.seedFile(kPath, makeStoreJson(1, {{163.0f, 500.0f, 300.0f, 10.0f},
                                         {166.0f, 700.0f, 300.0f, 10.0f}}));
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();

    EXPECT_FALSE(c.bin(20).hasData());  // off-grid centre not aliased into a bin
    EXPECT_TRUE(c.bin(21).isValid());   // exact on-grid centre loads normally
}

TEST(OutdoorStrideCalibrator, LoadResetsSessionStateAcrossRuns)
{
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    feed(c, goodSample(), 16);
    ASSERT_GE(c.acceptedThisSession(), 1u);

    // The instance is reused across runs; load() must reset per-session state
    // so the §6.4 write gate is evaluated per session, not cumulatively.
    c.load();
    EXPECT_EQ(c.acceptedThisSession(), 0u);
    EXPECT_FLOAT_EQ(c.steadySeconds(), 0.0f);
}

// --- Debug trace (§9.2) ------------------------------------------------------

namespace
{

// Count non-empty lines — robust whether or not the final row ends in '\n'.
size_t countNonEmptyLines(const std::string &s)
{
    size_t n = 0;
    size_t lineLen = 0;
    for (char ch : s) {
        if (ch == '\n') {
            if (lineLen > 0) {
                ++n;
            }
            lineLen = 0;
        } else {
            ++lineLen;
        }
    }
    if (lineLen > 0) {
        ++n;
    }
    return n;
}

size_t countOccurrences(const std::string &hay, const std::string &needle)
{
    size_t n = 0;
    for (size_t pos = hay.find(needle); pos != std::string::npos;
         pos = hay.find(needle, pos + needle.size())) {
        ++n;
    }
    return n;
}

} // namespace

TEST(OutdoorStrideCalibrator, DebugTraceRecordsEveryTickWithVerdicts)
{
    constexpr const char *kTrace = "../SharedData/stride_trace.csv";
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    c.enableTrace(kTrace);

    // 1 gate-rejected tick, then 16 good ticks (15th = accept, 16th continues),
    // then a dead-reckoning tick (gate reject).
    CalibratorSample dr = goodSample();
    dr.gps_fix_dead_reckoning = true;
    c.ingestSample(dr);            // discard_gate
    feed(c, goodSample(), 16);     // 14 qualifying, then 2 accepts
    c.ingestSample(dr);            // discard_gate (resets)

    ASSERT_GE(c.acceptedThisSession(), 2u);
    ASSERT_TRUE(fs.hasFile(kTrace));

    const std::string csv = fs.fileContents(kTrace);
    // Header + one row per ingestSample (1 + 16 + 1 = 18 ticks).
    EXPECT_EQ(countNonEmptyLines(csv), 1u + 18u);
    EXPECT_NE(csv.find("row,gps_speed_ms"), std::string::npos);   // header
    EXPECT_GE(countOccurrences(csv, "discard_gate"), 2u);
    EXPECT_GE(countOccurrences(csv, "qualifying"), 14u);
    EXPECT_GE(countOccurrences(csv, "accept"), 2u);
}

TEST(OutdoorStrideCalibrator, DebugTraceDisabledByDefaultWritesNothing)
{
    constexpr const char *kTrace = "../SharedData/stride_trace.csv";
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    feed(c, goodSample(), 16);
    EXPECT_FALSE(fs.hasFile(kTrace));  // no trace file when not enabled
}

TEST(OutdoorStrideCalibrator, DebugTraceClosedOnFinalise)
{
    constexpr const char *kTrace = "../SharedData/stride_trace.csv";
    SDK::Test::FakeFileSystem fs;
    OutdoorStrideCalibrator c(fs, kPath);
    c.load();
    c.enableTrace(kTrace);
    feed(c, goodSample(), 16);
    c.finalise();
    // "accept" rows were flushed and the file persists after the session ends.
    EXPECT_TRUE(fs.hasFile(kTrace));
    EXPECT_NE(fs.fileContents(kTrace).find("accept"), std::string::npos);
}
