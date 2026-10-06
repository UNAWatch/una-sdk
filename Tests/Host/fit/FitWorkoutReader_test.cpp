/**
 ******************************************************************************
 * @file    FitWorkoutReader_test.cpp
 * @brief   Tests for SDK::Fit::FitWorkoutReader.
 *
 * Every file here is built in the test (see FitBytes.hpp). Their shapes
 * follow the workout files a planning service exports: big-endian records,
 * local type 0 redefined before each message, step notes and the
 * description repeated in memo_glob parts.
 ******************************************************************************
 */

#include "fit/FitBytes.hpp"

#include "FakeFileSystem.hpp"
#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitWorkoutReader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

using SDK::Fit::FitWorkoutReader;
using SDK::Fit::MemoryByteSource;
using SDK::Workout::HeartRateTarget;
using SDK::Workout::Intensity;
using SDK::Workout::Program;
using SDK::Workout::StepEnd;
using SDK::Workout::Target;
using testfit::FDef;
using testfit::FitBytes;
using testfit::FVal;
using namespace testfit;

namespace {

using Result = FitWorkoutReader::Result;

constexpr uint32_t kInv32 = 0xFFFFFFFFu;
constexpr uint8_t  kInv8  = 0xFF;

// FIT enum values used below.
constexpr uint8_t kTime = 0, kDistance = 1, kCalories = 4, kOpenDur = 5, kRepeatSteps = 6,
                  kRepeatTime = 7;
constexpr uint8_t kSpeed = 0, kHr = 1, kOpenTgt = 2, kCadence = 3;
constexpr uint8_t kActive = 0, kRest = 1, kWarmup = 2, kCooldown = 3, kRecovery = 4;

struct StepSpec {
    uint16_t    index;
    uint8_t     durType;
    uint32_t    durValue  = kInv32;
    uint8_t     tgtType   = kOpenTgt;
    uint32_t    tgtValue  = 0;
    uint32_t    low       = kInv32;
    uint32_t    high      = kInv32;
    uint8_t     intensity = kActive;
    std::string notes;
};

// Builds a workout file the way the examples are laid out.
class Wkt {
public:
    explicit Wkt(bool bigEndian = true, uint8_t headerSize = 14) : b(headerSize), be(bigEndian) {}

    Wkt& fileId(uint8_t type = 5)
    {
        b.def(0, 0, {{0, 1, kEnum}, {1, 2, kU16}, {2, 2, kU16}, {4, 4, kU32}, {3, 4, kU32z}}, be)
            .data(0, {FVal::U(type), FVal::U(1), FVal::U(65534), FVal::U(1100000000), FVal::U(42)});
        // A file_creator message the reader does not use.
        b.def(0, 49, {{1, 1, kU8}, {0, 2, kU16}}, be).data(0, {FVal::U(0), FVal::U(0)});
        return *this;
    }

    Wkt& workout(const std::string& name, uint16_t steps, const std::string& desc = "",
                 uint8_t sport = 1)
    {
        b.def(0, 26, {{8, 64, kStr}, {17, 161, kStr}, {4, 1, kEnum}, {11, 1, kEnum},
                      {5, 4, kU32z}, {6, 2, kU16}, {254, 2, kU16}}, be)
            .data(0, {FVal::T(name), FVal::T(desc), FVal::U(sport), FVal::U(0), FVal::U(32),
                      FVal::U(steps), FVal::U(0)});
        return *this;
    }

    // A workout message without num_valid_steps.
    Wkt& workoutNoCount(const std::string& name)
    {
        b.def(0, 26, {{8, 64, kStr}, {4, 1, kEnum}}, be).data(0, {FVal::T(name), FVal::U(1)});
        return *this;
    }

    Wkt& step(const StepSpec& s)
    {
        b.def(0, 27, {{254, 2, kU16}, {7, 1, kEnum}, {5, 4, kU32}, {6, 4, kU32}, {4, 4, kU32},
                      {3, 1, kEnum}, {19, 1, kEnum}, {20, 4, kU32}, {1, 1, kEnum}, {2, 4, kU32},
                      {8, 20, kStr}}, be)
            .data(0, {FVal::U(s.index), FVal::U(s.intensity), FVal::U(s.low), FVal::U(s.high),
                      FVal::U(s.tgtValue), FVal::U(s.tgtType), FVal::U(kInv8), FVal::U(kInv32),
                      FVal::U(s.durType), FVal::U(s.durValue), FVal::T(s.notes)});
        return *this;
    }

    // A repeat step laid out as the examples do (no intensity or notes).
    Wkt& repeat(uint16_t index, uint32_t from, uint32_t count, uint8_t type = kRepeatSteps)
    {
        b.def(0, 27, {{1, 1, kEnum}, {4, 4, kU32}, {254, 2, kU16}, {2, 4, kU32}, {18, 1, kEnum}}, be)
            .data(0, {FVal::U(type), FVal::U(count), FVal::U(index), FVal::U(from), FVal::U(0)});
        return *this;
    }

