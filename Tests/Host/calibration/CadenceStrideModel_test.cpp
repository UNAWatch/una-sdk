#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "FakeFileSystem.hpp"

#include "SDK/Calibration/CadenceStrideModel.hpp"
#include "SDK/Calibration/CadenceStrideModelConfig.hpp"
#include "SDK/Calibration/StrideLut.hpp"

using SDK::Calibration::CadenceStrideModel;
using SDK::Calibration::Phase;
using SDK::Calibration::StrideLut;
namespace Cfg = SDK::Calibration::Config;

namespace
{

constexpr const char *kOutPath   = "../SharedData/stride.json";
constexpr const char *kDeltaPath  = "treadmill_delta.json";
constexpr const char *kDeltaBak   = "treadmill_delta.json.bak";

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

std::string makeDeltaJson(int version, const std::vector<float> &deltas,
                          bool withUnknown = false)
{
    std::ostringstream os;
    os << "{\"version\":" << version;
    if (withUnknown) {
        os << ",\"future_key\":42";
    }
    os << ",\"deltas_m\":[";
    for (size_t i = 0; i < deltas.size(); ++i) {
        if (i != 0) {
            os << ",";
        }
        os << deltas[i];
    }
    os << "]}";
    return os.str();
}

// Build an outdoor LUT seed with `nValid` valid bins (centres 82, 86, ...),
// each given `distEach` metres and a fixed valid step count. SL of each bin is
// distEach / (steps/2).
std::vector<SeedBin> phase2Bins(int nValid, float distEach, float steps = 1000.0f)
{
    std::vector<SeedBin> bins;
    for (int i = 0; i < nValid; ++i) {
        bins.push_back({82.0f + 4.0f * static_cast<float>(i), distEach, steps, 50.0f});
    }
    return bins;
}

// Expected Phase-1 demographic SL(c): mirrors CadenceStrideModel — a line through
// the two cadence anchors at the reference height, scaled by height, clamped.
float demoSL(float cadenceSpm, float heightM)
{
    const float slope = (Cfg::kDemoStrideHiM - Cfg::kDemoStrideLoM)
                      / (Cfg::kDemoCadenceHiSpm - Cfg::kDemoCadenceLoSpm);
    const float slRef = Cfg::kDemoStrideLoM + slope * (cadenceSpm - Cfg::kDemoCadenceLoSpm);
    float sl = (heightM / Cfg::kDemoRefHeightM) * slRef;
    if (sl < Cfg::kStrideMinM) sl = Cfg::kStrideMinM;
    if (sl > Cfg::kStrideMaxM) sl = Cfg::kStrideMaxM;
    return sl;
}

} // namespace

// --- Phase gate --------------------------------------------------------------

TEST(CadenceStrideModel, PhaseUncalibratedWhenNoLut)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_EQ(m.phase(), Phase::UNCALIBRATED);
    EXPECT_FALSE(m.outdoorLutReady());
}

TEST(CadenceStrideModel, PhaseCalibratedAtFullBinCount)
{
    SDK::Test::FakeFileSystem fs;
    // 8 valid bins → full tier (bin count is the sole condition).
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 625.0f)));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);
    EXPECT_TRUE(m.outdoorLutReady());
}

TEST(CadenceStrideModel, PhaseEstimateBelowFullBinCount)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(7, 1000.0f)));  // 7000 m, 7 bins
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    // >=1 valid bin but <8 → outdoor estimate (delta frozen), not the full tier.
    EXPECT_EQ(m.phase(), Phase::OUTDOOR_ESTIMATE);
    EXPECT_TRUE(m.usingOutdoorStride());
    EXPECT_FALSE(m.deltaLearningActive());
    EXPECT_FALSE(m.outdoorLutReady());
}

