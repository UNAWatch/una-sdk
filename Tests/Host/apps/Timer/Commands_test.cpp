// Pins the field-filling constructors of the Timer service<->GUI messages.
//
// These constructors are the whole send path now that SDK::send_msg forwards
// straight into them: a field left unset here is a field silently dropped on
// the wire, with nothing else to catch it.

#include <gtest/gtest.h>

// Spelled out, not "Commands.hpp": Stopwatch ships a Commands.hpp too and both
// app header dirs are on the test include path, so the bare name resolves to
// whichever CMake happens to list first.
#include "../../../../Examples/Apps/Timer/Software/Libs/Header/Commands.hpp"

#include <vector>

namespace {

using namespace CustomMessage;

std::vector<Timer> makeTimers(size_t n)
{
    std::vector<Timer> list;
    for (size_t i = 0; i < n; ++i) {
        list.push_back(Timer{ static_cast<uint16_t>(60 * (i + 1)),
                              static_cast<Timer::Effect>(i % Timer::EFFECT_COUNT) });
    }
    return list;
}

// -- Type ids ---------------------------------------------------------------

TEST(TimerCommands, EveryMessageCarriesItsOwnType)
{
    EXPECT_EQ(TimerStart{}.getType(),       TIMER_START);
    EXPECT_EQ(TimerControl{}.getType(),     TIMER_CONTROL);
    EXPECT_EQ(TimerRecentsSave{}.getType(), TIMER_RECENTS_SAVE);
    EXPECT_EQ(TimerStateMsg{}.getType(),    TIMER_STATE);
    EXPECT_EQ(TimerFired{}.getType(),       TIMER_FIRED);
    EXPECT_EQ(TimerRecents{}.getType(),     TIMER_RECENTS);
}

// The field-filling constructors delegate to the default one, so a type set
// only there would be lost if the delegation were ever dropped.
TEST(TimerCommands, FilledMessagesKeepTheirType)
{
    EXPECT_EQ(TimerStart(300, Timer::EFFECT_VIBRO).getType(), TIMER_START);
    EXPECT_EQ(TimerControl(TimerCmd::PAUSE).getType(),        TIMER_CONTROL);
    EXPECT_EQ(TimerRecentsSave(makeTimers(1)).getType(),      TIMER_RECENTS_SAVE);
    EXPECT_EQ(TimerFired(Timer{ 60, Timer::EFFECT_BEEP }, true).getType(), TIMER_FIRED);
    EXPECT_EQ(TimerRecents(makeTimers(1)).getType(),          TIMER_RECENTS);
    EXPECT_EQ(TimerStateMsg(TimerState::RUNNING, 1, 2, 3, Timer::EFFECT_BEEP).getType(),
              TIMER_STATE);
}

// -- GUI --> Service --------------------------------------------------------

TEST(TimerCommands, StartCarriesDurationAndEffect)
{
    const TimerStart msg(1234, Timer::EFFECT_VIBRO);

    EXPECT_EQ(msg.durationSec, 1234);
    EXPECT_EQ(msg.effect,      Timer::EFFECT_VIBRO);
}

TEST(TimerCommands, ControlCarriesTheSubCommand)
{
    EXPECT_EQ(TimerControl(TimerCmd::PAUSE).cmd,        TimerCmd::PAUSE);
    EXPECT_EQ(TimerControl(TimerCmd::REPLAY_ALERT).cmd, TimerCmd::REPLAY_ALERT);
}

// -- Service --> GUI --------------------------------------------------------

TEST(TimerCommands, StateCarriesTheWholeSnapshot)
{
    const TimerStateMsg msg(TimerState::PAUSED, 111u, 222u, 333, Timer::EFFECT_BEEP);

    EXPECT_EQ(msg.state,       static_cast<uint8_t>(TimerState::PAUSED));
    EXPECT_EQ(msg.endTick,     111u);
    EXPECT_EQ(msg.remainingMs, 222u);
    EXPECT_EQ(msg.durationSec, 333);
    EXPECT_EQ(msg.effect,      Timer::EFFECT_BEEP);
}

TEST(TimerCommands, FiredCarriesTheTimerAndTheBackgroundFlag)
{
    const Timer timer{ 90, Timer::EFFECT_BEEP };

    const TimerFired inApp(timer, false);
    EXPECT_EQ(inApp.durationSec, 90);
    EXPECT_EQ(inApp.effect,      Timer::EFFECT_BEEP);
    EXPECT_FALSE(inApp.background);

    EXPECT_TRUE(TimerFired(timer, true).background);
}

// -- Recents ----------------------------------------------------------------

TEST(TimerCommands, RecentsCarryEveryEntryInOrder)
{
    const std::vector<Timer> list = makeTimers(kMaxRecents);
    const TimerRecents       msg(list);

    ASSERT_EQ(msg.count, kMaxRecents);
    for (uint8_t i = 0; i < msg.count; ++i) {
        EXPECT_EQ(msg.entries[i].durationSec, list[i].durationSec) << "at " << int(i);
        EXPECT_EQ(msg.entries[i].effect,      list[i].effect)      << "at " << int(i);
    }
}

// The array is fixed so the message stays in the pool; an over-long list must
// be truncated rather than written past the end.
TEST(TimerCommands, RecentsTruncateToTheArrayCapacity)
{
    const std::vector<Timer> list = makeTimers(kMaxRecents + 5);

    EXPECT_EQ(TimerRecents(list).count,     kMaxRecents);
    EXPECT_EQ(TimerRecentsSave(list).count, kMaxRecents);
}

TEST(TimerCommands, EmptyRecentsSendAnEmptyList)
{
    const TimerRecentsSave msg{ std::vector<Timer>{} };

    EXPECT_EQ(msg.count, 0);
}

// Both directions carry the same payload; they must pack it the same way.
TEST(TimerCommands, BothRecentsDirectionsPackIdentically)
{
    const std::vector<Timer> list = makeTimers(2);
    const TimerRecents       toGui(list);
    const TimerRecentsSave   toService(list);

    ASSERT_EQ(toGui.count, toService.count);
    for (uint8_t i = 0; i < toGui.count; ++i) {
        EXPECT_EQ(toGui.entries[i].durationSec, toService.entries[i].durationSec);
        EXPECT_EQ(toGui.entries[i].effect,      toService.entries[i].effect);
    }
}

} // namespace