    Wkt& memo(uint16_t mesg, uint16_t parent, uint8_t field, uint32_t part, const std::string& text,
              uint8_t size = 0)
    {
        const uint8_t n = size ? size : static_cast<uint8_t>(text.size() + 1);
        b.def(0, 145, {{250, 4, kU32}, {1, 2, kU16}, {2, 2, kU16}, {3, 1, kU8}, {4, n, kU8z}}, be)
            .data(0, {FVal::U(part), FVal::U(mesg), FVal::U(parent), FVal::U(field), FVal::T(text)});
        return *this;
    }

    std::vector<uint8_t> bytes(bool headerCrc = true) const { return b.build(headerCrc); }

    FitBytes b;
    bool     be;
};

// Program is large: keep test copies off the stack.
struct Fixture {
    FitWorkoutReader         reader;
    std::unique_ptr<Program> prog = std::make_unique<Program>();

    Result read(const std::vector<uint8_t>& file)
    {
        MemoryByteSource src(file.data(), file.size());
        return reader.read(src, *prog);
    }
};

std::vector<uint8_t> easyRun()
{
    return Wkt().fileId().workout("Easy 6.5km", 1, "Conversational pace.")
        .step({0, kDistance, 650000, kOpenTgt, 0, kInv32, kInv32, kActive, "6.5km easy"})
        .bytes();
}

}  // namespace

// --- The shapes in the example plan ------------------------------------------

TEST(FitWorkoutReader, SingleDistanceStep)
{
    Fixture f;
    ASSERT_EQ(f.read(easyRun()), Result::Ok);
    const Program& p = *f.prog;
    EXPECT_STREQ(p.name, "Easy 6.5km");
    EXPECT_STREQ(p.description, "Conversational pace.");
    EXPECT_EQ(p.sport, 1);
    EXPECT_EQ(p.subSport, 0);
    ASSERT_EQ(p.stepCount, 1);
    EXPECT_EQ(p.steps[0].end, StepEnd::Distance);
    EXPECT_EQ(p.steps[0].distanceCm, 650000u);
    EXPECT_EQ(p.steps[0].target, Target::Open);
    EXPECT_STREQ(p.steps[0].notes, "6.5km easy");
    EXPECT_FALSE(p.degraded);
}

TEST(FitWorkoutReader, OpenWarmupSpeedRangesTimedRest)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("Tempo", 4)
        .step({0, kOpenDur, kInv32, kOpenTgt, 0, kInv32, kInv32, kWarmup, "Warm up"})
        .step({1, kDistance, 200000, kSpeed, 0, 3279, 3509, kActive, "2km at 4:55/km"})
        .step({2, kTime, 90000, kOpenTgt, 0, kInv32, kInv32, kRest, "90s rest"})
        .step({3, kDistance, 100000, kOpenTgt, 0, kInv32, kInv32, kCooldown, "Cool down"})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    const Program& p = *f.prog;
    ASSERT_EQ(p.stepCount, 4);
    EXPECT_EQ(p.steps[0].end, StepEnd::Open);
    EXPECT_EQ(p.steps[0].intensity, Intensity::Warmup);
    EXPECT_FALSE(p.steps[0].degraded);
    EXPECT_EQ(p.steps[1].target, Target::Speed);
    EXPECT_EQ(p.steps[1].speedLowMmps, 3279u);
    EXPECT_EQ(p.steps[1].speedHighMmps, 3509u);
    EXPECT_EQ(p.steps[2].end, StepEnd::Time);
    EXPECT_EQ(p.steps[2].durationMs, 90000u);
    EXPECT_EQ(p.steps[2].intensity, Intensity::Rest);
    EXPECT_EQ(p.steps[3].intensity, Intensity::Cooldown);
    EXPECT_FALSE(p.degraded);
}

TEST(FitWorkoutReader, RepeatBlock)
{
    // Steps 1-2 three times: over and under.
    Fixture f;
    const auto file = Wkt().fileId().workout("Over/under", 4)
        .step({0, kOpenDur, kInv32, kOpenTgt, 0, kInv32, kInv32, kWarmup})
        .step({1, kDistance, 100000, kSpeed, 0, 3390, 3636})
        .step({2, kDistance, 100000, kSpeed, 0, 3636, 3922})
        .repeat(3, 1, 3)
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    const auto& r = f.prog->steps[3];
    EXPECT_EQ(r.end, StepEnd::Repeat);
    EXPECT_EQ(r.repeatFrom, 1);
    EXPECT_EQ(r.repeatCount, 3u);
    EXPECT_EQ(r.target, Target::Open);
    EXPECT_FALSE(r.degraded);
}

