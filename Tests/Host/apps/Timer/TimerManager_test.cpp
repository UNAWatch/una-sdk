#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "KernelTestDoubles.hpp"
#include "Timer.hpp"
#include "TimerManager.hpp"

using SDK::TestSupport::KernelFixture;

namespace {

// Records the observer callbacks so tests can assert on fires and recents.
class RecordingCallback : public TimerManager::Callback {
public:
    void onFired(const Timer& timer) override
    {
        fired = true;
        firedTimer = timer;
        ++fireCount;
    }
    void onRecentsChanged(const std::vector<Timer>& list) override
    {
        ++recentsChanges;
        lastRecents = list;
    }

    bool  fired = false;
    Timer firedTimer{0, Timer::EFFECT_BEEP};
    int   fireCount = 0;
    int   recentsChanges = 0;
    std::vector<Timer> lastRecents;
};

// TimerManager returns this (private kNoTimeout) from execute() when nothing is
// counting; mirror it here for the assertions.
constexpr uint32_t kNoTimeout = 0xFFFFFFFFu;

}  // namespace

// -- State machine ------------------------------------------------------------

TEST(TimerManager, StartsIdle)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    EXPECT_EQ(tm.getState().state, TimerState::IDLE);
    EXPECT_FALSE(tm.hasActiveTimers());
}

TEST(TimerManager, StartArmsRunning)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);

    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::RUNNING);
    EXPECT_EQ(s.endTick, 1000u + 60000u);
    EXPECT_EQ(s.remainingMs, 60000u);
    EXPECT_EQ(s.durationSec, 60u);
    EXPECT_EQ(s.effect, Timer::EFFECT_BEEP);
    EXPECT_TRUE(tm.hasActiveTimers());
}

TEST(TimerManager, PauseFreezesRemainingAndIgnoresNonRunning)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_VIBRO, 1000);
    tm.pause(31000);                       // 30 s elapsed

    EXPECT_EQ(tm.getState().state, TimerState::PAUSED);
    EXPECT_EQ(tm.getState().remainingMs, 30000u);

    // pause() acts only on a running timer -- a second pause is a no-op.
    tm.pause(50000);
    EXPECT_EQ(tm.getState().remainingMs, 30000u);
}

TEST(TimerManager, ResumeReArmsFromRemaining)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);
    tm.pause(31000);                       // 30 s left
    tm.resume(50000);

    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::RUNNING);
    EXPECT_EQ(s.endTick, 50000u + 30000u);
}

TEST(TimerManager, ResetRunningReArmsFromTop)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);
    tm.reset(20000);

    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::RUNNING);
    EXPECT_EQ(s.endTick, 20000u + 60000u);
    EXPECT_EQ(s.remainingMs, 60000u);
}

TEST(TimerManager, ResetNeverStartsAStoppedTimer)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);
    tm.pause(31000);                       // PAUSED, 30 s left
    tm.reset(40000);                       // reload full duration, stay paused

    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::PAUSED);
    EXPECT_EQ(s.remainingMs, 60000u);
}

TEST(TimerManager, StopGoesIdle)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);
    tm.stop();

    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::IDLE);
    EXPECT_EQ(s.remainingMs, 0u);
    EXPECT_FALSE(tm.hasActiveTimers());
}

TEST(TimerManager, RepeatHoldsPausedAtFullDuration)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);
    RecordingCallback cb;
    tm.attachCallback(&cb);

    tm.start(10, Timer::EFFECT_BEEP, 1000);
    tm.execute(11000);                     // fire
    ASSERT_EQ(tm.getState().state, TimerState::FIRED);

    tm.repeat(20000);

    // repeat() re-arms at the full duration but stays PAUSED: the user resumes
    // it from the Running screen (it does NOT start counting on its own).
    const auto s = tm.getState();
    EXPECT_EQ(s.state, TimerState::PAUSED);
    EXPECT_EQ(s.remainingMs, 10000u);
}

// -- execute() ----------------------------------------------------------------

TEST(TimerManager, ExecuteReturnsRemainingWhileRunning)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.start(60, Timer::EFFECT_BEEP, 1000);

    EXPECT_EQ(tm.execute(31000), 30000u);
    EXPECT_EQ(tm.getState().state, TimerState::RUNNING);
}