TEST(CadenceStrideModel, PhaseCalibratedAtFullBinsRegardlessOfDistance)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 500.0f)));  // 4000 m, 8 bins
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    // 8 valid bins is the sole full-tier condition (no distance floor): even at
    // 4000 m total this is OUTDOOR_CALIBRATED with the delta LUT learning.
    EXPECT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);
    EXPECT_TRUE(m.outdoorLutReady());
    EXPECT_TRUE(m.deltaLearningActive());
}

TEST(CadenceStrideModel, PhaseEstimateAtOneValidBin)
{
    SDK::Test::FakeFileSystem fs;
    // A single valid bin (centre 82, SL = 750/(1000/2) = 1.5 m).
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(1, 750.0f)));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_ESTIMATE);
    EXPECT_TRUE(m.usingOutdoorStride());
    EXPECT_FALSE(m.deltaLearningActive());

    // The personalised outdoor stride is used: exact at the bin centre, and
    // shifted from the pure demographic curve elsewhere (measured 1.5 m at the
    // centre is above the demographic value there → a positive shift).
    EXPECT_NEAR(m.treadmillStrideLengthM(82.0f), 1.5f, 1e-4f);
    EXPECT_GT(m.treadmillStrideLengthM(160.0f), m.demographicStrideLengthM(160.0f));
    // One bin no longer collapses to a flat stride: it rides the demographic
    // cadence slope, so it stays cadence-dependent (detail in
    // OutdoorEstimateSingleBinRidesDemographicSlope).
    EXPECT_GT(m.treadmillStrideLengthM(200.0f), m.treadmillStrideLengthM(100.0f));
}

TEST(CadenceStrideModel, PhaseUncalibratedWhenNoValidBin)
{
    SDK::Test::FakeFileSystem fs;
    // A bin exists but is below the 200-step validity floor → no valid bins.
    fs.seedFile(kOutPath, makeStoreJson(1, {{82.0f, 50.0f, 100.0f, 5.0f}}));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_EQ(m.phase(), Phase::UNCALIBRATED);
    EXPECT_FALSE(m.usingOutdoorStride());
    EXPECT_NEAR(m.treadmillStrideLengthM(160.0f), m.demographicStrideLengthM(160.0f),
                1e-5f);
}

TEST(CadenceStrideModel, PhaseFrozenAfterStartSession)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::UNCALIBRATED);

    // Underlying file becomes phase-2-ready AFTER startSession; phase must not
    // change without a fresh startSession.
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 700.0f)));
    EXPECT_EQ(m.phase(), Phase::UNCALIBRATED);

    // A new session re-reads and re-freezes.
    m.startSession(1.75f);
    EXPECT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);
}

// --- Demographic SL (phase 1) ------------------------------------------------

TEST(CadenceStrideModel, DemographicStrideCadenceDependent)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);  // == reference height
    EXPECT_FLOAT_EQ(m.heightM(), 1.75f);
    // At the reference height the curve passes through the two anchors...
    EXPECT_NEAR(m.demographicStrideLengthM(Cfg::kDemoCadenceLoSpm), Cfg::kDemoStrideLoM, 1e-4f);
    EXPECT_NEAR(m.demographicStrideLengthM(Cfg::kDemoCadenceHiSpm), Cfg::kDemoStrideHiM, 1e-4f);
    // ...and stride grows with cadence (the whole point — not static).
    EXPECT_GT(m.demographicStrideLengthM(180.0f), m.demographicStrideLengthM(110.0f));
}

TEST(CadenceStrideModel, HeightFallbackOnImplausible)
{
    SDK::Test::FakeFileSystem fs;
    const float def = Cfg::kDefaultHeightM;
    for (float h : {0.0f, 1.0f, 2.5f, std::nanf(""),
                    std::numeric_limits<float>::infinity()}) {
        CadenceStrideModel m(fs, kOutPath, kDeltaPath);
        m.startSession(h);
        EXPECT_FLOAT_EQ(m.heightM(), def);
        EXPECT_NEAR(m.demographicStrideLengthM(160.0f), demoSL(160.0f, def), 1e-5f);
    }
}

