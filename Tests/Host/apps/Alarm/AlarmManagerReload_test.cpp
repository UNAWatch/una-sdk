#include <gtest/gtest.h>

#include <ctime>
#include <string>
#include <vector>

#include "KernelTestDoubles.hpp"
#include "Alarm.hpp"
#include "AlarmManager.hpp"
#include "FileCrc32.hpp"

using SDK::TestSupport::KernelFixture;
using Reload = AlarmManager::Reload;

namespace {

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

// 2026-01-01 00:00:00 UTC, a Thursday; the same zero-offset convention as AlarmManager_test.cpp.
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
    i.utc = kDay0 + static_cast<std::time_t>(day) * 86400 + hour * 3600 + minute * 60 + second;
    return i;
}

const std::string kList0700 =
    R"({"alarms":[{"on":true,"time_h":7,"time_m":0,"repeat":"every_day","effect":"beep"}]})";
const std::string kList0730 =
    R"({"alarms":[{"on":true,"time_h":7,"time_m":30,"repeat":"week_days","effect":"vibro"}]})";
const std::string kOneShot0700 =
    R"({"alarms":[{"on":true,"time_h":7,"time_m":0,"repeat":"no","effect":"beep_vibro"}]})";
const std::string kEmptyList = R"({"alarms":[]})";

std::string listOf(size_t n)
{
    std::string s = R"({"alarms":[)";
    for (size_t i = 0; i < n; ++i) {
        if (i) s += ",";
        s += R"({"on":false,"time_h":)" + std::to_string(i) + R"(,"time_m":0,"repeat":"no","effect":"beep"})";
    }
    return s + "]}";
}

uint32_t crcOf(const std::string& s)
{
    return fileCrc32(s.data(), s.size());
}

struct Harness {
    KernelFixture     fx;
    RecordingCallback cb;
    AlarmManager      mgr{ fx.kernel };

    explicit Harness(const std::string& onDisk)
    {
        if (!onDisk.empty()) {
            fx.fileSystem.seedFile("alarms.json", onDisk);
        }
        mgr.attachCallback(&cb);
        mgr.load();
        cb.listChanges = 0;
    }

    void phoneWrites(const std::string& content) { fx.fileSystem.seedFile("alarms.json", content); }

    bool exists(const char* path) const { return fx.fileSystem.exist(path); }

    std::string onDisk() const { return fx.fileSystem.readFile("alarms.json"); }

    uint32_t run(const Instant& now) { return mgr.execute(now.local, now.utc); }
};

}  // namespace


TEST(FileCrc32, MatchesZlibCheckValue)
{
    EXPECT_EQ(fileCrc32("123456789", 9), 0xCBF43926u);
    EXPECT_EQ(fileCrc32("", 0), 0u);
}


// -- The signature a reload is judged against ---------------------------------

TEST(AlarmManagerReload, LoadRecordsTheCrcOfTheFileItRead)
{
    Harness h{ kList0700 };

    EXPECT_EQ(h.mgr.knownCrc(), crcOf(kList0700));
    EXPECT_EQ(h.mgr.knownSize(), kList0700.size());
}

TEST(AlarmManagerReload, SaveRecordsTheCrcOfWhatItWroteAndLeavesNoTempFile)
{
    Harness h{ "" };

    Alarm a{};
    a.on = true; a.timeHours = 6; a.timeMinutes = 45;
    a.repeat = Alarm::REPEAT_WEEKENDS; a.effect = Alarm::EFFECT_VIBRO;
    ASSERT_TRUE(h.mgr.saveAlarmList({ a }));

    EXPECT_EQ(h.mgr.knownCrc(), crcOf(h.onDisk()));
    EXPECT_FALSE(h.exists("alarms.json.save"));
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNCHANGED);
}

TEST(AlarmManagerReload, UnchangedFileIsNotReloaded)
{
    Harness h{ kList0700 };

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNCHANGED);
    EXPECT_EQ(h.cb.listChanges, 0);
}


// -- A list written from outside ----------------------------------------------

