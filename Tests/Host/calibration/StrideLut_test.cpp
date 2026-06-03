#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "FakeFileSystem.hpp"

#include "SDK/Calibration/OutdoorStrideCalibConfig.hpp"
#include "SDK/Calibration/StrideLut.hpp"

using SDK::Calibration::StrideBin;
using SDK::Calibration::StrideLut;
namespace Cfg = SDK::Calibration::Config;

namespace
{

constexpr const char *kPath = "../SharedData/stride.json";
constexpr const char *kBakPath = "../SharedData/stride.json.bak";

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

// --- Bin layout (unchanged after the move) -----------------------------------

TEST(StrideLut, BinIndexFormula)
{
    EXPECT_EQ(StrideLut::binIndexForCadence(80.0f), 0u);
    EXPECT_EQ(StrideLut::binIndexForCadence(83.9f), 0u);
    EXPECT_EQ(StrideLut::binIndexForCadence(84.0f), 1u);
    EXPECT_EQ(StrideLut::binIndexForCadence(160.0f), 20u);
    EXPECT_EQ(StrideLut::binIndexForCadence(219.9f), 34u);
    EXPECT_EQ(StrideLut::binIndexForCadence(220.0f), 34u);
    EXPECT_EQ(StrideLut::binIndexForCadence(60.0f), 0u);   // clamp low
    EXPECT_EQ(StrideLut::binIndexForCadence(300.0f), 34u); // clamp high
}

TEST(StrideLut, BinCentres)
{
    EXPECT_FLOAT_EQ(StrideLut::binCentreSpm(0), 82.0f);
    EXPECT_FLOAT_EQ(StrideLut::binCentreSpm(20), 162.0f);
    EXPECT_FLOAT_EQ(StrideLut::binCentreSpm(34), 218.0f);
}

TEST(StrideLut, BinValidityAndStrideFormula)
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

// --- Read-only load ----------------------------------------------------------

TEST(StrideLut, LoadReproducesBinState)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(1, {{162.0f, 700.0f, 300.0f, 100.0f},
                                         {166.0f, 800.0f, 320.0f, 110.0f}}));
    StrideLut lut;
    EXPECT_TRUE(lut.loadFromFile(fs, kPath));

    EXPECT_NEAR(lut.bin(20).total_distance_m, 700.0f, 0.05f);
    EXPECT_NEAR(lut.bin(20).total_steps, 300.0f, 0.05f);
    EXPECT_NEAR(lut.bin(21).total_distance_m, 800.0f, 0.05f);
    EXPECT_TRUE(lut.bin(20).isValid());
    EXPECT_TRUE(lut.bin(21).isValid());
}

TEST(StrideLut, MissingFileLoadsAllZeroNoBackup)
{
    SDK::Test::FakeFileSystem fs;
    StrideLut lut;
    EXPECT_FALSE(lut.loadFromFile(fs, kPath));
    EXPECT_FALSE(fs.hasFile(kBakPath));  // read-only owner never writes .bak
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        EXPECT_FALSE(lut.bin(i).hasData());
    }
}

TEST(StrideLut, MalformedFileLoadsAllZeroNoBackup)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, "{ this is not valid json");
    StrideLut lut;
    EXPECT_FALSE(lut.loadFromFile(fs, kPath));
    EXPECT_FALSE(fs.hasFile(kBakPath));  // model does NOT back up
    for (size_t i = 0; i < StrideLut::kBinCount; ++i) {
        EXPECT_FALSE(lut.bin(i).hasData());
    }
}

TEST(StrideLut, OlderVersionLoadsEmptyButReturnsTrue)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(0, {{162.0f, 1000.0f, 300.0f, 100.0f}}));
    StrideLut lut;
    // Syntactically valid, just older → loads empty, still "a valid file".
    EXPECT_TRUE(lut.loadFromFile(fs, kPath));
    EXPECT_FALSE(lut.bin(20).hasData());
}

TEST(StrideLut, NewerVersionLoadsKnownFieldsIgnoringUnknown)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(99, {{162.0f, 1234.5f, 300.0f, 100.0f}},
                                     /*withUnknownKeys=*/true));
    StrideLut lut;
    EXPECT_TRUE(lut.loadFromFile(fs, kPath));
    EXPECT_NEAR(lut.bin(20).total_distance_m, 1234.5f, 0.05f);
    EXPECT_TRUE(lut.bin(20).isValid());
}

TEST(StrideLut, LoadClearsPriorState)
{
    SDK::Test::FakeFileSystem fs;
    fs.seedFile(kPath, makeStoreJson(1, {{162.0f, 700.0f, 300.0f, 100.0f}}));
    StrideLut lut;
    ASSERT_TRUE(lut.loadFromFile(fs, kPath));
    ASSERT_TRUE(lut.bin(20).hasData());

    // A second load from a missing file zeroes the prior state.
    EXPECT_FALSE(lut.loadFromFile(fs, "../SharedData/nonexistent.json"));
    EXPECT_FALSE(lut.bin(20).hasData());
}

// --- Phase-2 gate read path --------------------------------------------------

TEST(StrideLut, Phase2GateReady)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 8; ++i) {
        bins.push_back({82.0f + 4.0f * i, 700.0f, 300.0f, 100.0f});
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));
    StrideLut lut;
    ASSERT_TRUE(lut.loadFromFile(fs, kPath));

    EXPECT_EQ(lut.validBinCount(), 8u);
    EXPECT_NEAR(lut.totalCalibrationDistanceM(), 5600.0f, 1.0f);
    EXPECT_TRUE(lut.readyForPhase2());
}

TEST(StrideLut, Phase2GateNotReadyOnDistance)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 8; ++i) {
        bins.push_back({82.0f + 4.0f * i, 500.0f, 300.0f, 100.0f});  // 4000 m total
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));
    StrideLut lut;
    ASSERT_TRUE(lut.loadFromFile(fs, kPath));
    EXPECT_EQ(lut.validBinCount(), 8u);
    EXPECT_FALSE(lut.readyForPhase2());
}

TEST(StrideLut, Phase2GateNotReadyOnBinCount)
{
    SDK::Test::FakeFileSystem fs;
    std::vector<SeedBin> bins;
    for (int i = 0; i < 7; ++i) {
        bins.push_back({82.0f + 4.0f * i, 1000.0f, 300.0f, 100.0f});  // 7000 m total
    }
    fs.seedFile(kPath, makeStoreJson(1, bins));
    StrideLut lut;
    ASSERT_TRUE(lut.loadFromFile(fs, kPath));
    EXPECT_EQ(lut.validBinCount(), 7u);
    EXPECT_FALSE(lut.readyForPhase2());
}