TEST(CadenceStrideModel, Phase1StrideCadenceDependentIgnoresDelta)
{
    SDK::Test::FakeFileSystem fs;
    // Seed a delta file; phase 1 must NOT apply it.
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[10] = 0.5f;
    fs.seedFile(kDeltaPath, makeDeltaJson(1, deltas));

    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.80f);
    ASSERT_EQ(m.phase(), Phase::UNCALIBRATED);

    // Phase 1 uses the cadence-dependent demographic SL and ignores Δ.
    EXPECT_NEAR(m.treadmillStrideLengthM(80.0f),  demoSL(80.0f, 1.80f),  1e-5f);
    EXPECT_NEAR(m.treadmillStrideLengthM(122.0f), demoSL(122.0f, 1.80f), 1e-5f);  // delta bin ignored
    EXPECT_NEAR(m.treadmillStrideLengthM(200.0f), demoSL(200.0f, 1.80f), 1e-5f);
    // Not static: longer stride at higher cadence.
    EXPECT_GT(m.treadmillStrideLengthM(200.0f), m.treadmillStrideLengthM(80.0f));
}

TEST(CadenceStrideModel, DemographicStrideClampedToBounds)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(2.10f);  // max plausible height
    const float sl = m.demographicStrideLengthM(200.0f);
    EXPECT_GE(sl, Cfg::kStrideMinM);
    EXPECT_LE(sl, Cfg::kStrideMaxM);
}

// --- Outdoor SL(c) interpolation & shelves -----------------------------------

TEST(CadenceStrideModel, OutdoorStrideInterpolationAndShelves)
{
    SDK::Test::FakeFileSystem fs;
    // Valid bin 4 (centre 98) SL=1.0; valid bin 9 (centre 118) SL=1.5; an
    // invalid bin 6 (centre 106, <200 steps) sits between them and is skipped.
    fs.seedFile(kOutPath, makeStoreJson(1, {
        {98.0f, 200.0f, 400.0f, 50.0f},   // SL = 200/200 = 1.0
        {106.0f, 50.0f, 100.0f, 50.0f},   // invalid (100 steps)
        {118.0f, 300.0f, 400.0f, 50.0f},  // SL = 300/200 = 1.5
    }));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);

    EXPECT_NEAR(m.outdoorStrideLengthM(98.0f), 1.0f, 1e-4f);   // exact centre
    EXPECT_NEAR(m.outdoorStrideLengthM(118.0f), 1.5f, 1e-4f);  // exact centre
    EXPECT_NEAR(m.outdoorStrideLengthM(108.0f), 1.25f, 1e-4f); // midpoint interp (interior unchanged)

    // Outside the valid span the estimate is no longer a flat shelf: it rides the
    // demographic cadence slope, shifted to pass through the nearest valid bin.
    EXPECT_NEAR(m.outdoorStrideLengthM(82.0f),
                m.demographicStrideLengthM(82.0f)
                    + (1.0f - m.demographicStrideLengthM(98.0f)),
                1e-4f);
    EXPECT_NEAR(m.outdoorStrideLengthM(218.0f),
                m.demographicStrideLengthM(218.0f)
                    + (1.5f - m.demographicStrideLengthM(118.0f)),
                1e-4f);
    // Cadence-dependent (lower cadence below the lowest bin → shorter stride),
    // not the old flat 1.0.
    EXPECT_LT(m.outdoorStrideLengthM(82.0f), 1.0f);
}