TEST(TimerManager, ExecuteFiresAtExpiry)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);
    RecordingCallback cb;
    tm.attachCallback(&cb);

    tm.start(10, Timer::EFFECT_VIBRO, 1000);
    EXPECT_EQ(tm.execute(11000), kNoTimeout);

    EXPECT_EQ(tm.getState().state, TimerState::FIRED);
    EXPECT_EQ(tm.getState().remainingMs, 0u);
    EXPECT_TRUE(cb.fired);
    EXPECT_EQ(cb.fireCount, 1);
    EXPECT_EQ(cb.firedTimer.durationSec, 10u);
    EXPECT_EQ(cb.firedTimer.effect, Timer::EFFECT_VIBRO);
}

TEST(TimerManager, ExecuteIsNoopWhenNotRunning)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    EXPECT_EQ(tm.execute(5000), kNoTimeout);           // IDLE
    EXPECT_EQ(tm.getState().state, TimerState::IDLE);

    tm.start(60, Timer::EFFECT_BEEP, 1000);
    tm.pause(2000);
    EXPECT_EQ(tm.execute(9000), kNoTimeout);           // PAUSED
    EXPECT_EQ(tm.getState().state, TimerState::PAUSED);
}

TEST(TimerManager, ExecuteFiresAcrossUint32Wrap)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);
    RecordingCallback cb;
    tm.attachCallback(&cb);

    // Start ~500 ms before the uint32 tick wraps, so the 2 s deadline lands
    // just past the wrap and endTick < start in raw uint32 terms.
    const uint32_t start = 0xFFFFFFFFu - 500u;
    tm.start(2, Timer::EFFECT_BEEP, start);

    const uint32_t endTick = tm.getState().endTick;    // start + 2000, wrapped low
    ASSERT_LT(endTick, start);                          // confirm it wrapped

    // Just before the (wrapped) deadline: still counting.
    EXPECT_EQ(tm.execute(endTick - 100u), 100u);
    EXPECT_EQ(tm.getState().state, TimerState::RUNNING);

    // Just after: fires -- despite now being numerically far below start.
    EXPECT_EQ(tm.execute(endTick + 50u), kNoTimeout);
    EXPECT_EQ(tm.getState().state, TimerState::FIRED);
    EXPECT_TRUE(cb.fired);
}

// -- Recents (persisted through the in-memory filesystem) ---------------------

TEST(TimerManager, SaveRecentsCapsToMax)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);
    RecordingCallback cb;
    tm.attachCallback(&cb);

    const std::vector<Timer> five = {
        {60,  Timer::EFFECT_BEEP},
        {120, Timer::EFFECT_VIBRO},
        {180, Timer::EFFECT_BEEP},
        {240, Timer::EFFECT_BEEP},
        {300, Timer::EFFECT_BEEP},
    };
    EXPECT_TRUE(tm.saveRecents(five));

    ASSERT_EQ(tm.getRecents().size(), Timer::kMaxRecents);   // capped to 3
    EXPECT_EQ(tm.getRecents()[0].durationSec, 60u);          // keeps the first N
    EXPECT_EQ(tm.getRecents()[2].durationSec, 180u);
    ASSERT_EQ(cb.lastRecents.size(), Timer::kMaxRecents);
    EXPECT_EQ(cb.recentsChanges, 1);
}

TEST(TimerManager, RecentsRoundTripThroughStorage)
{
    KernelFixture fx;

    {
        TimerManager writer(fx.kernel);
        writer.saveRecents({ {90, Timer::EFFECT_VIBRO}, {600, Timer::EFFECT_BEEP} });
    }

    TimerManager reader(fx.kernel);        // same kernel -> same in-memory FS
    RecordingCallback cb;
    reader.attachCallback(&cb);
    reader.load();

    ASSERT_EQ(reader.getRecents().size(), 2u);
    EXPECT_EQ(reader.getRecents()[0].durationSec, 90u);
    EXPECT_EQ(reader.getRecents()[0].effect, Timer::EFFECT_VIBRO);
    EXPECT_EQ(reader.getRecents()[1].durationSec, 600u);
    EXPECT_EQ(reader.getRecents()[1].effect, Timer::EFFECT_BEEP);
    EXPECT_EQ(cb.recentsChanges, 1);       // load notifies once
}