TEST(FitWorkoutReader, NestedRepeatsSharingAFirstStep)
{
    // (400 m + 60 s) x5, then 60 s, the whole block x2. Both blocks start at
    // step 2: inner [2..4], outer [2..6].
    Fixture f;
    const auto file = Wkt().fileId().workout("400s", 8)
        .step({0, kOpenDur, kInv32, kOpenTgt, 0, kInv32, kInv32, kWarmup})
        .step({1, kTime, 90000, kOpenTgt, 0, kInv32, kInv32, kRest})
        .step({2, kDistance, 40000, kSpeed, 0, 4082, 4444})
        .step({3, kTime, 60000, kOpenTgt, 0, kInv32, kInv32, kRest})
        .repeat(4, 2, 5)
        .step({5, kTime, 60000, kOpenTgt, 0, kInv32, kInv32, kRest})
        .repeat(6, 2, 2)
        .step({7, kDistance, 150000, kOpenTgt, 0, kInv32, kInv32, kCooldown})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    const Program& p = *f.prog;
    ASSERT_EQ(p.stepCount, 8);
    EXPECT_EQ(p.steps[4].repeatFrom, 2);
    EXPECT_EQ(p.steps[4].repeatCount, 5u);
    EXPECT_EQ(p.steps[6].repeatFrom, 2);
    EXPECT_EQ(p.steps[6].repeatCount, 2u);
}

TEST(FitWorkoutReader, TwentyOneFlatSteps)
{
    Wkt w;
    w.fileId().workout("Pyramid", 21);
    for (uint16_t i = 0; i < 21; ++i) {
        w.step({i, i % 2 ? kDistance : kTime, i % 2 ? 20000u * i : 90000u});
    }
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_EQ(f.prog->stepCount, 21);
    EXPECT_EQ(f.prog->steps[19].distanceCm, 20000u * 19);
}

// --- Encoding variants -------------------------------------------------------

TEST(FitWorkoutReader, LittleEndianReadsTheSame)
{
    Fixture be, le;
    const auto build = [](bool bigEndian) {
        return Wkt(bigEndian).fileId().workout("Tempo", 2)
            .step({0, kTime, 600000, kSpeed, 0, 3279, 3509, kActive, "10 min"})
            .repeat(1, 0, 4)
            .bytes();
    };
    ASSERT_EQ(be.read(build(true)), Result::Ok);
    ASSERT_EQ(le.read(build(false)), Result::Ok);
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(be.prog->steps[i].end, le.prog->steps[i].end);
        EXPECT_EQ(be.prog->steps[i].durationMs, le.prog->steps[i].durationMs);
        EXPECT_EQ(be.prog->steps[i].speedLowMmps, le.prog->steps[i].speedLowMmps);
        EXPECT_EQ(be.prog->steps[i].repeatCount, le.prog->steps[i].repeatCount);
    }
    EXPECT_EQ(le.prog->steps[0].durationMs, 600000u);
}

TEST(FitWorkoutReader, TwelveByteHeader)
{
    Fixture f;
    const auto file = Wkt(true, 12).fileId().workout("Short header", 1)
        .step({0, kTime, 60000}).bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].durationMs, 60000u);
}

TEST(FitWorkoutReader, ZeroHeaderCrcIsAccepted)
{
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 1).step({0, kTime, 1000}).bytes(false)), Result::Ok);
}

TEST(FitWorkoutReader, CompressedTimestampRecordIsDecoded)
{
    // A step sent with a compressed-timestamp header on local type 1.
    Wkt w;
    w.fileId().workout("x", 1);
    w.b.def(1, 27, {{254, 2, kU16}, {1, 1, kEnum}, {2, 4, kU32}})
        .compressed(1, 7, {FVal::U(0), FVal::U(kTime), FVal::U(45000)});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].durationMs, 45000u);
}

TEST(FitWorkoutReader, DeveloperDataAndUnknownFieldsAreSkipped)
{
    Wkt w;
    w.fileId().workout("x", 1);
    w.b.def(0, 27, {{200, 3, 0x0D}, {254, 2, kU16}, {201, 4, kU32}, {1, 1, kEnum}, {2, 4, kU32}},
            true, {{0, 5, 0x0D}, {1, 2, kU16}})
        .data(0, {FVal::B({1, 2, 3}), FVal::U(0), FVal::U(7), FVal::U(kTime), FVal::U(30000)});
    // A message type the reader does not know at all.
    w.b.def(2, 300, {{0, 8, 0x0D}}).data(2, {FVal::B({9, 9, 9, 9, 9, 9, 9, 9})});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].durationMs, 30000u);
}

namespace {
// A source that hands out one byte at a time, to exercise buffering.
class Trickle : public SDK::Fit::IByteSource {
public:
    explicit Trickle(const std::vector<uint8_t>& d) : mD(d) {}
    size_t read(uint8_t* dst, size_t n) override
    {
        if (n == 0 || mPos == mD.size()) return 0;
        *dst = mD[mPos++];
        return 1;
    }
private:
    const std::vector<uint8_t>& mD;
    size_t                      mPos = 0;
};
}  // namespace

TEST(FitWorkoutReader, ShortReadsFromTheSourceAreFine)
{
    const auto file = easyRun();
    Trickle src(file);
    FitWorkoutReader reader;
    auto prog = std::make_unique<Program>();
    ASSERT_EQ(reader.read(src, *prog), Result::Ok);
    EXPECT_EQ(prog->steps[0].distanceCm, 650000u);
}

