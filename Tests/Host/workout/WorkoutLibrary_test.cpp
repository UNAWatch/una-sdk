/**
 ******************************************************************************
 * @file    WorkoutLibrary_test.cpp
 * @brief   Tests for SDK::Workout::Library, scheduleDate() and ScheduleState,
 *          on workout files written into an in-memory file system.
 ******************************************************************************
 */

#include "SDK/Workout/ScheduleState.hpp"
#include "SDK/Workout/WorkoutLibrary.hpp"

#include "KernelTestDoubles.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWorkoutWriter.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace SDK::Workout;
namespace fit = SDK::Fit;
using SDK::TestSupport::InMemoryFileSystem;

namespace {

void setText(char* dst, size_t cap, const char* src)
{
    const size_t n = std::min(std::strlen(src), cap - 1);
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

// A workout of @p distanceCm of running, then 60 s of rest, @p reps times.
std::unique_ptr<Program> program(const char* name, uint32_t distanceCm = 100000,
                                 uint32_t reps = 1, uint8_t sport = 1)
{
    auto p = std::make_unique<Program>();
    setText(p->name, sizeof(p->name), name);
    p->sport = sport;
    Step& run = p->steps[p->stepCount++];
    run.end = StepEnd::Distance;
    run.distanceCm = distanceCm;
    run.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::Distance);
    run.raw.durationValue = distanceCm;
    run.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::Open);
    run.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Active);
    Step& rest = p->steps[p->stepCount++];
    rest.end = StepEnd::Time;
    rest.durationMs = 60000;
    rest.intensity = Intensity::Rest;
    rest.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::Time);
    rest.raw.durationValue = 60000;
    rest.raw.targetType    = static_cast<uint8_t>(fit::WktStepTarget::Open);
    rest.raw.intensity     = static_cast<uint8_t>(fit::Intensity::Rest);
    if (reps > 1) {
        Step& rep = p->steps[p->stepCount++];
        rep.end = StepEnd::Repeat;
        rep.repeatFrom  = 0;
        rep.repeatCount = reps;
        rep.raw.durationType  = static_cast<uint8_t>(fit::WktStepDuration::RepeatUntilStepsComplete);
        rep.raw.durationValue = 0;
        rep.raw.targetValue   = reps;
    }
    Step& open = p->steps[p->stepCount++];
    open.end = StepEnd::Open;
    open.intensity = Intensity::Cooldown;
    open.raw.durationType = static_cast<uint8_t>(fit::WktStepDuration::Open);
    open.raw.targetType   = static_cast<uint8_t>(fit::WktStepTarget::Open);
    open.raw.intensity    = static_cast<uint8_t>(fit::Intensity::Cooldown);
    return p;
}

void writeFile(InMemoryFileSystem& fs, const std::string& path, const Program& p)
{
    auto file = fs.file(path.c_str());
    ASSERT_TRUE(file->open(true, true));
    fit::FitWriter w(*file);
    ASSERT_TRUE(w.begin(0));
    ASSERT_TRUE(w.defineMessage(0, fit::mesgNum(fit::MesgNum::FileId), {fit::field::FileId::Type}));
    ASSERT_TRUE(w.data(0).u8(static_cast<uint8_t>(fit::File::Workout)).write());
    ASSERT_TRUE(fit::writeWorkout(w, p, {1, 2, 3}));
    ASSERT_TRUE(w.finish());
    file->close();
}

std::string scheduled(const char* file)
{
    return std::string(kScheduleDir) + "/" + file;
}

std::string library(const char* file)
{
    return std::string(kWorkoutsDir) + "/" + file;
}

struct Scan {
    InMemoryFileSystem            fs;
    Library                       lib;
    fit::FitWorkoutReader         reader;
    std::unique_ptr<Program>      scratch = std::make_unique<Program>();

    void add(const std::string& path, const Program& p) { writeFile(fs, path, p); }
    void run(Ymd today) { lib.scan(fs, today, reader, *scratch); }

