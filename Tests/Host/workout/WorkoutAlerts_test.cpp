/**
 ******************************************************************************
 * @file    WorkoutAlerts_test.cpp
 * @brief   Tests for SDK::Workout::LeadInAlert, driven by a real Engine.
 ******************************************************************************
 */

#include "SDK/Workout/WorkoutAlerts.hpp"

#include <gtest/gtest.h>

#include <memory>
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
