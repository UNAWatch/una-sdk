/**
 ******************************************************************************
 * @file    FitWorkoutWriter_test.cpp
 * @brief   Tests for SDK::Fit::writeWorkout(): programs written to a FIT
 *          workout file and read back with FitWorkoutReader.
 ******************************************************************************
 */

#include "SDK/Fit/FitWorkoutWriter.hpp"

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWorkoutReader.hpp"
#include "SDK/Workout/Intervals.hpp"
#include "FakeFileSystem.hpp"
#include "fit/FitReader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace fit = SDK::Fit;
using namespace SDK::Workout;

namespace {

constexpr fit::WorkoutLocalTypes kLocal{1, 2, 3};

// @p program written as a FIT workout file, as bytes.
std::vector<uint8_t> writeFile(SDK::Test::FakeFileSystem& fs, const Program& program)
{
    auto file = fs.file("w.fit");
    EXPECT_TRUE(file->open(true, true));
    fit::FitWriter w(*file);
    EXPECT_TRUE(w.begin(0));
    EXPECT_TRUE(w.defineMessage(0, fit::mesgNum(fit::MesgNum::FileId), {fit::field::FileId::Type}));
    EXPECT_TRUE(w.data(0).u8(static_cast<uint8_t>(fit::File::Workout)).write());
    EXPECT_TRUE(fit::writeWorkout(w, program, kLocal));
    EXPECT_TRUE(w.finish());
    file->close();
    const std::string s = fs.fileContents("w.fit");
    return std::vector<uint8_t>(s.begin(), s.end());
}

// @p program written, then read back into @p back.
fit::FitWorkoutReader::Result roundTrip(const Program& program, Program& back)
{
    SDK::Test::FakeFileSystem fs;
    writeFile(fs, program);
    fit::FitWorkoutReader reader;
    auto file = fs.file("w.fit");
    return reader.read(*file, back);
}

void setText(char* dst, size_t cap, const char* src)
{
    const size_t n = std::min(std::strlen(src), cap - 1);
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

Step& add(Program& p)
{
    Step& s = p.steps[p.stepCount++];
    s = Step{};
    return s;
}

// A program with every kind of step the reader understands, plus one it
// runs in a simplified form, with raw values as a file would carry them.
std::unique_ptr<Program> mixedProgram()
{
    auto p = std::make_unique<Program>();
    setText(p->name, sizeof(p->name), "Mixed");
    setText(p->description, sizeof(p->description), "Short.");
    p->sport    = static_cast<uint8_t>(fit::Sport::Running);
    p->subSport = static_cast<uint8_t>(fit::SubSport::Generic);

    Step& warm = add(*p);  // 0: open warm-up with notes
    warm.end = StepEnd::Open;
    warm.intensity = Intensity::Warmup;
    setText(warm.notes, sizeof(warm.notes), "OWU: 2.5km warm up");
    warm.raw.durationType = static_cast<uint8_t>(fit::WktStepDuration::Open);
    warm.raw.targetType   = static_cast<uint8_t>(fit::WktStepTarget::Open);
    warm.raw.intensity    = static_cast<uint8_t>(fit::Intensity::Warmup);

    Step& run = add(*p);  // 1: 400 m at a speed range
    run.end = StepEnd::Distance;
    run.distanceCm = 40000;
    run.target = Target::Speed;
    run.speedLowMmps  = 4082;
    run.speedHighMmps = 4444;
    setText(run.name, sizeof(run.name), "Rep");
    run.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::Distance);
    run.raw.durationValue = 40000;
    run.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::Speed);
    run.raw.targetValue   = 0;
    run.raw.customLow     = 4082;
    run.raw.customHigh    = 4444;
    run.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Active);

    Step& rest = add(*p);  // 2: 60 s rest
    rest.end = StepEnd::Time;
    rest.durationMs = 60000;
    rest.intensity = Intensity::Rest;
    rest.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::Time);
    rest.raw.durationValue = 60000;
    rest.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::Open);
    rest.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Rest);

    Step& rep = add(*p);  // 3: steps 1-2 five times
    rep.end = StepEnd::Repeat;
    rep.repeatFrom  = 1;
    rep.repeatCount = 5;
    rep.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::RepeatUntilStepsComplete);
    rep.raw.durationValue = 1;
    rep.raw.targetValue   = 5;

    Step& hr = add(*p);  // 4: 10 min at 70-80 % of max heart rate
    hr.end = StepEnd::Time;
    hr.durationMs = 600000;
    hr.target = Target::HeartRate;
    hr.hrKind = HeartRateTarget::PercentMax;
    hr.hrLow  = 70;
    hr.hrHigh = 80;
    hr.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::Time);
    hr.raw.durationValue = 600000;
    hr.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::HeartRate);
    hr.raw.targetValue   = 0;
    hr.raw.customLow     = 70;
    hr.raw.customHigh    = 80;
    hr.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Active);

    Step& cal = add(*p);  // 5: ends on calories, which the watch runs as open
    cal.end = StepEnd::Open;
    cal.intensity = Intensity::Cooldown;
    cal.degraded = true;
    cal.raw.durationType  = 4;  // calories
    cal.raw.durationValue = 200;
    cal.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::Open);
    cal.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Cooldown);
    p->degraded = true;
    return p;
}

