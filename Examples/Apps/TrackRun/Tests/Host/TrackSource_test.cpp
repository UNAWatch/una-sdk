// Run::TrackSource: the run's distance, live speed and altitude as the
// service takes them, over generated traces.

#include "OvalTestUtil.hpp"
#include "SessionTestUtil.hpp"
#include "TraceGen.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/TrackStore.hpp"
#include "Run/TrackSource.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

struct Out {
    std::vector<float> totalM;    ///< After each reading
    std::vector<float> speedMps;  ///< After each tick
    float              maxSpeedMps = 0.0f;
    std::vector<float> altM;      ///< After each tick
    float              ascentM = 0.0f;
};

/// Drives a TrackSource as the service does: the pre-run readings prime it,
/// Start at the trace's start in its lane, each command before its reading,
/// each reading, and a 1 Hz tick at each reading's time with the barometer
/// from baroAltM (or none).
Out drive(const TraceGen::Trace& tr, Run::TrackSource& src, const Oval::TrackStore* store = nullptr,
          const std::vector<float>* baroAltM = nullptr)
{
    Out o;
    for (size_t i = 0; i < tr.startIndex; ++i) {
        src.prime(tr.samples[i]);
    }
    EXPECT_TRUE(src.start(tr.startMs, tr.startLane, Oval::LaneTable::outdoor(), store));
    size_t cmd = 0;
    bool   paused = false;
    float  asc = 0.0f, desc = 0.0f, lastAlt = NAN;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        for (; cmd < tr.commands.size() && tr.commands[cmd].beforeSample <= i; ++cmd) {
            const TraceGen::Command& c = tr.commands[cmd];
            switch (c.kind) {
                case TraceGen::CmdKind::Pause:   src.pause(c.tMs); paused = true; break;
                case TraceGen::CmdKind::Resume:  src.resume(c.tMs); paused = false; break;
                case TraceGen::CmdKind::SetLane: src.setLane(c.lane, c.tMs); break;
                default: break;   // Run's laps: the Lap button is not the Session's
            }
        }
        src.reading(tr.samples[i]);
        o.totalM.push_back(src.totalM());
        if (!paused) {
            Run::Baro b;
            if (baroAltM != nullptr) {
                const float a = (*baroAltM)[i];
                if (std::isfinite(lastAlt)) {
                    asc += a > lastAlt ? a - lastAlt : 0.0f;
                    desc += a < lastAlt ? lastAlt - a : 0.0f;
                }
                lastAlt    = a;
                b.valid    = true;
                b.altM     = a;
                b.ascentM  = asc;
                b.descentM = desc;
            }
            src.tick(tr.samples[i].tMs, b);
            o.speedMps.push_back(src.speedMps());
            o.maxSpeedMps = std::max(o.maxSpeedMps, src.speedMps());
            float alt = NAN;
            o.altM.push_back(src.altitude(alt) ? alt : NAN);
        }
    }
    src.finish();
    o.ascentM = src.ascentM();
    return o;
}

bool neverDecreases(const std::vector<float>& v)
{
    for (size_t i = 0; i < v.size(); ++i) {
        if (!std::isfinite(v[i]) || (i > 0 && v[i] < v[i - 1])) {
            return false;
        }
    }
    return true;
}

} // namespace

TEST(TrackSource, TheDistanceFollowsTheLaneAndNeverDecreases)
{
    // Five laps in lane 3 of a track not stored, at 4 m/s: the distance is
    // the lane's (about 15.3 m a lap more than lane 1), and never runs back.
    for (uint32_t seed : {1u, 2u, 3u}) {
        TraceGen::Generator g(TraceGen::Track(), OvalTest::noisy(2.0f), seed);
        g.begin(0.0f, 3);
        g.runLaps(5.0, SessionTest::jog(4.0f));
        const TraceGen::Trace tr = g.finish();
        static Run::TrackSource src;
        src = Run::TrackSource();
        const Out o = drive(tr, src);
        const double lane3M = Oval::LaneTable::outdoor().lapLengthM(3);
        EXPECT_TRUE(neverDecreases(o.totalM)) << "seed " << seed;
        EXPECT_NEAR(o.totalM.back(), 5.0 * lane3M, 0.02 * 5.0 * lane3M) << "seed " << seed;
        // The live speed over the last minute, on average: a single reading
        // carries the GPS noise.
        ASSERT_GT(o.speedMps.size(), 60u);
        float sum = 0.0f;
        for (size_t k = o.speedMps.size() - 60; k < o.speedMps.size(); ++k) {
            sum += o.speedMps[k];
        }
        EXPECT_NEAR(sum / 60.0f, 4.0f, 0.2f) << "seed " << seed << ": the live speed";
    }
}

