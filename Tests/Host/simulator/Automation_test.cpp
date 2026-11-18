/**
 * @file Automation_test.cpp
 * @brief Simulator scripts (SDK/Simulator/Automation): timing, buttons, sensor
 *        values, replay files and the PNG writer.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "SDK/Simulator/Automation/Overrides.hpp"
#include "SDK/Simulator/Automation/Png.hpp"
#include "SDK/Simulator/Automation/Script.hpp"

namespace
{

using namespace SDK::Simulator::Automation;

/// Records what a script asks of the simulator.
class FakeHost : public ScriptHost
{
public:
    bool hasButtons() const override { return buttons; }
    void buttonDown(Button b) override { events.push_back(std::string("down ") + buttonName(b)); }
    void buttonUp(Button b, bool click) override
    {
        events.push_back(std::string(click ? "click " : "up ") + buttonName(b));
    }
    bool saveDisplay(const std::string& path) override
    {
        events.push_back("shot " + std::filesystem::path(path).filename().string());
        return true;
    }
    void wristMotion() override { events.push_back("wrist"); }
    void endRun() override { events.push_back("end"); }

    bool                     buttons = true;
    std::vector<std::string> events;
};

class AutomationTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        Overrides& o = Overrides::get();
        o.setHeartRate(-1);
        o.setBattery(-1.0f);
        o.setPressure(0.0f);
        o.setGpsFix(Overrides::GpsFix::Auto);
        o.stopReplay();
        o.setWristHandler(nullptr);
        std::error_code ec;
        std::filesystem::remove_all(mDir, ec);
    }

    std::string write(const std::string& name, const std::string& text)
    {
        std::filesystem::create_directories(mDir);
        const std::string path = (mDir / name).string();
        std::ofstream(path) << text;
        return path;
    }

    // One directory per fixture, so parallel test processes don't share files.
    std::filesystem::path mDir = std::filesystem::temp_directory_path() /
                                 ("una-sim-automation-test-" + std::to_string(std::random_device{}()));
};

TEST_F(AutomationTest, ClickPressesThenReleasesAsAClick)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "1000 click R1\n"));

    runner.tickAt(999);
    EXPECT_TRUE(host.events.empty());
    runner.tickAt(1000);
    ASSERT_EQ(host.events, (std::vector<std::string>{ "down R1" }));
    runner.tickAt(1099);
    EXPECT_EQ(host.events.size(), 1u);
    runner.tickAt(1100);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down R1", "click R1" }));
}

TEST_F(AutomationTest, LongHoldReleasesWithoutAClick)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 hold L2 3500\n"));

    runner.tickAt(0);
    runner.tickAt(3499);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down L2" }));
    runner.tickAt(3500);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down L2", "up L2" }));
}

TEST_F(AutomationTest, ShortHoldCountsAsAClick)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 press L1\n499 release L1\n"));

    runner.tickAt(0);
    runner.tickAt(499);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down L1", "click L1" }));
}

TEST_F(AutomationTest, EventsInOneFrameRunInScriptOrder)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 click R1\n50 click L2\n"));

    // One frame covers both clicks: L2's press (50) comes before R1's release (100).
    runner.tickAt(0);
    runner.tickAt(200);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down R1", "down L2", "click R1", "click L2" }));
}

TEST_F(AutomationTest, ClickOrNotDoesNotDependOnTheFrames)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 hold R1 450\n"));

    // Frames at 0 and 600: the hold still lasted 450 ms, a click.
    runner.tickAt(0);
    runner.tickAt(600);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down R1", "click R1" }));
}

TEST_F(AutomationTest, ScheduledReleaseDoesNotEndALaterPress)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 hold R1 1000\n100 release R1\n200 press R1\n"));

    runner.tickAt(0);
    runner.tickAt(100);
    runner.tickAt(200);
    runner.tickAt(1000);   // the hold's release belonged to the first press
    EXPECT_EQ(host.events, (std::vector<std::string>{ "down R1", "click R1", "down R1" }));
}

TEST_F(AutomationTest, RelativeAndSecondTimes)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt",
                            "# comment line\n"
                            "1s     wrist      # trailing comment\n"
                            "+500   screenshot a\n"
                            "+1.5s  screenshot b.png\n"));

    runner.tickAt(999);
    EXPECT_TRUE(host.events.empty());
    runner.tickAt(1000);
    runner.tickAt(1500);
    runner.tickAt(2999);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "wrist", "shot a.png" }));
    runner.tickAt(3000);
    EXPECT_EQ(host.events.back(), "shot b.png");
}

TEST_F(AutomationTest, ExitEndsTheRunOnce)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "100 exit\n200 wrist\n"));

    runner.tickAt(100);
    EXPECT_TRUE(runner.stopped());
    runner.tickAt(300);
    EXPECT_EQ(host.events, (std::vector<std::string>{ "end" }));
}

TEST_F(AutomationTest, SensorStepsSetAndClearOverrides)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt",
                            "0   hr 152\n"
                            "0   battery 15\n"
                            "0   pressure 990.5\n"
                            "0   gps fix off\n"
                            "100 hr auto\n"
                            "100 gps fix on\n"));
    Overrides& o = Overrides::get();
    int   bpm = 0;
    float v   = 0.0f;

    runner.tickAt(0);
    EXPECT_TRUE(o.heartRate(bpm));
    EXPECT_EQ(bpm, 152);
    EXPECT_TRUE(o.battery(v));
    EXPECT_FLOAT_EQ(v, 15.0f);
    EXPECT_TRUE(o.pressure(v));
    EXPECT_FLOAT_EQ(v, 990.5f);
    EXPECT_EQ(o.gpsFix(), Overrides::GpsFix::Off);

    runner.tickAt(100);
    EXPECT_FALSE(o.heartRate(bpm));
    EXPECT_EQ(o.gpsFix(), Overrides::GpsFix::On);
}

TEST_F(AutomationTest, WristMotionNeedsASensorLayer)
{
    Overrides& o = Overrides::get();
    EXPECT_FALSE(o.wristMotion());   // no sensor layer registered
    int calls = 0;
    o.setWristHandler([&calls] { ++calls; });
    EXPECT_TRUE(o.wristMotion());
    EXPECT_EQ(calls, 1);
    o.setWristHandler(nullptr);
}

TEST_F(AutomationTest, HeartRateUsesTheValueValidated)
{
    FakeHost host;
    Runner   runner(host, true);
    runner.loadScript(write("s.txt", "0 hr 1.52e2\n"));
    runner.tickAt(0);
    int bpm = 0;
    ASSERT_TRUE(Overrides::get().heartRate(bpm));
    EXPECT_EQ(bpm, 152);
}

TEST_F(AutomationTest, ReplayFileRejectsAnIncompleteFirstRow)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    // Not a header: it starts with a number but lacks the speed.
    EXPECT_FALSE(Overrides::loadTrack(write("short.csv", "0,51.5,-0.1,20\n1,51.5,-0.1,20,3\n"), track, error));
    EXPECT_NE(error.find("expected"), std::string::npos);
}

TEST_F(AutomationTest, ReplayFileRejectsTrailingText)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    EXPECT_FALSE(Overrides::loadTrack(write("unit.csv", "0,51.5,-0.1,20,3km/h\n"), track, error));
    EXPECT_NE(error.find("unit.csv:1: expected"), std::string::npos);
    EXPECT_FALSE(Overrides::loadTrack(write("six.csv", "0,51.5,-0.1,20,3,7\n"), track, error));
    // Trailing whitespace (and a CRLF line end) is still a valid row.
    track.clear();
    EXPECT_TRUE(Overrides::loadTrack(write("ws.csv", "0,51.5,-0.1,20,3  \t\r\n"), track, error)) << error;
    ASSERT_EQ(track.size(), 1u);
    EXPECT_FLOAT_EQ(track[0].speed, 3.0f);
}

TEST_F(AutomationTest, ReplayFileRejectsOutOfRangeValues)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    EXPECT_FALSE(Overrides::loadTrack(write("fast.csv", "0,51.5,-0.1,20,3e38\n"), track, error));
    EXPECT_NE(error.find("out of range"), std::string::npos);
    EXPECT_FALSE(Overrides::loadTrack(write("lat.csv", "0,91,-0.1,20,3\n"), track, error));
}

TEST_F(AutomationTest, ReplayFileParsesWithAHeader)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    ASSERT_TRUE(Overrides::loadTrack(write("t.csv",
                                           "t_s,lat,lon,alt_m,speed_mps\r\n"
                                           "0,51.5,-0.1,20,3\r\n"
                                           "2,51.5001,-0.1,21,4\r\n"),
                                     track, error))
        << error;
    ASSERT_EQ(track.size(), 2u);
    EXPECT_DOUBLE_EQ(track[1].t, 2.0);
    EXPECT_DOUBLE_EQ(track[1].lat, 51.5001);
    EXPECT_FLOAT_EQ(track[1].speed, 4.0f);
}

TEST_F(AutomationTest, ReplayFileRejectsTimeGoingBack)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    EXPECT_FALSE(Overrides::loadTrack(write("t.csv", "0,1,1,0,0\n0,1,1,0,0\n"), track, error));
    EXPECT_NE(error.find("t_s must increase"), std::string::npos);
}

TEST_F(AutomationTest, ReplayFileRejectsNonFiniteValues)
{
    std::vector<Overrides::TrackPoint> track;
    std::string error;
    EXPECT_FALSE(Overrides::loadTrack(write("t.csv", "0,51.5,-0.1,20,3\n1,nan,-0.1,20,3\n"), track, error));
    EXPECT_NE(error.find("finite"), std::string::npos);
}

TEST_F(AutomationTest, ReplayStartsAtTheFirstFix)
{
    std::string error;
    ASSERT_TRUE(Overrides::get().startReplay(write("t.csv", "0,51.5,-0.1,20,3\n100,51.6,-0.1,30,3\n"), error));
    Overrides::TrackPoint fix;
    ASSERT_TRUE(Overrides::get().replayFix(fix));
    EXPECT_NEAR(fix.lat, 51.5, 0.001);   // a few ms into a 100 s track
    Overrides::get().stopReplay();
    EXPECT_FALSE(Overrides::get().replayFix(fix));
}

TEST_F(AutomationTest, ReplayHoldsTheFirstFixBeforeItsTime)
{
    std::string error;
    Overrides::TrackPoint fix;
    // One fix, 100 s in: no second point to read past.
    ASSERT_TRUE(Overrides::get().startReplay(write("one.csv", "100,51.5,-0.1,20,3\n"), error));
    ASSERT_TRUE(Overrides::get().replayFix(fix));
    EXPECT_DOUBLE_EQ(fix.lat, 51.5);
    EXPECT_FLOAT_EQ(fix.speed, 3.0f);
    // Two fixes from 100 s: no extrapolation backwards from them.
    ASSERT_TRUE(Overrides::get().startReplay(write("two.csv", "100,51.5,-0.1,20,3\n200,51.6,-0.1,30,3\n"), error));
    ASSERT_TRUE(Overrides::get().replayFix(fix));
    EXPECT_DOUBLE_EQ(fix.lat, 51.5);
    EXPECT_FLOAT_EQ(fix.alt, 20.0f);
}

TEST_F(AutomationTest, ReplayDistanceFollowsTheSpeedsAlongTheTrack)
{
    std::string error;
    // From 5 s to 15 s, speed rising 2 -> 4 m/s.
    ASSERT_TRUE(Overrides::get().startReplay(write("d.csv", "5,51.5,-0.1,20,2\n15,51.6,-0.1,30,4\n"), error));
    const Overrides& o = Overrides::get();
    EXPECT_NEAR(o.trackDistance(5, 15), 30.0, 1e-9);    // the whole track
    EXPECT_NEAR(o.trackDistance(0, 5), 0.0, 1e-9);      // before the first fix
    EXPECT_NEAR(o.trackDistance(10, 15), 17.5, 1e-9);   // 3 -> 4 m/s for 5 s
    EXPECT_NEAR(o.trackDistance(13, 30), 7.6, 1e-9);    // across the end: 3.6 -> 4 m/s for 2 s
    EXPECT_NEAR(o.trackDistance(20, 30), 0.0, 1e-9);    // after the end

    // A sensor's first call counts from the start of the replay.
    Overrides::ReplayCursor cursor;
    Overrides::TrackPoint   fix;
    double metres = -1.0;
    ASSERT_TRUE(o.replayAdvance(cursor, fix, metres));
    EXPECT_DOUBLE_EQ(metres, 0.0);   // a few ms in, before the first fix
    EXPECT_NE(cursor.replay, 0u);
}

TEST_F(AutomationTest, PngHasTheExpectedLayout)
{
    const uint8_t rgb[2 * 2 * 3] = { 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255 };
    const std::string path = (mDir / "x.png").string();
    std::filesystem::create_directories(mDir);
    ASSERT_TRUE(Png::writeRgb(path, 2, 2, rgb));

    std::ifstream in(path, std::ios::binary);
    const std::vector<uint8_t> f((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    ASSERT_GE(f.size(), 8u + 25u);
    EXPECT_TRUE(std::equal(sig, sig + 8, f.begin()));
    EXPECT_EQ(std::string(f.begin() + 12, f.begin() + 16), "IHDR");
    EXPECT_EQ(f[19], 2);   // width 2
    EXPECT_EQ(f[23], 2);   // height 2
    // Signature, IHDR, IDAT (zlib header + one stored block of 2 x (1 + 6) bytes + Adler-32), IEND.
    EXPECT_EQ(f.size(), 8u + 25u + (12u + 2u + 5u + 14u + 4u) + 12u);
    EXPECT_EQ(std::string(f.end() - 8, f.end() - 4), "IEND");
    // IEND's CRC is fixed: AE 42 60 82.
    EXPECT_EQ(f[f.size() - 4], 0xAE);
    EXPECT_EQ(f[f.size() - 1], 0x82);
}

using AutomationDeathTest = AutomationTest;

TEST_F(AutomationDeathTest, UnknownStepStopsWithExitCode2)
{
    const std::string path = write("bad.txt", "0 clik R1\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "unknown step 'clik'");
}

TEST_F(AutomationDeathTest, ButtonStepOnAGlanceStopsWithExitCode2)
{
    const std::string path = write("glance.txt", "0 click R1\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            host.buttons = false;
            Runner runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "no buttons");
}

TEST_F(AutomationDeathTest, TimeGoingBackStopsWithExitCode2)
{
    const std::string path = write("back.txt", "500 wrist\n400 wrist\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "before the previous step");
}

TEST_F(AutomationDeathTest, TimeTooLateStopsWithExitCode2)
{
    const std::string path = write("late.txt", "4294967296 exit\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "later than the latest time");
}

TEST_F(AutomationDeathTest, HoldPastTheLatestTimeStopsWithExitCode2)
{
    const std::string path = write("hold.txt", "0 hold R1 4294967296\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "released by the latest time");
}

TEST_F(AutomationDeathTest, FractionalHeartRateStopsWithExitCode2)
{
    const std::string path = write("hr.txt", "0 hr 150.5\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "whole number");
}

TEST_F(AutomationDeathTest, ScreenshotOutsideOutStopsWithExitCode2)
{
    const std::string path = write("shot.txt", "0 screenshot ../escape\n");
    EXPECT_EXIT(
        {
            FakeHost host;
            Runner   runner(host, true);
            runner.loadScript(path);
        },
        ::testing::ExitedWithCode(kExitBadInput), "inside --out");
}

} // namespace