void expectSameStep(const Step& a, const Step& b, int i)
{
    EXPECT_EQ(a.end, b.end) << "step " << i;
    EXPECT_EQ(a.durationMs, b.durationMs) << "step " << i;
    EXPECT_EQ(a.distanceCm, b.distanceCm) << "step " << i;
    EXPECT_EQ(a.repeatFrom, b.repeatFrom) << "step " << i;
    EXPECT_EQ(a.repeatCount, b.repeatCount) << "step " << i;
    EXPECT_EQ(a.target, b.target) << "step " << i;
    EXPECT_EQ(a.speedLowMmps, b.speedLowMmps) << "step " << i;
    EXPECT_EQ(a.speedHighMmps, b.speedHighMmps) << "step " << i;
    EXPECT_EQ(a.hrKind, b.hrKind) << "step " << i;
    EXPECT_EQ(a.hrLow, b.hrLow) << "step " << i;
    EXPECT_EQ(a.hrHigh, b.hrHigh) << "step " << i;
    EXPECT_EQ(a.intensity, b.intensity) << "step " << i;
    EXPECT_EQ(a.degraded, b.degraded) << "step " << i;
    EXPECT_STREQ(a.notes, b.notes) << "step " << i;
    EXPECT_STREQ(a.name, b.name) << "step " << i;
    EXPECT_EQ(a.raw.durationType, b.raw.durationType) << "step " << i;
    EXPECT_EQ(a.raw.durationValue, b.raw.durationValue) << "step " << i;
    EXPECT_EQ(a.raw.targetType, b.raw.targetType) << "step " << i;
    EXPECT_EQ(a.raw.customLow, b.raw.customLow) << "step " << i;
    EXPECT_EQ(a.raw.customHigh, b.raw.customHigh) << "step " << i;
    EXPECT_EQ(a.raw.intensity, b.raw.intensity) << "step " << i;
}

}  // namespace

TEST(FitWorkoutWriter, ProgramReadsBackTheSame)
{
    const auto p = mixedProgram();
    auto back = std::make_unique<Program>();
    ASSERT_EQ(roundTrip(*p, *back), fit::FitWorkoutReader::Result::Ok);
    EXPECT_STREQ(back->name, p->name);
    EXPECT_STREQ(back->description, p->description);
    EXPECT_EQ(back->sport, p->sport);
    EXPECT_EQ(back->subSport, p->subSport);
    EXPECT_EQ(back->degraded, p->degraded);
    ASSERT_EQ(back->stepCount, p->stepCount);
    for (uint16_t i = 0; i < p->stepCount; ++i) {
        expectSameStep(p->steps[i], back->steps[i], i);
    }
}