// --- memo_glob ---------------------------------------------------------------

TEST(FitWorkoutReader, MemoReplacesTheTruncatedDescription)
{
    const std::string full = std::string(255, 'a') + std::string(32, 'b');  // two parts
    Fixture f;
    const auto file = Wkt().fileId().workout("x", 1, full.substr(0, 160))
        .memo(26, 0, 17, 0, full.substr(0, 255), 255)
        .step({0, kTime, 1000, kOpenTgt, 0, kInv32, kInv32, kActive, "short"})
        .memo(27, 0, 8, 0, "short")
        .memo(26, 0, 17, 1, full.substr(255))
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_EQ(std::string(f.prog->description), full);
}

TEST(FitWorkoutReader, StepMemoBeforeItsStepStillWins)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("x", 1)
        .memo(27, 0, 8, 0, "The whole note, longer than the field")
        .step({0, kTime, 1000, kOpenTgt, 0, kInv32, kInv32, kActive, "The whole note, long"})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_STREQ(f.prog->steps[0].notes, "The whole note, longer than the field");
}

TEST(FitWorkoutReader, MemoPartOutOfOrderIsIgnored)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("x", 1, "field text")
        .memo(26, 0, 17, 0, "first,")
        .memo(26, 0, 17, 2, "third")  // part 1 never arrives
        .step({0, kTime, 1000})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_STREQ(f.prog->description, "first,");
}

TEST(FitWorkoutReader, LongTextIsCutOnACharacterBoundary)
{
    // 62 ASCII bytes, then a 3-byte character that cannot fit in 63.
    const std::string note = std::string(62, 'x') + "\xE2\x82\xAC" + "tail";
    Fixture f;
    const auto file = Wkt().fileId().workout("x", 1)
        .step({0, kTime, 1000})
        .memo(27, 0, 8, 0, note)
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_EQ(std::string(f.prog->steps[0].notes), std::string(62, 'x'));
}

// --- Targets -----------------------------------------------------------------

TEST(FitWorkoutReader, HeartRateTargets)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("HR", 4)
        .step({0, kTime, 60000, kHr, 3})                        // zone 3
        .step({1, kTime, 60000, kHr, 0, 240, 255})              // 140-155 bpm
        .step({2, kTime, 60000, kHr, 0, 70, 80})                // 70-80 % of max
        .step({3, kTime, 60000, kHr, 0, 80, 250})               // mixed: unusable
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    const Program& p = *f.prog;
    EXPECT_EQ(p.steps[0].target, Target::HeartRate);
    EXPECT_EQ(p.steps[0].hrKind, HeartRateTarget::Zone);
    EXPECT_EQ(p.steps[0].hrZone, 3);
    EXPECT_EQ(p.steps[1].hrKind, HeartRateTarget::Bpm);
    EXPECT_EQ(p.steps[1].hrLow, 140);
    EXPECT_EQ(p.steps[1].hrHigh, 155);
    EXPECT_EQ(p.steps[2].hrKind, HeartRateTarget::PercentMax);
    EXPECT_EQ(p.steps[2].hrLow, 70);
    EXPECT_EQ(p.steps[2].hrHigh, 80);
    EXPECT_EQ(p.steps[3].target, Target::Open);
    EXPECT_TRUE(p.steps[3].degraded);
    EXPECT_TRUE(p.degraded);
}

TEST(FitWorkoutReader, SpeedRangeGivenHighFirstIsOrdered)
{
    Fixture f;
    ASSERT_EQ(f.read(Wkt().fileId().workout("x", 1).step({0, kTime, 1000, kSpeed, 0, 3509, 3279}).bytes()),
              Result::Ok);
    EXPECT_EQ(f.prog->steps[0].speedLowMmps, 3279u);
    EXPECT_EQ(f.prog->steps[0].speedHighMmps, 3509u);
}

// --- Degraded steps ----------------------------------------------------------

TEST(FitWorkoutReader, UnsupportedThingsDegradeButStillRead)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("Odd", 7)
        .step({0, kTime, 60000, kCadence, 0, 85, 90})           // cadence target
        .step({1, kCalories, 50})                               // calorie end
        .step({2, kTime, 60000, kSpeed, 2})                     // speed zone 2
        .repeat(3, 2, 3, kRepeatTime)                           // repeat until time
        .step({4, kTime, 60000})
        .repeat(5, 4, 0)                                        // count 0
        .step({6, kTime, 60000, kOpenTgt, 0, kInv32, kInv32, kRecovery})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    const Program& p = *f.prog;
    EXPECT_EQ(p.steps[0].target, Target::Open);
    EXPECT_TRUE(p.steps[0].degraded);
    EXPECT_EQ(p.steps[1].end, StepEnd::Open);
    EXPECT_TRUE(p.steps[1].degraded);
    EXPECT_EQ(p.steps[2].target, Target::Open);
    EXPECT_TRUE(p.steps[2].degraded);
    EXPECT_EQ(p.steps[3].end, StepEnd::Repeat);
    EXPECT_EQ(p.steps[3].repeatCount, 1u);
    EXPECT_TRUE(p.steps[3].degraded);
    EXPECT_EQ(p.steps[5].repeatCount, 1u);
    EXPECT_TRUE(p.steps[5].degraded);
    EXPECT_EQ(p.steps[6].intensity, Intensity::Rest);
    EXPECT_FALSE(p.steps[6].degraded);
    EXPECT_TRUE(p.degraded);
    // The raw values survive for writing the workout back out.
    EXPECT_EQ(p.steps[0].raw.targetType, kCadence);
    EXPECT_EQ(p.steps[1].raw.durationType, kCalories);
    EXPECT_EQ(p.steps[3].raw.durationType, kRepeatTime);
}

