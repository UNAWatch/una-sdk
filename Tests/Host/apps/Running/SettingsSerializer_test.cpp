#include <gtest/gtest.h>

#include "KernelTestDoubles.hpp"
#include "Settings.hpp"
#include "SettingsSerializer.hpp"

namespace {

using SDK::TestSupport::KernelFixture;

TEST(RunningSettingsSerializer, MissingDistanceAlertDefaultsToDistanceIdDefault)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "phone_notif_en": true,
        "alert_time_id": 0
    })");

    Settings settings {};
    settings.alertDistanceId = Settings::Alerts::Distance::ID_OFF;
    settings.alertTimeId = Settings::Alerts::Time::ID_MIN_10;

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_DEFAULT);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_OFF);
}

TEST(RunningSettingsSerializer, InvalidDistanceAlertFallsBackToDistanceIdDefault)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "phone_notif_en": true,
        "alert_distance_id": 255,
        "alert_time_id": 0
    })");

    Settings settings {};
    settings.alertDistanceId = Settings::Alerts::Distance::ID_OFF;

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_DEFAULT);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_OFF);
}

TEST(RunningSettingsSerializer, ValidDistanceAlertLoadedAsIs)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "phone_notif_en": true,
        "alert_distance_id": 5,
        "alert_time_id": 0
    })");

    Settings settings {};

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_KM_MILL_5);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_OFF);
}

TEST(RunningSettingsSerializer, ValidTimeAlertLoadedAsIsWhenDistanceOff)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "phone_notif_en": true,
        "alert_distance_id": 0,
        "alert_time_id": 3
    })");

    Settings settings {};

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_OFF);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_MIN_10);
}

// A legacy file with both auto-laps enabled is normalized on load: distance
// wins (matching getLapDivSource's priority) and time is cleared, so the loaded
// state can never have both enabled.
TEST(RunningSettingsSerializer, BothAlertsEnabledNormalizesToDistance)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("settings.json", R"({
        "version": 1,
        "phone_notif_en": true,
        "alert_distance_id": 5,
        "alert_time_id": 3
    })");

    Settings settings {};

    SettingsSerializer serializer(fixture.kernel, "settings.json");
    ASSERT_TRUE(serializer.load(settings));

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_KM_MILL_5);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_OFF);
}

} // namespace
