/**
 ******************************************************************************
 * @file    WorkoutAlerts_test.cpp
 * @brief   Tests for SDK::Workout::LeadInAlert, driven by a real Engine.
 ******************************************************************************
 */

#include "SDK/Workout/WorkoutAlerts.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <utility>
#include <vector>

using namespace SDK::Workout;

namespace {

Step& add(Program& p, StepEnd end)
{
    Step& s = p.steps[p.stepCount++];
    s = Step{};
    s.end = end;
    return s;
}

// Drives an engine and a lead-in alert once a second, as an app does.
struct Drive {
    Engine      engine;
    LeadInAlert alert;
    uint32_t    t = 0;      // s
    uint32_t    cm = 0;
    std::vector<uint32_t> beeps;    // seconds that beeped
    std::vector<uint32_t> changes;  // seconds that changed step

    explicit Drive(const Program& p)
    {
        engine.start(p, 0, 0);
        alert.stepStarted();
    }

    void second(uint32_t speedCmps, bool speedValid = true)
    {
        ++t;
        cm += speedCmps;
        const auto how = engine.update(t * 1000u, cm, speedCmps * 10u, speedValid);
        if (how != Engine::Change::None) {
            changes.push_back(t);
            alert.stepStarted();
        } else if (alert.tick(engine.status())) {
            beeps.push_back(t);
        }
    }
};

}  // namespace

TEST(LeadInAlert, TimeStepBeepsEachOfItsLastFiveSeconds)
{
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Time).durationMs = 30000;
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 40; ++i) run.second(300);
    EXPECT_EQ(run.changes, (std::vector<uint32_t>{30}));
    EXPECT_EQ(run.beeps, (std::vector<uint32_t>{25, 26, 27, 28, 29}));
}

TEST(LeadInAlert, DistanceStepBeepsFromTheEstimatedFiveSeconds)
{
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Distance).distanceCm = 100000;  // 1 km
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 400; ++i) run.second(300);  // 3 m/s
    ASSERT_EQ(run.changes, (std::vector<uint32_t>{334}));
    EXPECT_EQ(run.beeps, (std::vector<uint32_t>{329, 330, 331, 332, 333}));
}

TEST(LeadInAlert, OnceStartedItKeepsItsRhythmThroughALostSpeed)
{
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Distance).distanceCm = 100000;
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 330; ++i) run.second(300);
    run.second(300, false);  // no speed reading at 331: no estimate either
    for (int i = 0; i < 10; ++i) run.second(300);
    EXPECT_EQ(run.beeps, (std::vector<uint32_t>{329, 330, 331, 332, 333}));
}

TEST(LeadInAlert, StoppingJustShortOfTheDistanceStopsBeeping)
{
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Distance).distanceCm = 100000;
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 330; ++i) run.second(300);
    for (int i = 0; i < 60; ++i) run.second(0);  // waiting at a crossing
    EXPECT_EQ(run.beeps.size(), static_cast<size_t>(LeadInAlert::kMaxBeeps));
    EXPECT_TRUE(run.changes.empty());
}

TEST(LeadInAlert, EachPassOfARepeatedStepGetsItsOwnLeadIn)
{
    // One step repeated: the step index is the same on every pass.
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Time).durationMs = 10000;
    Step& rep = add(*p, StepEnd::Repeat);
    rep.repeatFrom  = 0;
    rep.repeatCount = 3;
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 40; ++i) run.second(300);
    EXPECT_EQ(run.changes, (std::vector<uint32_t>{10, 20, 30}));
    EXPECT_EQ(run.beeps, (std::vector<uint32_t>{5, 6, 7, 8, 9, 15, 16, 17, 18, 19, 25, 26, 27, 28, 29}));
}

TEST(LeadInAlert, OpenStepsAndTheEndAreSilent)
{
    auto p = std::make_unique<Program>();
    add(*p, StepEnd::Open);
    Drive run(*p);
    for (int i = 0; i < 30; ++i) run.second(300);
    run.engine.next(run.t * 1000u, run.cm);  // the program ends
    run.second(300);
    EXPECT_TRUE(run.beeps.empty());
}

// --- ZoneAlerts ------------------------------------------------------------------

