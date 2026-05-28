#include <gtest/gtest.h>

#include "Settings.hpp"

TEST(RunningSettings, DefaultsUseDistanceLapAlertAndTimeOff)
{
    Settings settings {};

    EXPECT_EQ(settings.alertDistanceId, Settings::Alerts::Distance::ID_DEFAULT);
    EXPECT_EQ(settings.alertTimeId, Settings::Alerts::Time::ID_OFF);
}