// --- Broken files ------------------------------------------------------------

TEST(FitWorkoutReader, DamagedFilesAreRejected)
{
    Fixture f;
    auto file = easyRun();

    auto badCrc = file;
    badCrc.back() ^= 0x01;
    EXPECT_EQ(f.read(badCrc), Result::BadFileCrc);

    auto badHeaderCrc = file;
    badHeaderCrc[12] ^= 0x01;
    EXPECT_EQ(f.read(badHeaderCrc), Result::BadHeaderCrc);

    auto notFit = file;
    notFit[9] = 'X';
    EXPECT_EQ(f.read(notFit), Result::NotFit);

    auto cut = file;
    cut.resize(cut.size() - 10);
    EXPECT_EQ(f.read(cut), Result::Truncated);

    EXPECT_EQ(f.read({}), Result::NotFit);
}

TEST(FitWorkoutReader, DataWithoutADefinitionIsMalformed)
{
    Wkt w;
    w.fileId();
    w.b.raw({0x05, 0x00});  // data record for local type 5, never defined
    Fixture f;
    EXPECT_EQ(f.read(w.bytes()), Result::Malformed);
}

TEST(FitWorkoutReader, NotAWorkoutFile)
{
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId(4).workout("x", 1).step({0, kTime, 1000}).bytes()), Result::NotWorkout);
}

TEST(FitWorkoutReader, NoSteps)
{
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 0).bytes()), Result::NoSteps);
}

TEST(FitWorkoutReader, StepIndexesMustBeUniqueAndInRange)
{
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 2).step({0, kTime, 1}).step({0, kTime, 2}).bytes()),
              Result::BadStepIndex);
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 2).step({0, kTime, 1}).step({2, kTime, 2}).bytes()),
              Result::BadStepIndex);
    // num_valid_steps says 3, the file has 2.
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 3).step({0, kTime, 1}).step({1, kTime, 2}).bytes()),
              Result::BadStepIndex);
    // Without num_valid_steps the indexes must still run 0..n-1.
    EXPECT_EQ(f.read(Wkt().fileId().workoutNoCount("x").step({1, kTime, 1}).bytes()), Result::BadStepIndex);
    EXPECT_EQ(f.read(Wkt().fileId().workoutNoCount("x").step({0, kTime, 1}).bytes()), Result::Ok);
}

TEST(FitWorkoutReader, TooManySteps)
{
    Wkt w;
    w.fileId().workout("x", SDK::Workout::kMaxSteps + 1);
    for (uint16_t i = 0; i <= SDK::Workout::kMaxSteps; ++i) {
        w.step({i, kTime, 1000});
    }
    Fixture f;
    EXPECT_EQ(f.read(w.bytes()), Result::TooManySteps);
}

TEST(FitWorkoutReader, RepeatsMustPointBackAndNest)
{
    Fixture f;
    // Points at itself.
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 2).step({0, kTime, 1}).repeat(1, 1, 2).bytes()),
              Result::BadRepeat);
    // Blocks [0..2] and [1..4] overlap without nesting.
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 5)
                         .step({0, kTime, 1}).step({1, kTime, 1}).repeat(2, 0, 2)
                         .step({3, kTime, 1}).repeat(4, 1, 2).bytes()),
              Result::BadRepeat);
}

TEST(FitWorkoutReader, NestingDepthIsLimited)
{
    const auto build = [](uint16_t depth) {
        Wkt w;
        w.fileId().workout("x", static_cast<uint16_t>(depth + 1));
        w.step({0, kTime, 1});
        for (uint16_t d = 1; d <= depth; ++d) {
            w.repeat(d, 0, 2);
        }
        return w.bytes();
    };
    Fixture f;
    EXPECT_EQ(f.read(build(SDK::Workout::kMaxRepeatDepth)), Result::Ok);
    EXPECT_EQ(f.read(build(SDK::Workout::kMaxRepeatDepth + 1)), Result::NestingTooDeep);
}

// --- Reuse -------------------------------------------------------------------