    std::vector<std::string> files() const
    {
        std::vector<std::string> out;
        for (size_t i = 0; i < lib.size(); ++i) out.push_back(lib.entry(i).file);
        return out;
    }
};

}  // namespace

// --- scheduleDate ----------------------------------------------------------------

TEST(ScheduleDate, ReadsTheDayFromTheFileName)
{
    EXPECT_EQ(scheduleDate("2026-10-09.fit"), 20261009u);
    EXPECT_EQ(scheduleDate("2026-10-09.FIT"), 20261009u);
    EXPECT_EQ(scheduleDate("2028-02-29.fit"), 20280229u);  // leap year
    EXPECT_EQ(scheduleDate("2000-02-29.fit"), 20000229u);
}

TEST(ScheduleDate, RejectsAnythingElse)
{
    for (const char* bad : {"2026-02-29.fit", "1900-02-29.fit", "2026-13-01.fit", "2026-00-10.fit",
                            "2026-04-31.fit", "2026-10-00.fit", "2026-10-9.fit", "2026-10-09.txt",
                            "2026_10_09.fit", "2026-10-09-2.fit", "Long run.fit", "0000-01-01.fit",
                            "2026-1a-09.fit", ""}) {
        EXPECT_EQ(scheduleDate(bad), 0u) << bad;
    }
}

// --- Library ---------------------------------------------------------------------

TEST(WorkoutLibrary, ListsTodayThenComingThenPastThenTheOthersByName)
{
    Scan s;
    s.add(scheduled("2026-10-05.fit"), *program("Tue"));
    s.add(scheduled("2026-10-09.fit"), *program("Fri"));
    s.add(scheduled("2026-10-07.fit"), *program("Wed"));
    s.add(scheduled("2026-09-30.fit"), *program("Old"));
    s.add(scheduled("2026-10-12.fit"), *program("Mon"));
    s.add(library("tempo.fit"), *program("b Tempo"));
    s.add(library("easy.fit"), *program("A Easy"));
    s.add(scheduled("notes.fit"), *program("Long"));  // not a date: listed with the others
    s.run(20261007);

    EXPECT_EQ(s.files(), (std::vector<std::string>{"2026-10-07.fit", "2026-10-09.fit", "2026-10-12.fit",
                                                    "2026-10-05.fit", "2026-09-30.fit", "easy.fit",
                                                    "tempo.fit", "notes.fit"}));  // A, b, L: no case
    EXPECT_EQ(s.lib.today(), 0u);
    EXPECT_EQ(s.lib.entry(0).date, 20261007u);
    EXPECT_STREQ(s.lib.entry(0).name, "Wed");
    EXPECT_EQ(s.lib.entry(7).date, 0u);
    EXPECT_TRUE(s.lib.entry(7).inSchedule);
    EXPECT_EQ(s.lib.dropped(), 0u);

    char path[96];
    ASSERT_TRUE(s.lib.path(7, path, sizeof(path)));
    EXPECT_STREQ(path, ":/Workouts/Schedule/notes.fit");
    ASSERT_TRUE(s.lib.path(5, path, sizeof(path)));
    EXPECT_STREQ(path, ":/Workouts/easy.fit");
    EXPECT_FALSE(s.lib.path(5, path, 8));  // too small
}

TEST(WorkoutLibrary, NoWorkoutForTodayMeansNoToday)
{
    Scan s;
    s.add(scheduled("2026-10-09.fit"), *program("Fri"));
    s.run(20261007);
    EXPECT_EQ(s.lib.today(), Library::kNone);
    s.run(0);  // no date known
    EXPECT_EQ(s.lib.today(), Library::kNone);
}