TEST(FitWorkoutWriter, LongDescriptionGoesWholeIntoMemoParts)
{
    auto p = mixedProgram();
    // 462 bytes with two-byte characters across the 160-byte cut and the
    // memo part boundaries.
    std::string text;
    while (text.size() < 470) {
        text += "Steady \xC3\xA9 ";
    }
    text.resize(462);
    if ((static_cast<uint8_t>(text.back()) & 0xC0) == 0xC0) {
        text.back() = '.';  // don't end on half a character
    }
    setText(p->description, sizeof(p->description), text.c_str());

    SDK::Test::FakeFileSystem fs;
    const auto bytes = writeFile(fs, *p);

    // wkt_description holds a prefix that ends on a whole character.
    testfit::FitReader r(bytes);
    ASSERT_TRUE(r.ok());
    const auto workouts = r.withGlobal(fit::mesgNum(fit::MesgNum::Workout));
    ASSERT_EQ(workouts.size(), 1u);
    const auto& raw = workouts[0]->fields.at(fit::field::Workout::kWktDescriptionNum).raw;
    ASSERT_EQ(raw.size(), fit::kWktDescriptionBytes);
    const std::string prefix(reinterpret_cast<const char*>(raw.data()));
    EXPECT_LT(prefix.size(), static_cast<size_t>(fit::kWktDescriptionBytes));
    EXPECT_EQ(text.compare(0, prefix.size(), prefix), 0);
    EXPECT_NE(static_cast<uint8_t>(prefix.back()) & 0xC0, 0xC0u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::MemoGlob)).size(), 2u);

    // The reader puts the whole text back together.
    fit::FitWorkoutReader reader;
    auto back = std::make_unique<Program>();
    auto file = fs.file("w.fit");
    ASSERT_EQ(reader.read(*file, *back), fit::FitWorkoutReader::Result::Ok);
    EXPECT_EQ(std::string(back->description), text);
}

TEST(FitWorkoutWriter, ShortDescriptionHasNoMemoParts)
{
    const auto p = mixedProgram();
    SDK::Test::FakeFileSystem fs;
    testfit::FitReader r(writeFile(fs, *p));
    ASSERT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid());
    EXPECT_TRUE(r.withGlobal(fit::mesgNum(fit::MesgNum::MemoGlob)).empty());
}

TEST(FitWorkoutWriter, UnlimitedRepeatIsLeftOutAndLaterStepsRenumbered)
{
    IntervalsSpec spec;
    spec.repeats = 0;
    spec.run.end = IntervalsSpec::Phase::End::Time;
    spec.run.timeMs = 60000;
    spec.rest.end = IntervalsSpec::Phase::End::Time;
    spec.rest.timeMs = 30000;
    auto p = std::make_unique<Program>();
    const IntervalsLayout l = buildIntervals(spec, *p);
    ASSERT_NE(l.repeat, IntervalsLayout::kNone);
    ASSERT_NE(l.coolDown, IntervalsLayout::kNone);

    auto back = std::make_unique<Program>();
    ASSERT_EQ(roundTrip(*p, *back), fit::FitWorkoutReader::Result::Ok);
    ASSERT_EQ(back->stepCount, p->stepCount - 1);
    for (uint16_t i = 0; i < back->stepCount; ++i) {
        EXPECT_NE(back->steps[i].end, StepEnd::Repeat) << i;
    }
    // The cool-down moved down by one, where fitStepIndex() says it is.
    uint16_t coolDown = 0;
    ASSERT_TRUE(fitStepIndex(*p, l.coolDown, coolDown));
    EXPECT_EQ(coolDown, l.coolDown - 1);
    EXPECT_EQ(back->steps[coolDown].intensity, Intensity::Cooldown);
}

TEST(FitWorkoutWriter, IntervalsWithARepeatReadBackTheSame)
{
    IntervalsSpec spec;
    spec.repeats = 4;
    spec.lastRest = false;
    spec.run.end = IntervalsSpec::Phase::End::Distance;
    spec.run.distanceCm = 80000;
    spec.rest.end = IntervalsSpec::Phase::End::Time;
    spec.rest.timeMs = 90000;
    auto p = std::make_unique<Program>();
    buildIntervals(spec, *p);

    auto back = std::make_unique<Program>();
    ASSERT_EQ(roundTrip(*p, *back), fit::FitWorkoutReader::Result::Ok);
    EXPECT_STREQ(back->name, "Intervals");
    ASSERT_EQ(back->stepCount, p->stepCount);
    for (uint16_t i = 0; i < p->stepCount; ++i) {
        expectSameStep(p->steps[i], back->steps[i], i);
    }
}