TEST(FitWorkoutReader, ReusingReaderAndProgramLeavesNothingBehind)
{
    Fixture f;
    ASSERT_EQ(f.read(Wkt().fileId().workout("First", 2, "long description")
                         .step({0, kTime, 1000, kOpenTgt, 0, kInv32, kInv32, kActive, "one"})
                         .step({1, kTime, 2000, kOpenTgt, 0, kInv32, kInv32, kActive, "two"})
                         .memo(27, 1, 8, 0, "two from memo")
                         .bytes()),
              Result::Ok);
    ASSERT_EQ(f.read(easyRun()), Result::Ok);
    EXPECT_STREQ(f.prog->name, "Easy 6.5km");
    EXPECT_EQ(f.prog->stepCount, 1);
    EXPECT_STREQ(f.prog->steps[1].notes, "");
    EXPECT_STREQ(f.prog->steps[0].notes, "6.5km easy");
}

TEST(FitWorkoutReader, ResultNamesAreSet)
{
    for (int r = 0; r <= static_cast<int>(Result::ReadError); ++r) {
        EXPECT_STRNE(FitWorkoutReader::resultName(static_cast<Result>(r)), "?");
    }
}

// --- Review follow-ups -------------------------------------------------------

TEST(FitWorkoutReader, CharacterSplitAcrossMemoPartsIsDroppedWhenCut)
{
    // The description holds 511 bytes. Parts 0-1 give 510, part 2 ends with
    // the first byte of a 2-byte character, and part 3 starts with its second
    // byte, which no longer fits. The lead byte must not be left on its own.
    Fixture f;
    const auto file = Wkt().fileId().workout("x", 1)
        .memo(26, 0, 17, 0, std::string(255, 'a'), 255)
        .memo(26, 0, 17, 1, std::string(255, 'a'), 255)
        .memo(26, 0, 17, 2, "\xC3", 1)
        .memo(26, 0, 17, 3, "\xA9more")
        .step({0, kTime, 1000})
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_EQ(std::string(f.prog->description), std::string(510, 'a'));
}

TEST(FitWorkoutReader, StepWithoutAnIndexIsRejected)
{
    Wkt w;
    w.fileId().workout("x", 1);
    w.b.def(0, 27, {{1, 1, kEnum}, {2, 4, kU32}}).data(0, {FVal::U(kTime), FVal::U(1000)});
    Fixture f;
    EXPECT_EQ(f.read(w.bytes()), Result::BadStepIndex);
}

TEST(FitWorkoutReader, IndexBeyondTheLimitInASmallWorkoutIsABadIndex)
{
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 2).step({0, kTime, 1}).step({150, kTime, 1}).bytes()),
              Result::BadStepIndex);
}

TEST(FitWorkoutReader, HeartRateOutsidePlausibleRangesDegrades)
{
    Fixture f;
    const auto file = Wkt().fileId().workout("HR", 2)
        .step({0, kTime, 60000, kHr, 0, 70000, 70100})  // not a heart rate
        .step({1, kTime, 60000, kHr, 0, 101, 355})      // 1-255 bpm: the extremes
        .bytes();
    ASSERT_EQ(f.read(file), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].target, Target::Open);
    EXPECT_TRUE(f.prog->steps[0].degraded);
    EXPECT_EQ(f.prog->steps[1].hrKind, HeartRateTarget::Bpm);
    EXPECT_EQ(f.prog->steps[1].hrLow, 1);
    EXPECT_EQ(f.prog->steps[1].hrHigh, 255);
}

TEST(FitWorkoutReader, HugeRepeatCountIsClamped)
{
    Fixture f;
    ASSERT_EQ(f.read(Wkt().fileId().workout("x", 3).step({0, kTime, 1}).repeat(1, 0, 0xFFFFFFFEu)
                         .repeat(2, 0, SDK::Workout::kMaxRepeatCount).bytes()),
              Result::Ok);
    EXPECT_EQ(f.prog->steps[1].repeatCount, SDK::Workout::kMaxRepeatCount);
    EXPECT_TRUE(f.prog->steps[1].degraded);
    EXPECT_EQ(f.prog->steps[2].repeatCount, SDK::Workout::kMaxRepeatCount);
    EXPECT_FALSE(f.prog->steps[2].degraded);
}

TEST(FitWorkoutReader, RepeatStartingAtARepeatIsRejected)
{
    // Step 2 repeats [0..1]; step 3 repeats from step 2, so [2..3] overlaps
    // [0..2] without nesting.
    Fixture f;
    EXPECT_EQ(f.read(Wkt().fileId().workout("x", 4).step({0, kTime, 1}).step({1, kTime, 1})
                         .repeat(2, 0, 2).repeat(3, 2, 2).bytes()),
              Result::BadRepeat);
}

TEST(FitWorkoutReader, StepNameIsRead)
{
    Wkt w;
    w.fileId().workout("x", 1);
    w.b.def(0, 27, {{254, 2, kU16}, {0, 16, kStr}, {1, 1, kEnum}, {2, 4, kU32}})
        .data(0, {FVal::U(0), FVal::T("Strides"), FVal::U(kTime), FVal::U(20000)});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_STREQ(f.prog->steps[0].name, "Strides");
}