// A single valid bin must NOT collapse to a flat (cadence-independent) stride:
// that would be a downgrade from the cadence-dependent demographic tier. The
// estimate tier rides the demographic slope shifted through the one measured bin.
TEST(CadenceStrideModel, OutdoorEstimateSingleBinRidesDemographicSlope)
{
    SDK::Test::FakeFileSystem fs;
    // One valid bin at centre 130 (>= 200 steps), SL = 300 / (400/2) = 1.5.
    fs.seedFile(kOutPath, makeStoreJson(1, {{130.0f, 300.0f, 400.0f, 50.0f}}));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_ESTIMATE);

    // Exactly the measured stride at the bin centre.
    EXPECT_NEAR(m.outdoorStrideLengthM(130.0f), 1.5f, 1e-4f);

    // Cadence-dependent away from the bin: lower → shorter, higher → longer.
    const float slLo = m.outdoorStrideLengthM(100.0f);
    const float slHi = m.outdoorStrideLengthM(160.0f);
    EXPECT_LT(slLo, 1.5f);
    EXPECT_GT(slHi, 1.5f);

    // It is the demographic curve shifted to pass through the bin (parallel shift).
    const float shift = 1.5f - m.demographicStrideLengthM(130.0f);
    EXPECT_NEAR(slLo, m.demographicStrideLengthM(100.0f) + shift, 1e-4f);
    EXPECT_NEAR(slHi, m.demographicStrideLengthM(160.0f) + shift, 1e-4f);
    const float dDemo =
        m.demographicStrideLengthM(160.0f) - m.demographicStrideLengthM(100.0f);
    EXPECT_NEAR(slHi - slLo, dDemo, 1e-4f);
}

TEST(CadenceStrideModel, OutdoorStrideFallsBackToDemographicWhenNoValidBin)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);  // empty LUT
    EXPECT_NEAR(m.outdoorStrideLengthM(160.0f), m.demographicStrideLengthM(160.0f), 1e-5f);
}

// --- Delta Δ(c) dense interpolation ------------------------------------------

TEST(CadenceStrideModel, Phase2StrideIsPureSlWhenDeltaZero)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));  // SL = 1.3
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);

    // No delta file → Δ == 0 everywhere → stride == SL(c).
    EXPECT_NEAR(m.deltaAt(100.0f), 0.0f, 1e-6f);
    EXPECT_NEAR(m.treadmillStrideLengthM(100.0f), m.outdoorStrideLengthM(100.0f),
                1e-5f);
}

TEST(CadenceStrideModel, DeltaDenseInterpolationWithShelves)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[10] = 0.2f;  // centre 122
    fs.seedFile(kDeltaPath, makeDeltaJson(1, deltas));

    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);

    EXPECT_NEAR(m.deltaAt(122.0f), 0.2f, 1e-5f);   // exact bin 10
    EXPECT_NEAR(m.deltaAt(118.0f), 0.0f, 1e-5f);   // bin 9
    EXPECT_NEAR(m.deltaAt(120.0f), 0.1f, 1e-5f);   // midway 9↔10
    EXPECT_NEAR(m.deltaAt(124.0f), 0.1f, 1e-5f);   // midway 10↔11
    EXPECT_NEAR(m.deltaAt(82.0f), 0.0f, 1e-5f);    // bin 0
    EXPECT_NEAR(m.deltaAt(70.0f), 0.0f, 1e-5f);    // shelf below bin 0
    EXPECT_NEAR(m.deltaAt(260.0f), 0.0f, 1e-5f);   // shelf above bin 34
}

// --- Post-run, phase 1 -------------------------------------------------------

TEST(CadenceStrideModel, PostRunPhase1NeverWritesDelta)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::UNCALIBRATED);

    float steps[StrideLut::kBinCount] {};
    steps[20] = 400.0f;
    // D_actual within sane bounds → accepted, but phase 1 must not persist.
    auto r = m.applyPostRunCalibration(steps, /*D_estimated=*/500.0f,
                                       /*D_actual=*/520.0f);
    EXPECT_TRUE(r.dActualAccepted);
    EXPECT_FALSE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, 520.0f);
    EXPECT_FALSE(fs.hasFile(kDeltaPath));
}

TEST(CadenceStrideModel, PostRunPhase1RejectedReturnsEstimate)
{
    SDK::Test::FakeFileSystem fs;
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);

    float steps[StrideLut::kBinCount] {};
    steps[20] = 400.0f;
    // D_actual = 5x D_estimated → ratio gate rejects.
    auto r = m.applyPostRunCalibration(steps, 500.0f, 2500.0f);
    EXPECT_FALSE(r.dActualAccepted);
    EXPECT_FALSE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, 500.0f);
    EXPECT_FALSE(fs.hasFile(kDeltaPath));
}

