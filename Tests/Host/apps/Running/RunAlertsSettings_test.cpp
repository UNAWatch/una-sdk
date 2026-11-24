// Settings > Run Alerts: the settings fields and their serialization.
// The file stays version 1; the run_alerts keys are additive, and a file from
// before them loads the defaults.

#include <gtest/gtest.h>

#include "KernelTestDoubles.hpp"
#include "Settings.hpp"
#include "SettingsSerializer.hpp"

namespace {

using SDK::TestSupport::KernelFixture;

// Values that differ from every default.
Settings::RunAlerts nonDefaultRunAlerts()
{
    Settings::RunAlerts ra;
    ra.runWalkEn        = true;
    ra.runWalkRunSec    = 840;    // 14:00
    ra.runWalkWalkSec   = 120;    // 2:00
    ra.paceFastSecPerKm = 270.0f; // 4:30 /km
    ra.paceSlowSecPerKm = 330.0f; // 5:30 /km
    ra.hrHighBpm        = 165;
    ra.hrLowBpm         = 120;
    ra.backInTap        = true;
    return ra;
}

void expectDefaults(const Settings::RunAlerts& ra)
{
    EXPECT_FALSE(ra.runWalkEn);
    EXPECT_EQ(ra.runWalkRunSec, 240u);
    EXPECT_EQ(ra.runWalkWalkSec, 60u);
    EXPECT_EQ(ra.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(ra.paceSlowSecPerKm, 0.0f);
    EXPECT_EQ(ra.hrHighBpm, 0);
    EXPECT_EQ(ra.hrLowBpm, 0);
    EXPECT_FALSE(ra.backInTap);
}

TEST(RunAlertsSettings, DefaultsAreOffWithRunFourWalkOne)
{
    Settings settings {};
    expectDefaults(settings.runAlerts);
}

TEST(RunAlertsSettings, SaveLoadRoundTripsEveryField)
{
    KernelFixture fixture;
    Settings saved {};
    saved.runAlerts = nonDefaultRunAlerts();

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.save(saved));

    Settings loaded {};
    ASSERT_TRUE(serializer.load(loaded));

    const Settings::RunAlerts& ra = loaded.runAlerts;
    EXPECT_TRUE(ra.runWalkEn);
    EXPECT_EQ(ra.runWalkRunSec, 840u);
    EXPECT_EQ(ra.runWalkWalkSec, 120u);
    EXPECT_EQ(ra.paceFastSecPerKm, 270.0f);
    EXPECT_EQ(ra.paceSlowSecPerKm, 330.0f);
    EXPECT_EQ(ra.hrHighBpm, 165);
    EXPECT_EQ(ra.hrLowBpm, 120);
    EXPECT_TRUE(ra.backInTap);
}

TEST(RunAlertsSettings, OldFileWithoutRunAlertsLoadsDefaults)
{
    // A settings.json from before the feature, loaded into a struct that holds
    // other values: the defaults, not what the struct held.
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "auto_pause_en": false,
        "phone_notif_en": true,
        "calib_trace_en": false,
        "alert_distance_id": 1,
        "alert_time_id": 0,
        "intervals": {
            "repeats_num": 4,
            "run_metric": "TIME",
            "run_time": 180,
            "run_distance": 0,
            "rest_metric": "TIME",
            "rest_time": 60,
            "rest_distance": 0,
            "warm_up": true,
            "cool_down": true,
            "last_rest": true
        }
    })");

    Settings settings {};
    settings.runAlerts = nonDefaultRunAlerts();

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    expectDefaults(settings.runAlerts);
}

TEST(RunAlertsSettings, OutOfRangeValuesFallBack)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": {
            "run_walk_en": true,
            "run_walk_run_time": 0,
            "run_walk_walk_time": 3601,
            "pace_fast": 59,
            "pace_slow": 1801,
            "hr_high": 241,
            "hr_low": 29,
            "back_in_tap": true
        }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    const Settings::RunAlerts& ra = settings.runAlerts;
    EXPECT_TRUE(ra.runWalkEn);
    EXPECT_EQ(ra.runWalkRunSec, 240u);   // default
    EXPECT_EQ(ra.runWalkWalkSec, 60u);   // default
    EXPECT_EQ(ra.paceFastSecPerKm, 0.0f); // Off
    EXPECT_EQ(ra.paceSlowSecPerKm, 0.0f); // Off
    EXPECT_EQ(ra.hrHighBpm, 0);           // Off
    EXPECT_EQ(ra.hrLowBpm, 0);            // Off
    EXPECT_TRUE(ra.backInTap);
}

