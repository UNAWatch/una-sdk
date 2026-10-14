/**
 ******************************************************************************
 * @file    StepLabel_test.cpp
 * @brief   Tests for SDK::Workout::stepLabel() and its parts.
 ******************************************************************************
 */

#include "SDK/Workout/StepLabel.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>

using namespace SDK::Workout;

namespace {

Step timed(uint32_t ms, Intensity i = Intensity::Active)
{
    Step s;
    s.end = StepEnd::Time;
    s.durationMs = ms;
    s.intensity = i;
    return s;
}

Step measured(uint32_t cm, Intensity i = Intensity::Active)
{
    Step s;
    s.end = StepEnd::Distance;
    s.distanceCm = cm;
    s.intensity = i;
    return s;
}

Step open(Intensity i = Intensity::Active)
{
    Step s;
    s.end = StepEnd::Open;
    s.intensity = i;
    return s;
}

Step withPace(Step s, uint32_t lowMmps, uint32_t highMmps)
{
    s.target = Target::Speed;
    s.speedLowMmps = lowMmps;
    s.speedHighMmps = highMmps;
    return s;
}

Step withHr(Step s, HeartRateTarget kind, uint16_t low, uint16_t high, uint8_t zone = 0)
{
    s.target = Target::HeartRate;
    s.hrKind = kind;
    s.hrLow = low;
    s.hrHigh = high;
    s.hrZone = zone;
    return s;
}

void setNotes(Step& s, const char* text)
{
    std::memcpy(s.notes, text, std::strlen(text) + 1);
}

std::string label(const Step& s, bool imperial = false)
{
    char buf[64];
    const size_t n = stepLabel(s, imperial, buf, sizeof(buf));
    EXPECT_EQ(n, std::strlen(buf));
    return buf;
}

std::string duration(const Step& s, bool imperial = false)
{
    char buf[32];
    stepDuration(s, imperial, buf, sizeof(buf));
    return buf;
}

}  // namespace

TEST(StepLabel, NotesWinWhenThereAreAny)
{
    Step s = withPace(measured(40000), 4082, 4444);
    setNotes(s, "400m at 3:55/km");
    EXPECT_EQ(label(s), "400m at 3:55/km");
}

TEST(StepLabel, MadeFromDurationKindAndTarget)
{
    EXPECT_EQ(label(withPace(measured(40000), 4082, 4444)), "400 m 3:45-4:05 /km");
    EXPECT_EQ(label(timed(90000, Intensity::Rest)), "90 s rest");
    EXPECT_EQ(label(open(Intensity::Warmup)), "Warm up");
    EXPECT_EQ(label(measured(150000, Intensity::Cooldown)), "1.5 km cool down");
    EXPECT_EQ(label(withHr(timed(600000), HeartRateTarget::Bpm, 140, 155)), "10 min 140-155 bpm");
    EXPECT_EQ(label(withHr(open(), HeartRateTarget::PercentMax, 70, 80)), "70-80% max");
    EXPECT_EQ(label(withHr(measured(321869), HeartRateTarget::Zone, 0, 0, 3), true), "2 mi zone 3");
    EXPECT_EQ(label(open()), "Open");
    EXPECT_EQ(label(open(Intensity::Rest)), "Rest");
}

TEST(StepLabel, Durations)
{
    EXPECT_EQ(duration(timed(45000)), "45 s");
    EXPECT_EQ(duration(timed(300000)), "5 min");
    EXPECT_EQ(duration(timed(90500)), "91 s");  // rounded to the second
    EXPECT_EQ(duration(timed(119000)), "119 s");
    EXPECT_EQ(duration(timed(150000)), "2:30");
    EXPECT_EQ(duration(timed(3900000)), "1:05:00");
    EXPECT_EQ(duration(measured(20000)), "200 m");
    EXPECT_EQ(duration(measured(100000)), "1 km");
    EXPECT_EQ(duration(measured(212500)), "2.13 km");
    EXPECT_EQ(duration(measured(40234), true), "0.25 mi");
    EXPECT_EQ(duration(measured(160934), true), "1 mi");
    EXPECT_EQ(duration(open()), "");
    Step rep;
    rep.end = StepEnd::Repeat;
    rep.repeatCount = 5;
    EXPECT_EQ(duration(rep), "x5");
    EXPECT_EQ(label(rep), "X5");
}

TEST(StepLabel, PaceRangesReadFastestFirstInTheRunnersUnits)
{
    const Step s = withPace(measured(100000), 3279, 3509);  // 5:05-4:45 /km
    EXPECT_EQ(label(s), "1 km 4:45-5:05 /km");
    EXPECT_EQ(label(s, true), "0.62 mi 7:39-8:11 /mi");
    EXPECT_EQ(label(withPace(timed(60000), 4000, 4000)), "60 s 4:10 /km");  // a single pace
    EXPECT_EQ(paceSeconds(0, false), 0u);
    EXPECT_EQ(paceSeconds(4082, false), 245u);
}

TEST(StepLabel, CutOnACharacterBoundaryWhenTooLong)
{
    Step s = open();
    setNotes(s, "Caf\xC3\xA9 run");  // two-byte e-acute at bytes 3-4
    char buf[5];
    EXPECT_EQ(stepLabel(s, false, buf, sizeof(buf)), 3u);
    EXPECT_STREQ(buf, "Caf");
}
