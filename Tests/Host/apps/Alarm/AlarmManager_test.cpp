#include <gtest/gtest.h>

#include <ctime>
#include <string>
#include <vector>

#include "KernelTestDoubles.hpp"
#include "Alarm.hpp"
#include "AlarmManager.hpp"

using SDK::TestSupport::KernelFixture;

namespace {

// Records the observer callbacks so tests can assert on rings and list changes.
class RecordingCallback : public AlarmManager::AlarmCallback {
public:
    void onAlarm(const Alarm& alarm) override
    {
        ++rings;
        lastAlarm = alarm;
    }

    void onListChanged(const std::vector<Alarm>& list) override
    {
        ++listChanges;
        lastList = list;
    }

    int   rings       = 0;
    int   listChanges = 0;
    Alarm lastAlarm{};
    std::vector<Alarm> lastList;
};

// Day 0 is 2026-01-01 00:00:00 UTC, a Thursday (tm_wday 4). The tests run in a
// zero-offset zone: the local wall clock and the UTC stamp advance together,
// which is all AlarmManager requires of the pair.
constexpr std::time_t kDay0     = 1767225600;
constexpr int         kDay0Wday = 4;

struct Instant {
    std::tm     local{};
    std::time_t utc = 0;
};

Instant at(int day, int hour, int minute, int second = 0)
{
    Instant i;
    i.local.tm_hour = hour;
    i.local.tm_min  = minute;
    i.local.tm_sec  = second;
    i.local.tm_wday = (kDay0Wday + day) % 7;
    i.utc = kDay0
          + static_cast<std::time_t>(day) * 86400
          + static_cast<std::time_t>(hour) * 3600
          + static_cast<std::time_t>(minute) * 60
          + second;
    return i;
}

Alarm alarmAt(uint8_t hour, uint8_t minute,
              Alarm::Repeat repeat = Alarm::REPEAT_EVERY_DAY,
              bool on = true,
              Alarm::Effect effect = Alarm::EFFECT_BEEP)
{
    Alarm alarm{};
    alarm.on          = on;
    alarm.timeHours   = hour;
    alarm.timeMinutes = minute;
    alarm.repeat      = repeat;
    alarm.effect      = effect;
    return alarm;
}

// A manager with one alarm already saved and an observer attached.
struct Harness {
    KernelFixture     fx;
    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };

    explicit Harness(const Alarm& alarm)
    {
        mgr.attachCallback(&cb);
        EXPECT_TRUE(mgr.saveAlarmList({ alarm }));
        cb.rings = 0;
    }

    uint32_t run(const Instant& now) { return mgr.execute(now.local, now.utc); }
};

}  // namespace


// -- Ringing, and ringing only once -------------------------------------------

TEST(AlarmManager, RingsWhenItsMinuteArrives)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 6, 59, 30));
    EXPECT_EQ(h.cb.rings, 0);

    h.run(at(0, 7, 0, 1));
    EXPECT_EQ(h.cb.rings, 1);
    EXPECT_EQ(h.cb.lastAlarm.timeHours, 7);
    EXPECT_EQ(h.cb.lastAlarm.timeMinutes, 0);
    EXPECT_TRUE(h.cb.lastAlarm.on);
}

// execute() is called again for every message the service receives, and a
// resident service is handed events it never asked for (a glance tick at 1 Hz,
// for one). Repeated calls inside the minute must not ring repeatedly.
TEST(AlarmManager, RingsOnlyOnceHoweverOftenExecuteIsCalled)
{
    Harness h{ alarmAt(7, 0) };

    for (int second = 0; second < 60; ++second) {
        h.run(at(0, 7, 0, second));
    }

    EXPECT_EQ(h.cb.rings, 1);
}

TEST(AlarmManager, WeekdayAlarmStaysSilentAtTheWeekend)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_WEEK_DAYS) };

    h.run(at(2, 7, 0, 5));      // day 2 == Saturday
    EXPECT_EQ(h.cb.rings, 0);

    h.run(at(4, 7, 0, 5));      // day 4 == Monday
    EXPECT_EQ(h.cb.rings, 1);
}


// -- Defect 1: Stop inside the firing minute re-armed the alarm ---------------

TEST(AlarmManager, StopInsideTheFiringMinuteDoesNotRingAgain)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    // What the ringing screen's Stop sends, and what the service does with it.
    h.mgr.disableAllActiveAlarm();

    // The service loops straight back into execute() with the wall clock still
    // inside the alarm's minute. This used to ring again and leave a fresh
    // snooze behind, so Stop behaved as Snooze.
    h.run(at(0, 7, 0, 3));
    h.run(at(0, 7, 0, 30));
    h.run(at(0, 7, 0, 59));

    EXPECT_EQ(h.cb.rings, 1);
}