TEST(WorkoutLibrary, LeavesOutWhatIsNotARunningWorkout)
{
    Scan s;
    s.add(library("run.fit"), *program("Run"));
    s.add(library("generic.FIT"), *program("Generic", 100000, 1, 0));
    s.add(library("ride.fit"), *program("Ride", 100000, 1, 2));  // cycling
    s.fs.seedFile(library("broken.fit"), "not a FIT file at all");
    s.fs.seedFile(library("notes.txt"), "hello");
    s.add(library(std::string(64, 'x').append(".fit").c_str()), *program("Long name"));
    s.run(20261007);
    EXPECT_EQ(s.files(), (std::vector<std::string>{"generic.FIT", "run.fit"}));
}

TEST(WorkoutLibrary, UsesTheFileNameWhenTheWorkoutHasNone)
{
    Scan s;
    s.add(library("Hill reps.fit"), *program(""));
    s.run(20261007);
    ASSERT_EQ(s.lib.size(), 1u);
    EXPECT_STREQ(s.lib.entry(0).name, "Hill reps");
}

TEST(WorkoutLibrary, KeepsTotalsWithRepeatsExpanded)
{
    Scan s;
    s.add(library("reps.fit"), *program("Reps", 40000, 5));
    s.run(20261007);
    ASSERT_EQ(s.lib.size(), 1u);
    const Library::Entry& e = s.lib.entry(0);
    EXPECT_EQ(e.distanceCm, 5u * 40000u);
    EXPECT_EQ(e.timeMs, 5u * 60000u);
    EXPECT_EQ(e.steps, 11u);  // 5 x (run, rest) + the open cool-down
    EXPECT_EQ(e.openSteps, 1u);
    EXPECT_FALSE(e.degraded);
}

TEST(WorkoutLibrary, WhenFullThePastGoesFirstOldestFirst)
{
    Scan s;
    char name[32];
    s.add(scheduled("2026-10-07.fit"), *program("Today"));
    for (int d = 1; d <= 20; ++d) {  // 20 days to come
        std::snprintf(name, sizeof(name), "2026-11-%02d.fit", d);
        s.add(scheduled(name), *program("Coming"));
    }
    for (int i = 0; i < 30; ++i) {
        std::snprintf(name, sizeof(name), "w%02d.fit", i);
        s.add(library(name), *program(name));
    }
    for (int d = 1; d <= 19; ++d) {  // 19 days gone by
        std::snprintf(name, sizeof(name), "2026-09-%02d.fit", d);
        s.add(scheduled(name), *program("Past"));
    }
    s.run(20261007);

    ASSERT_EQ(s.lib.size(), Library::kMaxEntries);
    EXPECT_EQ(s.lib.dropped(), 70u - Library::kMaxEntries);
    // The 6 oldest past days went; the 13 most recent stayed, newest first.
    const auto files = s.files();
    EXPECT_EQ(files[0], "2026-10-07.fit");
    EXPECT_EQ(files[21], "2026-09-19.fit");
    EXPECT_EQ(files[33], "2026-09-07.fit");
    EXPECT_EQ(files[34], "w00.fit");
    for (const auto& f : files) {
        EXPECT_NE(f, "2026-09-06.fit");
        EXPECT_NE(f, "2026-09-01.fit");
    }
}

TEST(WorkoutLibrary, WhenFullOfOthersTheEndOfTheNameOrderGoes)
{
    Scan s;
    char name[32];
    for (int i = 69; i >= 0; --i) {  // listed out of order
        std::snprintf(name, sizeof(name), "w%02d.fit", i);
        s.add(library(name), *program(name));
    }
    s.add(scheduled("2026-10-07.fit"), *program("Today"));
    s.run(20261007);

    ASSERT_EQ(s.lib.size(), Library::kMaxEntries);
    const auto files = s.files();
    EXPECT_EQ(files.front(), "2026-10-07.fit");  // today is always kept
    EXPECT_EQ(files[1], "w00.fit");
    EXPECT_EQ(files.back(), "w62.fit");
}