TEST(CadenceStrideModel, PostRunEstimateTierDoesNotLearnDelta)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(1, 750.0f)));  // estimate tier
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_ESTIMATE);

    float steps[StrideLut::kBinCount] {};
    steps[0] = 400.0f;
    // Accepted D_actual still corrects the recorded distance, but the delta LUT
    // must NOT learn or persist while in the estimate tier.
    auto r = m.applyPostRunCalibration(steps, /*D_estimated=*/500.0f,
                                       /*D_actual=*/520.0f);
    EXPECT_TRUE(r.dActualAccepted);
    EXPECT_FALSE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, 520.0f);
    EXPECT_FALSE(fs.hasFile(kDeltaPath));
}

// --- Post-run, phase 2 happy path --------------------------------------------

TEST(CadenceStrideModel, PostRunPhase2WeightedUpdateIdentityAndPersist)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));  // SL = 1.3
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);

    // Used bins 0 (centre 82) and 7 (centre 110). Distances are >= the delta
    // learning floor (kDeltaLearnMinDistanceM = 2000 m) so the LUT actually learns.
    float steps[StrideLut::kBinCount] {};
    steps[0] = 600.0f;
    steps[7] = 1000.0f;

    const float dEst = 2000.0f;
    const float dAct = 2200.0f;  // ratio 1.1, implied mean stride 2.75 → accepted
    auto r = m.applyPostRunCalibration(steps, dEst, dAct);

    EXPECT_TRUE(r.dActualAccepted);
    EXPECT_TRUE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, dAct);

    // Per-bin deltas (deltaAt at exact centre == mDelta[i]).
    const float d0 = m.deltaAt(82.0f);
    const float d7 = m.deltaAt(110.0f);
    EXPECT_GT(d0, 0.0f);
    EXPECT_GT(d7, 0.0f);
    EXPECT_GT(d7, d0);  // bin 7 has more steps → larger correction

    // Verification identity (pre-clamp; here no clamp triggers):
    //   Σ (S_i / 2) · Δδ_i = η · ΔD
    const float lhs = (steps[0] / 2.0f) * d0 + (steps[7] / 2.0f) * d7;
    const float rhs = Cfg::kLearningRateEta * (dAct - dEst);
    EXPECT_NEAR(lhs, rhs, 1e-2f);

    // Only used bins changed.
    EXPECT_NEAR(m.deltaAt(86.0f), 0.0f, 1e-6f);   // bin 1, unused
    EXPECT_NEAR(m.deltaAt(106.0f), 0.0f, 1e-6f);  // bin 6, unused

    // Persisted and reload reproduces.
    ASSERT_TRUE(fs.hasFile(kDeltaPath));
    CadenceStrideModel reloaded(fs, kOutPath, kDeltaPath);
    reloaded.startSession(1.75f);
    EXPECT_NEAR(reloaded.deltaAt(82.0f), d0, 1e-5f);
    EXPECT_NEAR(reloaded.deltaAt(110.0f), d7, 1e-5f);
}

// Outdoor-calibrated tier, accepted D_actual, but the session is shorter than
// the delta-learning distance floor: the recorded distance is still corrected,
// but the delta LUT must NOT learn or persist.
TEST(CadenceStrideModel, PostRunCalibratedBelowMinDistanceCorrectsFitButNoDelta)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));  // SL = 1.3
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);

    float steps[StrideLut::kBinCount] {};
    steps[0] = 300.0f;
    steps[7] = 500.0f;

    // D_estimated below kDeltaLearnMinDistanceM (2000 m). D_actual ratio 1.1,
    // implied mean stride 2.75 → would be accepted and would normally learn.
    const float dEst = 1000.0f;
    const float dAct = 1100.0f;
    auto r = m.applyPostRunCalibration(steps, dEst, dAct);

    EXPECT_TRUE(r.dActualAccepted);
    EXPECT_FALSE(r.deltaLutUpdated);
    EXPECT_FLOAT_EQ(r.distanceForFitM, dAct);   // FIT distance still corrected
    EXPECT_NEAR(m.deltaAt(82.0f), 0.0f, 1e-6f); // LUT untouched
    EXPECT_NEAR(m.deltaAt(110.0f), 0.0f, 1e-6f);
    EXPECT_FALSE(fs.hasFile(kDeltaPath));        // nothing persisted
}