TEST(TrackSource, ALaneChangeDoesNotShowAsABurstOfPace)
{
    // On a stored track, lane 1 for one and a half laps, the lane set to 4
    // mid-lap, then on: the lap so far is measured again on lane 4 at once,
    // and the live speed over the next half minute stays near the running
    // speed, as it was before the change.
    for (uint32_t seed : {4u, 5u, 6u}) {
        TraceGen::Track tk;
        Oval::TrackStore store;
        SessionTest::storeTrack(store, tk, 0.0, 0.0);
        TraceGen::Generator g(tk, OvalTest::noisy(1.5f), seed);
        g.begin(0.0f, 1);
        g.runLaps(1.5, SessionTest::jog(4.0f));
        g.changeLane(4);
        g.runLaps(1.0, SessionTest::jog(4.0f));
        const TraceGen::Trace tr = g.finish();
        static Run::TrackSource src;
        src = Run::TrackSource();
        const Out o = drive(tr, src, &store);
        EXPECT_TRUE(neverDecreases(o.totalM)) << "seed " << seed;
        size_t change = 0;
        for (const TraceGen::Command& c : tr.commands) {
            change = c.kind == TraceGen::CmdKind::SetLane ? c.beforeSample - tr.startIndex : change;
        }
        ASSERT_GT(change, 30u);
        float before = 0.0f, after = 0.0f;
        for (size_t k = change - 30; k < change; ++k) {
            before = std::max(before, o.speedMps[k]);
        }
        for (size_t k = change; k < change + 30 && k < o.speedMps.size(); ++k) {
            after = std::max(after, o.speedMps[k]);
        }
        EXPECT_LT(after, before * 1.15f) << "seed " << seed << ": no burst of pace from the lane change";
    }
}

TEST(TrackSource, APauseHoldsTheDistance)
{
    // A minute paused, the runner walking on: the total holds while paused.
    // (Laps count by where the runner is on the track, so a line walked over
    // while paused is credited after the resume: docs/OVAL.md, the pause.)
    TraceGen::Generator g(TraceGen::Track(), OvalTest::noisy(1.5f), 7);
    g.begin(0.0f, 1);
    g.runLaps(1.0, SessionTest::jog(4.0f));
    g.pause();
    g.run(80.0, SessionTest::jog(1.3f));
    g.resume();
    g.runLaps(1.0, SessionTest::jog(4.0f));
    const TraceGen::Trace tr = g.finish();
    static Run::TrackSource src;
    src = Run::TrackSource();
    const Out o = drive(tr, src);
    EXPECT_TRUE(neverDecreases(o.totalM));
    size_t firstPaused = 0, lastPaused = 0;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        if (tr.pausedAt[i] != 0) {
            firstPaused = firstPaused == 0 ? i : firstPaused;
            lastPaused  = i;
        }
    }
    ASSERT_GT(lastPaused, firstPaused);
    const size_t base = tr.startIndex;
    ASSERT_GT(firstPaused, base) << "a reading before the pause";
    // Against the last reading before the pause: the first paused one adds
    // nothing either.
    EXPECT_NEAR(o.totalM[lastPaused - base], o.totalM[firstPaused - 1 - base], 1.0f) << "nothing counts while paused";
}

TEST(TrackSource, TheElevationIsHeldOnATrack)
{
    // The barometer wandering by a few metres on a flat track: the record's
    // altitude stays where the run joined the track, and no climb counts.
    TraceGen::Generator g(TraceGen::Track(), OvalTest::noisy(1.5f), 9);
    g.begin(0.0f, 2);
    g.runLaps(4.0, SessionTest::jog(4.0f));
    const TraceGen::Trace tr = g.finish();
    std::vector<float> baro(tr.samples.size());
    for (size_t i = 0; i < baro.size(); ++i) {
        baro[i] = 50.0f + 3.0f * std::sin(static_cast<float>(i) / 20.0f);
    }
    static Run::TrackSource src;
    src = Run::TrackSource();
    const Out o = drive(tr, src, nullptr, &baro);
    ASSERT_FALSE(o.altM.empty());
    const float last = o.altM.back();
    ASSERT_TRUE(std::isfinite(last));
    size_t held = 0;
    for (float a : o.altM) {
        held += a == last ? 1u : 0u;
    }
    EXPECT_GT(held, o.altM.size() / 2) << "held for most of the run";
    EXPECT_LT(o.ascentM, 3.0f) << "the barometer's wander is not climbing on a track";
}

TEST(TrackSource, TheStartScreenFindsAStoredTrack)
{
    TraceGen::Track tk;
    Oval::TrackStore store;
    const uint8_t number = SessionTest::storeTrack(store, tk, 0.0, 0.0);
    ASSERT_NE(number, 0u);

    TraceGen::Generator g(tk, OvalTest::noisy(1.5f), 3);
    g.begin(0.0f, 1);
    g.runLaps(0.2, SessionTest::jog(4.0f));
    const TraceGen::Trace tr = g.finish();
    Run::TrackSource src;
    EXPECT_EQ(src.preRun(&store).status, Run::PreRun::Status::Unknown) << "no fix yet";
    for (size_t i = 0; i < tr.startIndex; ++i) {
        src.prime(tr.samples[i]);
    }
    const Run::PreRun p = src.preRun(&store);
    EXPECT_EQ(p.status, Run::PreRun::Status::Stored);
    EXPECT_EQ(p.trackNumber, number);
    EXPECT_EQ(src.preRun(nullptr).status, Run::PreRun::Status::New) << "no stored tracks: a new track";
}