TEST(FitWorkoutReader, SignedAndFloatFieldsCountAsAbsent)
{
    // duration_value sent as sint32: absent, so the time step degrades to open.
    Wkt w;
    w.fileId().workout("x", 1);
    w.b.def(0, 27, {{254, 2, kU16}, {1, 1, kEnum}, {2, 4, 0x85}})
        .data(0, {FVal::U(0), FVal::U(kTime), FVal::U(60000)});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].end, StepEnd::Open);
    EXPECT_TRUE(f.prog->steps[0].degraded);
}

TEST(FitWorkoutReader, MemoWithoutAParentIsIgnoredForSteps)
{
    Wkt w;
    w.fileId().workout("x", 1).step({0, kTime, 1, kOpenTgt, 0, kInv32, kInv32, kActive, "field"});
    w.b.def(0, 145, {{250, 4, kU32}, {1, 2, kU16}, {3, 1, kU8}, {4, 16, kU8z}})
        .data(0, {FVal::U(0), FVal::U(27), FVal::U(8), FVal::T("no parent")});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_STREQ(f.prog->steps[0].notes, "field");
}

TEST(FitWorkoutReader, OlderMemoFieldIsAccepted)
{
    Wkt w;
    w.fileId().workout("x", 1, "field text");
    w.b.def(0, 145, {{250, 4, kU32}, {1, 2, kU16}, {2, 2, kU16}, {3, 1, kU8}, {0, 16, 0x0D}})
        .data(0, {FVal::U(0), FVal::U(26), FVal::U(0), FVal::U(17), FVal::T("from field 0")});
    w.step({0, kTime, 1});
    Fixture f;
    ASSERT_EQ(f.read(w.bytes()), Result::Ok);
    EXPECT_STREQ(f.prog->description, "from field 0");
}

TEST(FitWorkoutReader, MemoForAStepThatNeverArrivesLeavesNothing)
{
    Fixture f;
    ASSERT_EQ(f.read(Wkt().fileId().workout("x", 1).step({0, kTime, 1}).memo(27, 99, 0, 0, "ghost").bytes()),
              Result::Ok);
    EXPECT_STREQ(f.prog->steps[99].name, "");
}

TEST(FitWorkoutReader, HeaderLongerThanFourteenBytes)
{
    Fixture f;
    ASSERT_EQ(f.read(Wkt(true, 16).fileId().workout("x", 1).step({0, kTime, 5000}).bytes()), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].durationMs, 5000u);
}

TEST(FitWorkoutReader, ProtocolThreeIsNotDecoded)
{
    auto file = easyRun();
    file[1] = 0x30;
    file[12] = file[13] = 0;  // header CRC now stale; zero means "not given"
    Fixture f;
    EXPECT_EQ(f.read(file), Result::NotFit);
}

TEST(FitWorkoutReader, RecordStraddlingTheDataEndIsMalformed)
{
    // Claim one byte less data than there is: the last record crosses the end.
    auto file = easyRun();
    const uint32_t size = file[4] | (file[5] << 8) | (file[6] << 16) | (uint32_t(file[7]) << 24);
    const uint32_t less = size - 1;
    for (int i = 0; i < 4; ++i) file[4 + i] = static_cast<uint8_t>(less >> (8 * i));
    file[12] = file[13] = 0;
    Fixture f;
    EXPECT_EQ(f.read(file), Result::Malformed);
}

TEST(FitWorkoutReader, HugeDataSizeIsNotFit)
{
    auto file = easyRun();
    file[4] = file[5] = file[6] = 0xFF;
    file[7] = 0xFF;
    file[12] = file[13] = 0;
    Fixture f;
    EXPECT_EQ(f.read(file), Result::NotFit);
}

TEST(FitWorkoutReader, BytesAfterTheFileCrcAreIgnored)
{
    auto file = easyRun();
    file.insert(file.end(), {1, 2, 3, 4, 5});
    Fixture f;
    EXPECT_EQ(f.read(file), Result::Ok);
}

TEST(FitWorkoutReader, ReadsFromAFile)
{
    SDK::Test::FakeFileSystem fs;
    const auto bytes = easyRun();
    fs.seedFile("w.fit", std::string(bytes.begin(), bytes.end()));
    Fixture f;
    auto file = fs.file("w.fit");
    ASSERT_EQ(f.reader.read(*file, *f.prog), Result::Ok);
    EXPECT_EQ(f.prog->steps[0].distanceCm, 650000u);
    EXPECT_FALSE(file->isOpen());

    auto missing = fs.file("none.fit");
    EXPECT_EQ(f.reader.read(*missing, *f.prog), Result::ReadError);
}