TEST(AlarmManagerReload, ExternalWriteReplacesTheListOnce)
{
    Harness h{ kList0700 };

    h.phoneWrites(kList0730);

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);
    ASSERT_EQ(h.mgr.getAlarmList().size(), 1u);
    EXPECT_EQ(h.mgr.getAlarmList()[0].timeMinutes, 30);
    EXPECT_EQ(h.mgr.getAlarmList()[0].repeat, Alarm::REPEAT_WEEK_DAYS);
    EXPECT_EQ(h.cb.listChanges, 1);

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNCHANGED);
    EXPECT_EQ(h.cb.listChanges, 1);
}

// The case the stock app cannot serve: nothing armed at boot, so its service has already exited.
TEST(AlarmManagerReload, FirstAlarmWrittenFromOutsideRings)
{
    Harness h{ kEmptyList };
    EXPECT_FALSE(h.mgr.hasActiveAlarms());

    h.phoneWrites(kList0700);
    ASSERT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);
    EXPECT_TRUE(h.mgr.hasActiveAlarms());

    h.run(at(0, 7, 0, 2));
    EXPECT_EQ(h.cb.rings, 1);
}

// The stock app's one-shot auto-save rewrote the list it loaded at boot over whatever was there.
TEST(AlarmManagerReload, OneShotAutoSaveKeepsTheExternallyWrittenList)
{
    Harness h{ kList0730 };

    h.phoneWrites(kOneShot0700);
    ASSERT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);
    h.run(at(0, 7, 0, 1));

    EXPECT_EQ(h.cb.rings, 1);
    const std::string saved = h.onDisk();
    EXPECT_NE(saved.find(R"("time_h":7,"time_m":0)"), std::string::npos) << saved;
    EXPECT_EQ(saved.find(R"("time_m":30)"), std::string::npos) << saved;
    EXPECT_NE(saved.find(R"("on":false)"), std::string::npos) << saved;
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNCHANGED);
}

TEST(AlarmManagerReload, EditingARingingAlarmFromOutsideCancelsItsSnooze)
{
    Harness h{ kList0700 };
    h.run(at(0, 7, 0, 1));
    ASSERT_EQ(h.cb.rings, 1);
    ASSERT_NE(h.fx.fileSystem.readFile("snoozes.json").find(R"("time_h":7)"), std::string::npos);

    h.phoneWrites(kList0730);
    ASSERT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);

    h.run(at(0, 7, 5, 1));
    EXPECT_EQ(h.cb.rings, 1);
    EXPECT_EQ(h.fx.fileSystem.readFile("snoozes.json").find(R"("time_h":7)"), std::string::npos);
}


// -- Writes that must not replace the list ------------------------------------

TEST(AlarmManagerReload, TornWriteKeepsTheListInMemory)
{
    Harness h{ kList0700 };

    h.phoneWrites(kList0730.substr(0, kList0730.size() / 2));

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNREADABLE);
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNREADABLE);
    ASSERT_EQ(h.mgr.getAlarmList().size(), 1u);
    EXPECT_EQ(h.mgr.getAlarmList()[0].timeMinutes, 0);
    EXPECT_TRUE(h.mgr.hasActiveAlarms());
    EXPECT_EQ(h.cb.listChanges, 0);

    h.phoneWrites(kList0730);
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);
}

TEST(AlarmManagerReload, ARecordThatFailsToParseRejectsTheWholeList)
{
    Harness h{ kList0700 };

    h.phoneWrites(R"({"alarms":[{"on":true,"time_h":7,"time_m":30,"repeat":"fortnightly","effect":"beep"}]})");

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNREADABLE);
    EXPECT_EQ(h.mgr.getAlarmList()[0].timeMinutes, 0);
}

TEST(AlarmManagerReload, MoreThanTwentyAlarmsIsRejected)
{
    Harness h{ kList0700 };

    h.phoneWrites(listOf(20));
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);

    h.phoneWrites(listOf(21));
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNREADABLE);
    EXPECT_EQ(h.mgr.getAlarmList().size(), 20u);
}

TEST(AlarmManagerReload, DeletedListKeepsTheListInMemory)
{
    Harness h{ kList0700 };

    h.fx.fileSystem.remove("alarms.json");

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::MISSING);
    EXPECT_TRUE(h.mgr.hasActiveAlarms());
}