TEST(AlarmManager, StopInsideTheFiringMinuteLeavesNoSnoozeFiveMinutesLater)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    h.mgr.disableAllActiveAlarm();
    h.run(at(0, 7, 0, 3));

    h.run(at(0, 7, 5, 0));
    h.run(at(0, 7, 10, 0));

    EXPECT_EQ(h.cb.rings, 1);
}

// The per-alarm Stop path clears the same tracking list, so it had the same bug.
TEST(AlarmManager, StopOneAlarmInsideTheFiringMinuteDoesNotRingAgain)
{
    const Alarm alarm = alarmAt(7, 0);
    Harness h{ alarm };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    h.mgr.disableAlarm(alarm);
    h.run(at(0, 7, 0, 20));

    EXPECT_EQ(h.cb.rings, 1);
}

// The latch is a one-minute affair: tomorrow the alarm must ring as usual.
TEST(AlarmManager, StoppingTodayDoesNotSuppressTomorrow)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    h.mgr.disableAllActiveAlarm();
    h.run(at(0, 7, 0, 3));
    ASSERT_EQ(h.cb.rings, 1);

    h.run(at(1, 7, 0, 2));
    EXPECT_EQ(h.cb.rings, 2);
}


// -- Defect 2: snooze re-trigger matched on exact hh:mm -----------------------

TEST(AlarmManager, SnoozedAlarmRingsAgainFiveMinutesLater)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    h.run(at(0, 7, 4, 0));
    EXPECT_EQ(h.cb.rings, 1);

    h.run(at(0, 7, 5, 1));
    EXPECT_EQ(h.cb.rings, 2);
}

// Miss the one minute the deadline named -- a skipped wake-up, a clock nudge --
// and the entry used to sit there for ever, never ringing and never erased,
// with isSnoozed() suppressing the alarm on every following day.
TEST(AlarmManager, SnoozeStillRingsWhenItsExactMinuteIsMissed)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    h.run(at(0, 7, 7, 30));     // 07:05 never observed
    EXPECT_EQ(h.cb.rings, 2);
}

TEST(AlarmManager, ASnoozeMissedByHoursIsDroppedAndTheAlarmKeepsWorking)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    // Hours later: ringing now would ring at the wrong time.
    h.run(at(0, 11, 0, 0));
    EXPECT_EQ(h.cb.rings, 1);
    EXPECT_EQ(h.mgr.execute(at(0, 11, 0, 1).local, at(0, 11, 0, 1).utc), 59000u);

    // ... and the entry is gone, so tomorrow is not suppressed.
    h.run(at(1, 7, 0, 2));
    EXPECT_EQ(h.cb.rings, 2);
}

// A snooze that is already past saving must not suppress the alarm it came
// from. The alarm scan runs before the snooze sweep, so a spent entry that the
// sweep is about to drop was still enough to skip the alarm for that minute --
// and the alarm's minute does not come round again until the next day.
TEST(AlarmManager, ASpentSnoozeDoesNotSwallowItsAlarmsOwnMinute)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    // A day later, inside the alarm's own minute, with the snooze from
    // yesterday still on the books and hours past its deadline.
    h.run(at(1, 7, 0, 3));
    EXPECT_EQ(h.cb.rings, 2);
}

// The same hole, reached the way a user would: the watch is off over the
// snooze's deadline and comes back inside the alarm's minute the next day.
TEST(AlarmManager, ASnoozeRestoredStaleDoesNotSwallowTheAlarm)
{
    KernelFixture fx;

    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0) }));
        mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
        ASSERT_EQ(cb.rings, 1);
    }

    RecordingCallback cb2;
    AlarmManager      mgr2{ fx.kernel };
    mgr2.attachCallback(&cb2);
    mgr2.load();

    mgr2.execute(at(1, 7, 0, 5).local, at(1, 7, 0, 5).utc);
    EXPECT_EQ(cb2.rings, 1);
}

// A due snooze and an unrelated alarm landing in the same minute both ring.
TEST(AlarmManager, ASnoozeAndAnotherAlarmInTheSameMinuteBothRing)
{
    KernelFixture     fx;
    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };
    mgr.attachCallback(&cb);
    ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0), alarmAt(7, 5) }));

    mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
    ASSERT_EQ(cb.rings, 1);

    // 07:05 is both the second alarm's time and the first one's snooze deadline.
    mgr.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb.rings, 3);
}