// --- Post-run gates ----------------------------------------------------------

TEST(CadenceStrideModel, PostRunPhase2GatesReject)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));

    // Helper: fresh model + a single used bin.
    auto run = [&](float dEst, float dAct, float binSteps) {
        CadenceStrideModel m(fs, kOutPath, kDeltaPath);
        m.startSession(1.75f);
        float steps[StrideLut::kBinCount] {};
        steps[0] = binSteps;
        return m.applyPostRunCalibration(steps, dEst, dAct);
    };

    // Ratio gate: D_actual far above 2x D_estimated.
    {
        auto r = run(1000.0f, 3000.0f, 400.0f);
        EXPECT_FALSE(r.dActualAccepted);
        EXPECT_FALSE(r.deltaLutUpdated);
        EXPECT_FLOAT_EQ(r.distanceForFitM, 1000.0f);
    }
    // Implied-stride gate: tiny step count makes mean stride absurd.
    {
        auto r = run(1000.0f, 1100.0f, 2.0f);  // mean stride 1100/(1)=1100 m
        EXPECT_FALSE(r.dActualAccepted);
        EXPECT_FLOAT_EQ(r.distanceForFitM, 1000.0f);
    }
    // S_total == 0.
    {
        CadenceStrideModel m(fs, kOutPath, kDeltaPath);
        m.startSession(1.75f);
        float steps[StrideLut::kBinCount] {};  // all zero
        auto r = m.applyPostRunCalibration(steps, 1000.0f, 1000.0f);
        EXPECT_FALSE(r.dActualAccepted);
        EXPECT_FLOAT_EQ(r.distanceForFitM, 1000.0f);
    }
    // D_estimated <= 0: rejected by the ratio gate; phase-2 rejection keeps the
    // (zero) estimate, never substituting D_actual.
    {
        auto r = run(0.0f, 100.0f, 400.0f);
        EXPECT_FALSE(r.dActualAccepted);
        EXPECT_FLOAT_EQ(r.distanceForFitM, 0.0f);
    }

    // No delta file written by any rejection.
    EXPECT_FALSE(fs.hasFile(kDeltaPath));
}

// --- Per-bin clamp -----------------------------------------------------------

TEST(CadenceStrideModel, PostRunPhase2PerBinClamp)
{
    SDK::Test::FakeFileSystem fs;
    // Bin 0 (centre 82) has a high SL of 4.8; the rest are normal valid bins.
    std::vector<SeedBin> bins = phase2Bins(8, 650.0f);  // SL 1.3 each
    bins[0] = {82.0f, 2400.0f, 1000.0f, 50.0f};         // SL = 2400/500 = 4.8
    fs.seedFile(kOutPath, makeStoreJson(1, bins));

    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    ASSERT_EQ(m.phase(), Phase::OUTDOOR_CALIBRATED);
    ASSERT_NEAR(m.outdoorStrideLengthM(82.0f), 4.8f, 1e-3f);

    // Single used bin 0, S = 2000. D_estimated = 2500 (>= the 2000 m delta floor),
    // D_actual = 4500 (implied mean stride 4500/1000 = 4.5 ≤ max, ratio 1.8 →
    // accepted). Raw δ = 2·η·ΔD/S = 0.8 would push SL→5.6; clamp to 5.0
    // back-solves δ = 0.2.
    float steps[StrideLut::kBinCount] {};
    steps[0] = 2000.0f;
    auto r = m.applyPostRunCalibration(steps, /*D_estimated=*/2500.0f,
                                       /*D_actual=*/4500.0f);
    ASSERT_TRUE(r.dActualAccepted);
    ASSERT_TRUE(r.deltaLutUpdated);

    EXPECT_NEAR(m.deltaAt(82.0f), 0.2f, 1e-3f);                  // clamped δ
    EXPECT_NEAR(m.treadmillStrideLengthM(82.0f), Cfg::kStrideMaxM, 1e-3f);

    // Persisted deltas reflect the clamped value.
    CadenceStrideModel reloaded(fs, kOutPath, kDeltaPath);
    reloaded.startSession(1.75f);
    EXPECT_NEAR(reloaded.deltaAt(82.0f), 0.2f, 1e-3f);
}