// -- A phone's temp-file-plus-rename ------------------------------------------

TEST(AlarmManagerReload, RenameInFlightIsWaitedOutThenAdopted)
{
    Harness h{ kList0700 };

    h.fx.fileSystem.seedFile("alarms.json.tmp", kList0730);
    h.fx.fileSystem.remove("alarms.json");

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::IN_PROGRESS);
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::IN_PROGRESS);
    EXPECT_EQ(h.mgr.getAlarmList()[0].timeMinutes, 0);

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::ADOPTED);
    EXPECT_EQ(h.mgr.getAlarmList()[0].timeMinutes, 30);
    EXPECT_FALSE(h.exists("alarms.json.tmp"));
    EXPECT_EQ(h.onDisk(), kList0730);
}

TEST(AlarmManagerReload, RenameThatCompletesInTimeIsAnOrdinaryReload)
{
    Harness h{ kList0700 };

    h.fx.fileSystem.seedFile("alarms.json.tmp", kList0730);
    h.fx.fileSystem.remove("alarms.json");
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::IN_PROGRESS);

    h.fx.fileSystem.rename("alarms.json.tmp", "alarms.json");
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::RELOADED);
}

TEST(AlarmManagerReload, AbandonedPartialTempFileIsDiscarded)
{
    Harness h{ kList0700 };

    h.fx.fileSystem.seedFile("alarms.json.tmp", kList0730.substr(0, 20));
    h.fx.fileSystem.remove("alarms.json");

    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::IN_PROGRESS);
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::IN_PROGRESS);
    EXPECT_EQ(h.mgr.reloadIfChanged(), Reload::UNREADABLE);
    EXPECT_FALSE(h.exists("alarms.json.tmp"));
    EXPECT_TRUE(h.mgr.hasActiveAlarms());
}

TEST(AlarmManagerReload, LoadCompletesARenameAResetInterrupted)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("alarms.json.tmp", kList0730);

    AlarmManager mgr{ fx.kernel };
    mgr.load();

    ASSERT_EQ(mgr.getAlarmList().size(), 1u);
    EXPECT_EQ(mgr.getAlarmList()[0].timeMinutes, 30);
    EXPECT_FALSE(fx.fileSystem.exist("alarms.json.tmp"));
    EXPECT_EQ(mgr.knownCrc(), crcOf(kList0730));
}

TEST(AlarmManagerReload, LoadPrefersTheWatchesOwnInterruptedSave)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("alarms.json.save", kList0700);
    fx.fileSystem.seedFile("alarms.json.tmp", kList0730);

    AlarmManager mgr{ fx.kernel };
    mgr.load();

    EXPECT_EQ(mgr.getAlarmList()[0].timeMinutes, 0);
}


// -- The bytes a phone must reproduce for its CRC to match a watch save ---------

// Tools/test_alarm_codec.py asserts the phone codec emits this same string.
TEST(AlarmManagerReload, SaveWritesCompactJsonInFieldOrder)
{
    Harness h{ "" };

    Alarm a{};
    a.on = true; a.timeHours = 7; a.timeMinutes = 30;
    a.repeat = Alarm::REPEAT_WEEK_DAYS; a.effect = Alarm::EFFECT_BEEP_AND_VIBRO;
    ASSERT_TRUE(h.mgr.saveAlarmList({ a }));

    EXPECT_EQ(h.onDisk(),
        R"({"alarms":[{"on":true,"time_h":7,"time_m":30,"repeat":"week_days","effect":"beep_vibro"}]})");
}


// -- Telling a phone that this build reloads ----------------------------------

TEST(AlarmManagerReload, LoadWritesTheSyncMarker)
{
    Harness h{ kList0700 };

    EXPECT_EQ(h.fx.fileSystem.readFile("alarm_sync.json"), R"({"version":1})");
}

TEST(AlarmManagerReload, LoadLeavesAnExistingSyncMarkerAlone)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("alarm_sync.json", R"({"version":2})");
    AlarmManager mgr{ fx.kernel };
    mgr.load();

    EXPECT_EQ(fx.fileSystem.readFile("alarm_sync.json"), R"({"version":2})");
}