TEST(AlarmManager, AClockStepBackwardsReanchorsTheSnooze)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    // The clock is set back an hour mid-snooze. The deadline is now unreachable
    // for an hour; re-anchoring keeps the snooze to its five minutes.
    h.run(at(0, 6, 0, 2));
    EXPECT_EQ(h.cb.rings, 1);

    h.run(at(0, 6, 5, 3));
    EXPECT_EQ(h.cb.rings, 2);
}

TEST(AlarmManager, SnoozeRingsFourTimesThenGivesUp)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_NO) };

    h.run(at(0, 7, 0, 2));
    for (int minute = 5; minute <= 30; minute += 5) {
        h.run(at(0, 7, minute, 2));
    }

    EXPECT_EQ(h.cb.rings, 5);                   // the alarm, then four re-rings
    // The one-time alarm switched itself off and the snooze is spent, so there
    // is nothing left for the service to stay awake for.
    EXPECT_FALSE(h.mgr.hasActiveAlarms());
}


// -- Defect 3: a one-off alarm's snooze was orphaned by its own trigger -------

TEST(AlarmManager, OneShotAlarmDisablesItselfWhenItRings)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_NO) };

    h.run(at(0, 7, 0, 2));

    ASSERT_EQ(h.mgr.getAlarmList().size(), 1u);
    EXPECT_FALSE(h.mgr.getAlarmList()[0].on);
}

// The trigger clears `on`, and removeObsoleteSnoozedAlarms() drops snoozes for
// alarms that are off -- so any list save inside the snooze window silently
// cancelled the snooze the user had just asked for.
TEST(AlarmManager, OneShotSnoozeSurvivesAListSave)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_NO) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    // The user toggles or edits something in the app while the snooze is pending.
    std::vector<Alarm> list = h.mgr.getAlarmList();
    list.push_back(alarmAt(9, 30, Alarm::REPEAT_NO, false));
    ASSERT_TRUE(h.mgr.saveAlarmList(list));

    h.run(at(0, 7, 5, 1));
    EXPECT_EQ(h.cb.rings, 2);
}

TEST(AlarmManager, OneShotSnoozeSurvivesStoppingAnotherAlarm)
{
    const Alarm oneShot = alarmAt(7, 0, Alarm::REPEAT_NO);
    const Alarm other   = alarmAt(9, 30);

    KernelFixture     fx;
    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };
    mgr.attachCallback(&cb);
    ASSERT_TRUE(mgr.saveAlarmList({ oneShot, other }));

    mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
    ASSERT_EQ(cb.rings, 1);

    mgr.disableAlarm(other);

    mgr.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb.rings, 2);
}

// A re-trigger reaches the GUI as an ActivatedAlarm, and the GUI reads `on` as
// "an alarm is ringing" -- so a one-shot's re-ring must carry it set even
// though the alarm itself is now off.
TEST(AlarmManager, AReTriggerCarriesTheRingingFlag)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_NO, true, Alarm::EFFECT_VIBRO) };

    h.run(at(0, 7, 0, 2));
    h.run(at(0, 7, 5, 1));

    ASSERT_EQ(h.cb.rings, 2);
    EXPECT_TRUE(h.cb.lastAlarm.on);
    EXPECT_EQ(h.cb.lastAlarm.timeHours, 7);
    EXPECT_EQ(h.cb.lastAlarm.effect, Alarm::EFFECT_VIBRO);
}

// The user's own decision to switch an alarm off still cancels its snooze.
TEST(AlarmManager, SwitchingAnAlarmOffCancelsItsSnooze)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_EQ(h.cb.rings, 1);

    std::vector<Alarm> list = h.mgr.getAlarmList();
    list[0].on = false;
    ASSERT_TRUE(h.mgr.saveAlarmList(list));

    h.run(at(0, 7, 5, 1));
    EXPECT_EQ(h.cb.rings, 1);
    EXPECT_FALSE(h.mgr.hasActiveAlarms());
}

TEST(AlarmManager, DeletingAnAlarmCancelsItsSnooze)
{
    Harness h{ alarmAt(7, 0) };

    h.run(at(0, 7, 0, 2));
    ASSERT_TRUE(h.mgr.saveAlarmList({}));

    h.run(at(0, 7, 5, 1));
    EXPECT_EQ(h.cb.rings, 1);
}


// -- Defect 4: pending snoozes were RAM-only ---------------------------------