TEST(TimerManager, RecentsWorstCaseRoundTrip)
{
    KernelFixture fx;
    // kMaxRecents entries at the 4-digit max duration and the longest effect --
    // the largest file the writer produces; it must still fit the scratch buffer.
    const std::vector<Timer> maxed(
        Timer::kMaxRecents,
        Timer{Timer::kMaxDurationSec, Timer::EFFECT_BEEP_AND_VIBRO});

    {
        TimerManager writer(fx.kernel);
        ASSERT_TRUE(writer.saveRecents(maxed));
    }

    TimerManager reader(fx.kernel);
    reader.load();

    ASSERT_EQ(reader.getRecents().size(), Timer::kMaxRecents);
    EXPECT_EQ(reader.getRecents()[0].durationSec, Timer::kMaxDurationSec);
    EXPECT_EQ(reader.getRecents()[Timer::kMaxRecents - 1].effect,
              Timer::EFFECT_BEEP_AND_VIBRO);
}

TEST(TimerManager, LoadWithNoFileYieldsEmpty)
{
    KernelFixture fx;
    TimerManager tm(fx.kernel);

    tm.load();                             // nothing seeded

    EXPECT_TRUE(tm.getRecents().empty());
}

TEST(TimerManager, LoadMalformedFileYieldsEmpty)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("timer.json", "this is not json {");
    TimerManager tm(fx.kernel);

    tm.load();

    EXPECT_TRUE(tm.getRecents().empty());
}

TEST(TimerManager, LoadKeepsValidEntriesAndSkipsInvalid)
{
    KernelFixture fx;
    // The second entry's duration exceeds kMaxDurationSec, so it is rejected
    // while the first, valid entry is kept.
    fx.fileSystem.seedFile(
        "timer.json",
        R"({"recents":[{"sec":60,"effect":"beep"},{"sec":9999999,"effect":"beep"}]})");
    TimerManager tm(fx.kernel);

    tm.load();

    ASSERT_EQ(tm.getRecents().size(), 1u);
    EXPECT_EQ(tm.getRecents()[0].durationSec, 60u);
    EXPECT_EQ(tm.getRecents()[0].effect, Timer::EFFECT_BEEP);
}

TEST(TimerManager, LoadAllEntriesInvalidYieldsEmpty)
{
    KernelFixture fx;
    // Valid JSON with a recents array, but every entry is unusable (bad duration,
    // then bad effect) -- the list ends up empty.
    fx.fileSystem.seedFile(
        "timer.json",
        R"({"recents":[{"sec":9999999,"effect":"beep"},{"sec":30,"effect":"nope"}]})");
    TimerManager tm(fx.kernel);

    tm.load();

    EXPECT_TRUE(tm.getRecents().empty());
}

TEST(TimerManager, LoadsHandFormattedFile)
{
    KernelFixture fx;
    // A user could pretty-print the file over USB: 4-space indent, one field per
    // line, CRLF newlines (Windows). It must still load and fit the scratch buffer.
    const std::string pretty =
        "{\r\n"
        "    \"recents\": [\r\n"
        "        {\r\n"
        "            \"sec\": 5999,\r\n"
        "            \"effect\": \"beep_vibro\"\r\n"
        "        },\r\n"
        "        {\r\n"
        "            \"sec\": 3000,\r\n"
        "            \"effect\": \"vibro\"\r\n"
        "        },\r\n"
        "        {\r\n"
        "            \"sec\": 60,\r\n"
        "            \"effect\": \"beep\"\r\n"
        "        }\r\n"
        "    ]\r\n"
        "}\r\n";
    fx.fileSystem.seedFile("timer.json", pretty);
    TimerManager tm(fx.kernel);

    tm.load();

    ASSERT_EQ(tm.getRecents().size(), 3u);
    EXPECT_EQ(tm.getRecents()[0].durationSec, 5999u);
    EXPECT_EQ(tm.getRecents()[0].effect, Timer::EFFECT_BEEP_AND_VIBRO);
    EXPECT_EQ(tm.getRecents()[1].durationSec, 3000u);
    EXPECT_EQ(tm.getRecents()[1].effect, Timer::EFFECT_VIBRO);
    EXPECT_EQ(tm.getRecents()[2].durationSec, 60u);
    EXPECT_EQ(tm.getRecents()[2].effect, Timer::EFFECT_BEEP);
}