// --- Delta file policy -------------------------------------------------------

TEST(CadenceStrideModel, DeltaFileMissingLoadsZeros)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_NEAR(m.deltaAt(122.0f), 0.0f, 1e-6f);
    EXPECT_FALSE(fs.hasFile(kDeltaBak));
}

TEST(CadenceStrideModel, DeltaFileMalformedLoadsZerosNoBackup)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    fs.seedFile(kDeltaPath, "{ not valid json");
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_NEAR(m.deltaAt(122.0f), 0.0f, 1e-6f);
    EXPECT_FALSE(fs.hasFile(kDeltaBak));  // model never writes .bak
}

TEST(CadenceStrideModel, DeltaFileWrongLengthLoadsZeros)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> shortDeltas(StrideLut::kBinCount - 1, 0.3f);  // wrong length
    fs.seedFile(kDeltaPath, makeDeltaJson(1, shortDeltas));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_NEAR(m.deltaAt(122.0f), 0.0f, 1e-6f);
}

TEST(CadenceStrideModel, DeltaFileNonFiniteLoadsZeros)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[5] = 0.3f;
    // Inject a NaN literal into the array by hand.
    std::string json = makeDeltaJson(1, deltas);
    // Replace the first "0.3" value with NaN to simulate a corrupt entry.
    const std::string nanJson =
        "{\"version\":1,\"deltas_m\":[NaN,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"
        "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}";
    fs.seedFile(kDeltaPath, nanJson);
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    // Either the JSON is rejected as malformed or the non-finite entry zeroes
    // the whole LUT; in both cases all deltas are zero.
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        EXPECT_NEAR(m.deltaAt(StrideLut::binCentreSpm(i)), 0.0f, 1e-6f);
    }
}

TEST(CadenceStrideModel, DeltaFileOlderVersionLoadsZeros)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[10] = 0.4f;
    fs.seedFile(kDeltaPath, makeDeltaJson(0, deltas));  // version 0 < 1
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_NEAR(m.deltaAt(122.0f), 0.0f, 1e-6f);
    EXPECT_FALSE(fs.hasFile(kDeltaBak));
}

TEST(CadenceStrideModel, DeltaFileNewerVersionLoadsKnownFields)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[10] = 0.25f;
    fs.seedFile(kDeltaPath, makeDeltaJson(99, deltas, /*withUnknown=*/true));
    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    EXPECT_NEAR(m.deltaAt(122.0f), 0.25f, 1e-5f);
}

TEST(CadenceStrideModel, DeltaFileRoundTrip)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kOutPath, makeStoreJson(1, phase2Bins(8, 650.0f)));
    std::vector<float> deltas(StrideLut::kBinCount, 0.0f);
    deltas[3]  = 0.1f;
    deltas[15] = -0.05f;
    deltas[30] = 0.2f;
    fs.seedFile(kDeltaPath, makeDeltaJson(1, deltas));

    CadenceStrideModel m(fs, kOutPath, kDeltaPath);
    m.startSession(1.75f);
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        EXPECT_NEAR(m.deltaAt(StrideLut::binCentreSpm(i)), deltas[i], 1e-5f);
    }
}