namespace {
// Fails after handing out @p goodBytes bytes.
class FailingSource : public SDK::Fit::IByteSource {
public:
    FailingSource(const std::vector<uint8_t>& d, size_t goodBytes) : mD(d), mGood(goodBytes) {}
    size_t read(uint8_t* dst, size_t n) override
    {
        if (mPos >= mGood) { mFailed = true; return 0; }
        const size_t k = std::min(n, mGood - mPos);
        std::memcpy(dst, mD.data() + mPos, k);
        mPos += k;
        return k;
    }
    bool failed() const override { return mFailed; }
private:
    const std::vector<uint8_t>& mD;
    size_t mGood, mPos = 0;
    bool   mFailed = false;
};
}  // namespace

TEST(FitWorkoutReader, AReadErrorIsReportedAsSuch)
{
    const auto file = easyRun();
    FailingSource src(file, file.size() / 2);
    FitWorkoutReader reader;
    auto prog = std::make_unique<Program>();
    EXPECT_EQ(reader.read(src, *prog), Result::ReadError);
}

TEST(FitWorkoutReader, AnotherFileTypeIsTurnedAwayAfterItsFileId)
{
    // The source fails just past file_id: a reader that went on would
    // report a read error instead.
    Wkt w;
    w.fileId(4).workout("x", 3);
    for (uint16_t i = 0; i < 3; ++i) {
        w.step({i, kTime, 1000});
    }
    const auto file = w.bytes();
    FailingSource src(file, Wkt().fileId(4).bytes().size() - 2);
    FitWorkoutReader reader;
    auto prog = std::make_unique<Program>();
    EXPECT_EQ(reader.read(src, *prog), Result::NotWorkout);
}

TEST(FitWorkoutReader, MutatedFilesNeverBreakTheProgram)
{
    // Seeded mutation smoke test: damaged input may be rejected, but it must
    // never leave a Program that breaks its own invariants. Most mutations
    // re-fix the CRCs so they reach the record decoder.
    const std::vector<std::vector<uint8_t>> seeds = {
        easyRun(),
        Wkt().fileId().workout("400s", 8, "desc")
            .step({0, kOpenDur, kInv32, kOpenTgt, 0, kInv32, kInv32, kWarmup, "wu"})
            .step({1, kTime, 90000})
            .step({2, kDistance, 40000, kSpeed, 0, 4082, 4444})
            .memo(27, 2, 8, 0, "400m at pace")
            .step({3, kTime, 60000})
            .repeat(4, 2, 5)
            .step({5, kTime, 60000})
            .repeat(6, 2, 2)
            .step({7, kDistance, 150000, kHr, 0, 240, 255})
            .bytes(),
    };
    uint32_t rng = 12345;
    const auto next = [&rng]() { rng = rng * 1664525u + 1013904223u; return rng >> 8; };
    Fixture f;
    for (int iter = 0; iter < 20000; ++iter) {
        std::vector<uint8_t> d = seeds[iter % seeds.size()];
        const int edits = 1 + static_cast<int>(next() % 4);
        for (int e = 0; e < edits && !d.empty(); ++e) {
            const size_t at = next() % d.size();
            switch (next() % 4) {
            case 0: d[at] = static_cast<uint8_t>(next()); break;
            case 1: d.insert(d.begin() + static_cast<long>(at), static_cast<uint8_t>(next())); break;
            case 2: d.erase(d.begin() + static_cast<long>(at)); break;
            case 3: d.resize(at); break;
            }
        }
        if (next() % 10 < 7 && d.size() >= 16 && d[0] >= 14) {
            // Re-fix the data size and both CRCs.
            const uint32_t body = static_cast<uint32_t>(d.size() - d[0] - 2);
            for (int i = 0; i < 4; ++i) d[4 + i] = static_cast<uint8_t>(body >> (8 * i));
            const uint16_t hc = SDK::Fit::fitCrcUpdate(0, d.data(), 12);
            d[12] = static_cast<uint8_t>(hc); d[13] = static_cast<uint8_t>(hc >> 8);
            const uint16_t fc = SDK::Fit::fitCrcUpdate(0, d.data(), d.size() - 2);
            d[d.size() - 2] = static_cast<uint8_t>(fc); d[d.size() - 1] = static_cast<uint8_t>(fc >> 8);
        }
        if (f.read(d) != Result::Ok) {
            continue;
        }
        const Program& p = *f.prog;
        ASSERT_GT(p.stepCount, 0);
        ASSERT_LE(p.stepCount, SDK::Workout::kMaxSteps);
        ASSERT_EQ(p.name[sizeof(p.name) - 1], '\0');
        ASSERT_EQ(p.description[sizeof(p.description) - 1], '\0');
        for (uint16_t i = 0; i < p.stepCount; ++i) {
            const auto& s = p.steps[i];
            ASSERT_EQ(s.notes[sizeof(s.notes) - 1], '\0');
            ASSERT_EQ(s.name[sizeof(s.name) - 1], '\0');
            if (s.end == StepEnd::Repeat) {
                ASSERT_LT(s.repeatFrom, i);
                ASSERT_GE(s.repeatCount, 1u);
                ASSERT_LE(s.repeatCount, SDK::Workout::kMaxRepeatCount);
            }
        }
    }
}