namespace {

using A = ZoneAlerts::Alert;

// The alerts for @p zones, one per tick, as (tick, alert) for those that fire.
std::vector<std::pair<uint32_t, A>> alertsFor(const std::vector<Zone>& zones)
{
    ZoneAlerts z;
    z.stepStarted();
    std::vector<std::pair<uint32_t, A>> out;
    for (size_t i = 0; i < zones.size(); ++i) {
        const A a = z.tick(zones[i]);
        if (a != A::None) {
            out.emplace_back(static_cast<uint32_t>(i + 1), a);
        }
    }
    return out;
}

// @p before, then @p n ticks of @p z.
std::vector<Zone> ticks(Zone z, size_t n, std::vector<Zone> before = {})
{
    before.insert(before.end(), n, z);
    return before;
}

using Alerts = std::vector<std::pair<uint32_t, A>>;

}  // namespace

TEST(ZoneAlerts, AStepThatStaysInTheBandIsQuiet)
{
    EXPECT_TRUE(alertsFor(ticks(Zone::In, 120, ticks(Zone::Unknown, 8))).empty());
}

TEST(ZoneAlerts, LeavingTheBandAlertsAndRepeatsEveryThirtySeconds)
{
    auto zones = ticks(Zone::In, 40, ticks(Zone::Unknown, 8));  // in until 48
    zones = ticks(Zone::Above, 70, zones);                       // 49 to 118
    EXPECT_EQ(alertsFor(zones), (Alerts{{49, A::Above}, {79, A::Above}, {109, A::Above}}));
}

TEST(ZoneAlerts, ComingBackInAfterAnAlertSaysSoOnce)
{
    auto zones = ticks(Zone::In, 40, ticks(Zone::Unknown, 8));
    zones = ticks(Zone::Below, 10, zones);  // 49 to 58
    zones = ticks(Zone::In, 30, zones);     // 59 on
    EXPECT_EQ(alertsFor(zones), (Alerts{{49, A::Below}, {59, A::BackIn}}));
}

TEST(ZoneAlerts, NothingInTheFirstTwentySecondsOfAStep)
{
    // Off pace from the start: the first alert comes at 21 s.
    EXPECT_EQ(alertsFor(ticks(Zone::Below, 30, ticks(Zone::Unknown, 8))),
              (Alerts{{21, A::Below}}));
    // A dip out and back within the first 20 s says nothing either way.
    auto zones = ticks(Zone::Above, 5, ticks(Zone::In, 10));
    zones = ticks(Zone::In, 30, zones);
    EXPECT_TRUE(alertsFor(zones).empty());
}

TEST(ZoneAlerts, CrossingToTheOtherSideAlertsAtOnce)
{
    auto zones = ticks(Zone::Above, 25);   // alert at 21
    zones = ticks(Zone::Below, 5, zones);  // 26 on
    EXPECT_EQ(alertsFor(zones), (Alerts{{21, A::Above}, {26, A::Below}}));
}

TEST(ZoneAlerts, NoSignalHoldsAlertsBackButKeepsTheOneOutstanding)
{
    auto zones = ticks(Zone::Above, 25);       // alert at 21
    zones = ticks(Zone::NoSignal, 40, zones);  // 26 to 65: quiet
    zones = ticks(Zone::Unknown, 5, zones);    // the hold-off after it
    zones = ticks(Zone::In, 10, zones);        // 71: back in
    EXPECT_EQ(alertsFor(zones), (Alerts{{21, A::Above}, {71, A::BackIn}}));
}

TEST(ZoneAlerts, StillOutAfterAGapAlertsAgainOnceThirtySecondsHavePassed)
{
    auto zones = ticks(Zone::Above, 25);       // alert at 21
    zones = ticks(Zone::NoSignal, 10, zones);  // 26 to 35
    zones = ticks(Zone::Above, 30, zones);     // 36 on; 30 s after 21 is 51
    EXPECT_EQ(alertsFor(zones), (Alerts{{21, A::Above}, {51, A::Above}}));
}

TEST(ZoneAlerts, ANewStepStartsQuietAndForgetsTheLast)
{
    ZoneAlerts z;
    z.stepStarted();
    for (int i = 0; i < 25; ++i) z.tick(Zone::Above);  // alerted at 21
    z.stepStarted();
    // On pace in the new step: no "back in" for the old step's alert.
    for (int i = 0; i < 60; ++i) {
        EXPECT_EQ(z.tick(Zone::In), A::None) << i;
    }
}