TEST(WorkoutLibrary, NoFoldersMeansAnEmptyList)
{
    Scan s;
    s.run(20261007);
    EXPECT_EQ(s.lib.size(), 0u);
    EXPECT_EQ(s.lib.today(), Library::kNone);
}

// --- ScheduleState ---------------------------------------------------------------

TEST(ScheduleState, NothingRecordedMeansNotCompleted)
{
    InMemoryFileSystem fs;
    ScheduleState st(fs);
    st.load();
    EXPECT_FALSE(st.isCompleted(20261009, "2026-10-09.fit"));
    EXPECT_FALSE(st.completed());  // nothing started
}

TEST(ScheduleState, CompletionIsSavedAndReadBack)
{
    InMemoryFileSystem fs;
    {
        ScheduleState st(fs);
        st.load();
        ASSERT_TRUE(st.started(20261009, "2026-10-09.fit"));
        EXPECT_FALSE(st.isCompleted(20261009, "2026-10-09.fit"));
    }
    EXPECT_EQ(fs.readFile(ScheduleState::kDefaultPath),
              R"({"date":"2026-10-09","file":"2026-10-09.fit","completed":false})");
    {
        ScheduleState st(fs);
        st.load();
        EXPECT_FALSE(st.isCompleted(20261009, "2026-10-09.fit"));
        ASSERT_TRUE(st.completed());
    }
    ScheduleState st(fs);
    st.load();
    EXPECT_TRUE(st.isCompleted(20261009, "2026-10-09.fit"));
    EXPECT_FALSE(st.isCompleted(20261010, "2026-10-09.fit"));
    EXPECT_FALSE(st.isCompleted(20261009, "other.fit"));
}

TEST(ScheduleState, StartingTheSameWorkoutAgainKeepsItCompleted)
{
    InMemoryFileSystem fs;
    ScheduleState st(fs);
    st.load();
    st.started(20261009, "2026-10-09.fit");
    st.completed();
    ASSERT_TRUE(st.started(20261009, "2026-10-09.fit"));
    EXPECT_TRUE(st.isCompleted(20261009, "2026-10-09.fit"));
}

TEST(ScheduleState, StartingAnotherReplacesTheRecord)
{
    InMemoryFileSystem fs;
    ScheduleState st(fs);
    st.load();
    st.started(20261009, "2026-10-09.fit");
    st.completed();
    ASSERT_TRUE(st.started(20261010, "2026-10-10.fit"));
    EXPECT_FALSE(st.isCompleted(20261009, "2026-10-09.fit"));
    EXPECT_FALSE(st.isCompleted(20261010, "2026-10-10.fit"));
    st.completed();
    ScheduleState again(fs);
    again.load();
    EXPECT_TRUE(again.isCompleted(20261010, "2026-10-10.fit"));
}

TEST(ScheduleState, AFileThatIsNotARecordReadsAsNone)
{
    for (const char* text : {"", "{", "not json", R"({"date":"2026-02-30","file":"a.fit","completed":true})",
                             R"({"file":"a.fit","completed":true})", R"({"date":"2026-10-09","completed":true})",
                             R"({"date":"2026-10-09","file":"","completed":true})"}) {
        InMemoryFileSystem fs;
        fs.seedFile(ScheduleState::kDefaultPath, text);
        ScheduleState st(fs);
        st.load();
        EXPECT_FALSE(st.isCompleted(20261009, "a.fit")) << text;
        EXPECT_FALSE(st.completed()) << text;
    }
}

TEST(ScheduleState, ReadsARecordWrittenElsewhere)
{
    InMemoryFileSystem fs;
    fs.seedFile(ScheduleState::kDefaultPath,
                "{ \"completed\": true,\n  \"file\": \"2026-10-09.fit\", \"date\": \"2026-10-09\" }\n");
    ScheduleState st(fs);
    st.load();
    EXPECT_TRUE(st.isCompleted(20261009, "2026-10-09.fit"));
}