TEST(AlarmManager, APendingSnoozeSurvivesARestart)
{
    KernelFixture fx;

    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0) }));

        mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
        ASSERT_EQ(cb.rings, 1);
    }

    // Reboot / OTA / shutdown: no COMMAND_APP_STOP, no chance to save on the
    // way out. The service comes back and the snooze is still owed.
    RecordingCallback cb2;
    AlarmManager      mgr2{ fx.kernel };
    mgr2.attachCallback(&cb2);
    mgr2.load();

    EXPECT_TRUE(mgr2.hasActiveAlarms());

    mgr2.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb2.rings, 1);
    EXPECT_EQ(cb2.lastAlarm.timeHours, 7);
    EXPECT_EQ(cb2.lastAlarm.timeMinutes, 0);
    EXPECT_TRUE(cb2.lastAlarm.on);
}

TEST(AlarmManager, AOneShotSnoozeSurvivesARestartEvenThoughItsAlarmIsOff)
{
    KernelFixture fx;

    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0, Alarm::REPEAT_NO) }));
        mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
        ASSERT_FALSE(mgr.getAlarmList()[0].on);
    }

    RecordingCallback cb2;
    AlarmManager      mgr2{ fx.kernel };
    mgr2.attachCallback(&cb2);
    mgr2.load();

    mgr2.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb2.rings, 1);
}

TEST(AlarmManager, ARestoredSnoozeIsDroppedWhenItsAlarmIsGone)
{
    KernelFixture fx;

    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0) }));
        mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
    }

    // The alarm file is rewritten without that alarm (deleted from another
    // session, or a factory-reset list).
    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(9, 30) }));
    }

    RecordingCallback cb2;
    AlarmManager      mgr2{ fx.kernel };
    mgr2.attachCallback(&cb2);
    mgr2.load();

    mgr2.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb2.rings, 0);
}

TEST(AlarmManager, AStoppedSnoozeIsNotResurrectedByARestart)
{
    KernelFixture fx;

    {
        RecordingCallback cb;
        AlarmManager      mgr{ fx.kernel };
        mgr.attachCallback(&cb);
        ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0) }));
        mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
        mgr.disableAllActiveAlarm();
    }

    RecordingCallback cb2;
    AlarmManager      mgr2{ fx.kernel };
    mgr2.attachCallback(&cb2);
    mgr2.load();

    mgr2.execute(at(0, 7, 5, 1).local, at(0, 7, 5, 1).utc);
    EXPECT_EQ(cb2.rings, 0);
}

TEST(AlarmManager, CorruptSnoozeStorageIsIgnored)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("snoozes.json", "{\"snoozes\": [ this is not json ");

    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };
    mgr.attachCallback(&cb);
    ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(7, 0) }));
    mgr.load();

    EXPECT_EQ(cb.rings, 0);

    mgr.execute(at(0, 7, 0, 2).local, at(0, 7, 0, 2).utc);
    EXPECT_EQ(cb.rings, 1);
}


// -- Defect 5: an idle service asked to be woken every minute for ever -------

TEST(AlarmManager, AsksForNoWakeUpWhenNothingIsArmed)
{
    KernelFixture     fx;
    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };
    mgr.attachCallback(&cb);

    EXPECT_EQ(mgr.execute(at(0, 7, 0, 0).local, at(0, 7, 0, 0).utc),
              AlarmManager::kNoWork);

    ASSERT_TRUE(mgr.saveAlarmList({ alarmAt(9, 30, Alarm::REPEAT_EVERY_DAY, false) }));
    EXPECT_EQ(mgr.execute(at(0, 7, 0, 0).local, at(0, 7, 0, 0).utc),
              AlarmManager::kNoWork);
}

TEST(AlarmManager, AsksForTheNextMinuteWhileAnAlarmIsArmed)
{
    Harness h{ alarmAt(7, 0) };

    EXPECT_EQ(h.run(at(0, 6, 0, 0)), 60000u);
    EXPECT_EQ(h.run(at(0, 6, 0, 45)), 15000u);
    EXPECT_EQ(h.run(at(0, 6, 0, 59)), 1000u);
}

TEST(AlarmManager, NeverAsksForAZeroLengthWait)
{
    Harness h{ alarmAt(7, 0) };

    Instant leap = at(0, 6, 59, 59);
    leap.local.tm_sec = 60;             // a leap second, as localtime may report it

    EXPECT_GT(h.run(leap), 0u);
}

TEST(AlarmManager, StopsAskingForWakeUpsOnceTheLastAlarmIsStopped)
{
    Harness h{ alarmAt(7, 0, Alarm::REPEAT_NO) };

    h.run(at(0, 7, 0, 2));
    h.mgr.disableAllActiveAlarm();

    EXPECT_EQ(h.run(at(0, 7, 0, 3)), AlarmManager::kNoWork);
}