TEST(RunAlertsSettings, NegativeAndHugeValuesFallBack)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": {
            "run_walk_run_time": -5,
            "run_walk_walk_time": 99999999,
            "pace_fast": -270,
            "pace_slow": 1e9,
            "hr_high": 300,
            "hr_low": -100
        }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    const Settings::RunAlerts& ra = settings.runAlerts;
    EXPECT_EQ(ra.runWalkRunSec, 240u);
    EXPECT_EQ(ra.runWalkWalkSec, 60u);
    EXPECT_EQ(ra.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(ra.paceSlowSecPerKm, 0.0f);
    EXPECT_EQ(ra.hrHighBpm, 0);
    EXPECT_EQ(ra.hrLowBpm, 0);
}

TEST(RunAlertsSettings, RangeEdgesLoadAsIs)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": {
            "run_walk_run_time": 3600,
            "run_walk_walk_time": 5,
            "pace_fast": 60,
            "pace_slow": 1800,
            "hr_high": 240,
            "hr_low": 30
        }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    const Settings::RunAlerts& ra = settings.runAlerts;
    EXPECT_EQ(ra.runWalkRunSec, 3600u);
    EXPECT_EQ(ra.runWalkWalkSec, 5u);
    EXPECT_EQ(ra.paceFastSecPerKm, 60.0f);
    EXPECT_EQ(ra.paceSlowSecPerKm, 1800.0f);
    EXPECT_EQ(ra.hrHighBpm, 240);
    EXPECT_EQ(ra.hrLowBpm, 30);
}

TEST(RunAlertsSettings, InvertedPaceAndHrPairsLoadAsOff)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": {
            "pace_fast": 330,
            "pace_slow": 270,
            "hr_high": 120,
            "hr_low": 165
        }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.runAlerts.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(settings.runAlerts.paceSlowSecPerKm, 0.0f);
    EXPECT_EQ(settings.runAlerts.hrHighBpm, 0);
    EXPECT_EQ(settings.runAlerts.hrLowBpm, 0);
}

TEST(RunAlertsSettings, EqualLimitsLoadAsOff)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": { "pace_fast": 300, "pace_slow": 300, "hr_high": 150, "hr_low": 150 }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.runAlerts.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(settings.runAlerts.paceSlowSecPerKm, 0.0f);
    EXPECT_EQ(settings.runAlerts.hrHighBpm, 0);
    EXPECT_EQ(settings.runAlerts.hrLowBpm, 0);
}

TEST(RunAlertsSettings, SingleLimitsAreKept)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "run_alerts": { "pace_fast": 0, "pace_slow": 360, "hr_high": 170, "hr_low": 0 }
    })");

    Settings settings {};
    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.runAlerts.paceFastSecPerKm, 0.0f);
    EXPECT_EQ(settings.runAlerts.paceSlowSecPerKm, 360.0f);
    EXPECT_EQ(settings.runAlerts.hrHighBpm, 170);
    EXPECT_EQ(settings.runAlerts.hrLowBpm, 0);
}

TEST(RunAlertsSettings, ExistingKeysLoadUnchangedAlongside)
{
    KernelFixture fixture;
    Settings saved {};
    saved.phoneNotifEn                 = false;
    saved.alertDistanceId              = Settings::Alerts::Distance::ID_OFF;
    saved.alertTimeId                  = Settings::Alerts::Time::ID_MIN_5;
    saved.intervals.repeatsNum         = 6;
    saved.intervals.runMetric          = Settings::Intervals::Metric::DISTANCE;
    saved.intervals.runDistance        = 400.0f;
    saved.intervals.restMetric         = Settings::Intervals::Metric::TIME;
    saved.intervals.restTime           = 90;
    saved.intervals.warmUp             = false;
    saved.intervals.coolDown           = true;
    saved.intervals.lastRest           = false;
    saved.runAlerts                    = nonDefaultRunAlerts();

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.save(saved));

    Settings loaded {};
    ASSERT_TRUE(serializer.load(loaded));

    EXPECT_EQ(loaded.version, Settings::kVersion);
    EXPECT_FALSE(loaded.phoneNotifEn);
    EXPECT_EQ(loaded.alertDistanceId, Settings::Alerts::Distance::ID_OFF);
    EXPECT_EQ(loaded.alertTimeId, Settings::Alerts::Time::ID_MIN_5);
    EXPECT_EQ(loaded.intervals.repeatsNum, 6);
    EXPECT_EQ(loaded.intervals.runMetric, Settings::Intervals::Metric::DISTANCE);
    EXPECT_EQ(loaded.intervals.runDistance, 400.0f);
    EXPECT_EQ(loaded.intervals.restMetric, Settings::Intervals::Metric::TIME);
    EXPECT_EQ(loaded.intervals.restTime, 90u);
    EXPECT_FALSE(loaded.intervals.warmUp);
    EXPECT_TRUE(loaded.intervals.coolDown);
    EXPECT_FALSE(loaded.intervals.lastRest);
    EXPECT_TRUE(loaded.runAlerts.runWalkEn);
}

TEST(RunAlertsSettings, VersionStaysOne)
{
    EXPECT_EQ(Settings::kVersion, 1);
}

} // namespace
