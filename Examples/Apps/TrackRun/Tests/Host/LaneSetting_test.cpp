// The runner's lane during a run (Oval::Session::setLane()), and
// leaving a track judged beyond its lanes (Oval::Tracker).

#include "SessionTestUtil.hpp"
#include "TraceCsv.hpp"


#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <vector>

using namespace SessionTest;
using Oval::Session;
using TraceGen::Pt;

namespace {

// ---------------------------------------------------------------------------
// A Tracker on a supplied track, a reading a second at a chosen offset.
// ---------------------------------------------------------------------------

struct TrackerDriver {
    Oval::LocalFrame f;
    Oval::TrackModel m;   // in f
    Oval::TrackModel geo; // supplied
    Oval::Tracker    t;
    Oval::LaneTable  table = Oval::LaneTable::outdoor();
    uint32_t         tMs   = 1000000u;
    float            tau   = 0.1f;
    double           dist  = 1000.0;
    Oval::Vec2       lastP;
    bool             haveP = false;
    int              offOn = 0, onOff = 0;
    bool             wasOff = false, everLeft = false;

    TrackerDriver()
    {
        const double lat = 51.5, lon = -0.12;
        geo = Oval::TrackModel::fromGeo(lat, lon, 20.0f, 36.5f, false, 50.0f);
        f.setOrigin(lat, lon);
        m = geo;
        m.placeIn(f);
    }
    Oval::Sample sampleAt(Oval::Vec2 p)
    {
        Oval::Sample s;
        s.tMs = tMs;
        f.toLatLon(p, s.lat, s.lon);
        s.fix  = true;
        s.hdop = -1.0f;
        if (haveP) {
            dist += Oval::length(p - lastP);
        }
        lastP = p;
        haveP = true;
        s.gpsDistM = static_cast<float>(dist);
        s.altM     = 50.0f;
        return s;
    }
    void begin(uint8_t lane)
    {
        const Oval::Vec2 p = m.pointAt(tau, table.lineOffsetM(lane));
        for (int i = 0; i < 6; ++i) {
            t.prime(sampleAt(p));
            tMs += 1000;
        }
        t.start(tMs - 500, lane, table, &geo);
    }
    // One second along the line laneOff (metres from the kerb) at v, lat off it.
    void step(float laneOff, float lat, float v = 5.0f)
    {
        const float len = m.lapLengthM(laneOff);
        tau = m.tauOf(std::fmod(m.sOf(tau, laneOff) + v, len), laneOff);
        tMs += 1000;
        t.update(sampleAt(m.pointAt(tau, laneOff, lat)));
        onOff += t.isOffTrack() && !wasOff ? 1 : 0;
        offOn += !t.isOffTrack() && wasOff ? 1 : 0;
        wasOff   = t.isOffTrack();
        everLeft = everLeft || t.hasLeft();
    }
};

} // namespace

// ---------------------------------------------------------------------------
TEST(OffLanes, EdgeLanesLeaveOnceOrStay)
{
    // A runner keeping a fixed offset beyond the edge lane (inside lane 1, or
    // outside lane 8) for 4 minutes: within offLanesM the track, beyond it
    // off once and left, never flipping between the two.
    struct Case {
        uint8_t lane;
        float   lat;
        bool    leaves;
    };
    const Case cases[] = {{1, -11.0f, false}, {1, -13.5f, true}, {1, -16.0f, true},
                          {8, 11.0f, false},  {8, 13.5f, true},  {8, 16.0f, true}};
    for (const Case& c : cases) {
        TrackerDriver d;
        d.begin(c.lane);
        const float off = d.table.lineOffsetM(c.lane);
        for (int i = 0; i < 180; ++i) {
            d.step(off, 0.0f);
        }
        d.onOff = d.offOn = 0;
        for (int i = 0; i < 240; ++i) {
            d.step(off, c.lat, 4.0f);
        }
        EXPECT_EQ(d.everLeft, c.leaves) << "lane " << int(c.lane) << " at " << c.lat << " m";
        EXPECT_LE(d.onOff, c.leaves ? 1 : 0) << "lane " << int(c.lane) << " at " << c.lat << " m";
        EXPECT_EQ(d.offOn, 0) << "lane " << int(c.lane) << " at " << c.lat << " m: no flipping back";
    }
}

TEST(OffLanes, AReadingOffBeforeAPauseIsForgotten)
{
    // One reading off the track, a long pause, then off the track after the
    // resume: the runner left after the resume, not before the pause.
    TrackerDriver d;
    d.begin(1);
    const float line1 = d.table.lineOffsetM(1);
    for (int i = 0; i < 40; ++i) {
        d.step(line1, 0.0f);
    }
    d.step(line1, -40.0f);
    ASSERT_FALSE(d.t.isOffTrack());
    d.t.pause(d.tMs + 500);
    d.tMs += 600000;
    const uint32_t resumeT = d.tMs + 200;
    d.t.resume(resumeT);
    for (int i = 0; i < 5 && !d.t.isOffTrack(); ++i) {
        d.step(line1, -40.0f);
    }
    ASSERT_TRUE(d.t.isOffTrack());
    EXPECT_GE(Oval::msDiff(d.t.offTrackFromTMs(), resumeT), 0) << "dated after the resume";
}

TEST(OffLanes, APauseInAShortSpellOffCreditsNothingWalked)
{
    // Off the track, then a long pause in which the runner walks round the
    // outside and back on: the way walked while paused is not credited,
    // whether or not they were already stepping back on at the pause.
    for (int onBefore : {0, 2}) {
        TrackerDriver d;
        d.begin(1);
        const float line1 = d.table.lineOffsetM(1);
        for (int i = 0; i < 40; ++i) {
            d.step(line1, 0.0f);
        }
        for (int i = 0; i < 5 && !d.t.isOffTrack(); ++i) {
            d.step(line1, -40.0f, 1.0f);
        }
        for (int i = 0; i < onBefore; ++i) {
            d.step(line1, 0.0f, 1.0f);
        }
        ASSERT_TRUE(d.t.isOffTrack());
        ASSERT_FALSE(d.t.hasLeft());
        d.t.pause(d.tMs + 500);
        const float before = d.t.measuredDistanceM();
        d.tau = d.m.tauOf(std::fmod(d.m.sOf(d.tau, line1) + 80.0f, d.m.lapLengthM(line1)), line1);
        d.tMs += 120000;
        d.t.resume(d.tMs + 200);
        for (int i = 0; i < 8; ++i) {
            d.step(line1, 0.0f, 1.0f);
        }
        ASSERT_FALSE(d.t.isOffTrack());
        EXPECT_LT(d.t.measuredDistanceM() - before, 30.0f) << "the 80 m walked while paused; on before " << onBefore;
    }
}

TEST(OffLanes, AWrongLaneLeavesWhereTheRunnerLeft)
{
    // Lane 8 set, the runner over in lane 1 for two laps, then off into the
    // infield: off once it is clear of the lanes, and left where they left
    // (not at the last reading on lane 8's line, laps back).
    for (int wrong = 0; wrong < 2; ++wrong) {
        TrackerDriver d;
        d.begin(wrong ? 8 : 1);
        const float l8 = d.table.lineOffsetM(8), l1 = d.table.lineOffsetM(1);
        const float first = wrong ? l8 : l1;
        for (int i = 0; i < 120; ++i) {
            d.step(first, 0.0f);
        }
        for (int i = 0; i < 6; ++i) {
            d.step(l1, (first - l1) * (1.0f - static_cast<float>(i + 1) / 6.0f));
        }
        for (int i = 0; i < 170; ++i) {
            d.step(l1, 0.0f);
        }
        const uint32_t leaveT = d.tMs;
        float lat = 0.0f;
        for (int i = 0; i < 60; ++i) {
            lat = lat - 1.5f < -30.0f ? -30.0f : lat - 1.5f;
            d.step(l1, lat, 0.2f);
        }
        EXPECT_TRUE(d.t.isOffTrack() || d.t.hasLeft()) << "wrong " << wrong;
        EXPECT_NEAR(Oval::msDiff(d.t.offTrackFromTMs(), leaveT) / 1000.0, 0.0, 15.0) << "wrong " << wrong;
    }
}

// ---------------------------------------------------------------------------
// The runner's lane in a Session
// ---------------------------------------------------------------------------

TEST(LaneSetting, ALaneGivenAgainEndsTheNoLaneBlock)
{
    // "No lane" and the lane again at once (a mis-tap put right), then a
    // detour 60 m off: the track is joined again by itself on coming back.
    for (uint32_t seed : {51u, 52u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.5f), seed);
        const uint8_t lane = 2;
        const double  tauIn = 0.3;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1500.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.runLaps(2.0, OvalTest::runner(4.5f));
        g.run(150.0, OvalTest::runner(4.5f));
        g.setLane(Session::kNoLane);
        g.setLane(lane);
        g.run(250.0, OvalTest::runner(4.5f));
        g.runLaps(1.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        const double tauOut = g.tauNow();
        g.road({g.trackPointAt(tauOut, lane, 60.0)}, OvalTest::steady(1.5f));
        g.stand(60.0);
        const double tauRe = g.tauAfter(tauOut, lane, 40.0);
        g.road({g.trackPointAt(tauRe, lane, 30.0), g.trackPointAt(g.tauAfter(tauRe, lane, -12.0), lane, 20.0),
                g.trackPointAt(tauRe, lane, 0.0)},
               OvalTest::steady(1.5f));
        g.joinTrack(tauRe, lane);
        g.runLaps(3.0, OvalTest::runner(4.5f));
        g.run(120.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        g.road(routeOut(g, lane, 1500.0, rng), jog(3.3f));
        const TraceGen::Trace tr = g.finish();
        Oval::TrackStore store;
        storeTrack(store, tk, -1.0, 2.0, lane);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        EXPECT_EQ(r.joins.size(), 2u) << "seed " << seed << ": joined on arrival and after the detour";
        EXPECT_EQ(r.runLaps, 6u) << "seed " << seed;
    }
}

TEST(LaneSetting, NoLaneAfterWalkingOffKeepsTheWayOff)
{
    // "No lane" set after the runner has gone off the track, before the
    // exit is confirmed: the road starts where they left, so its GPS
    // distance is the same as with no press.
    for (uint32_t seed : {61u, 62u}) {
        double road[2] = {0.0, 0.0};
        for (int press = 0; press < 2; ++press) {
            std::mt19937 rng(PortableRandom::seeded(seed));
            const TraceGen::Track tk = randomTrack(rng);
            TraceGen::Generator g(tk, OvalTest::noisy(2.5f), seed);
            const uint8_t lane = 3;
            g.begin(0.2f, lane);
            g.runLaps(4.3, OvalTest::runner(4.5f));
            g.leaveToRoad();
            const std::vector<Pt> away = routeOut(g, lane, 1000.0, rng);
            g.road({away.front()}, jog(3.3f));
            const Pt here = g.here();
            const Pt a1   = away.size() > 1 ? away[1] : Pt{here.u * 2, here.v * 2};
            const double dx = a1.u - here.u, dy = a1.v - here.v, len = std::sqrt(dx * dx + dy * dy);
            g.road({{here.u + dx / len * 50.0, here.v + dy / len * 15.0}}, jog(3.3f));
            if (press) {
                g.setLane(Session::kNoLane);
            }
            g.road(std::vector<Pt>(away.begin() + 1, away.end()), jog(3.3f));
            const TraceGen::Trace tr = g.finish();
            const Replay::SessionResult r = Replay::runSession(tr);
            road[press] = r.finalRoad;
            EXPECT_NEAR(r.finalRoad, tr.gpsRoadM.back(), 30.0) << "seed " << seed << " press " << press;
        }
        EXPECT_NEAR(road[1], road[0], 10.0) << "seed " << seed << ": the press costs no road";
    }
}

TEST(LaneSetting, ALaneAfterPoorGpsWaitsForAGoodReading)
{
    // A lane set on arriving at a track that is not stored, after the last
    // stretch of road had only poor readings: the visit begins at the next
    // good reading, not at the last good one hundreds of metres back.
    for (uint32_t seed : {71u, 72u, 73u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.5f), seed);
        const uint8_t lane = 2;
        const double  tauIn = 0.4;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1500.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(std::vector<Pt>(in.begin(), in.end() - 3), jog(3.3f));
        g.poorHdop(true, 5.0f, 6.0f);
        g.road(std::vector<Pt>(in.end() - 3, in.end()), jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.run(5.0, OvalTest::runner(4.5f));
        g.setLane(lane);
        g.run(20.0, OvalTest::runner(4.5f));
        g.poorHdop(false);
        g.runLaps(4.0, OvalTest::runner(4.5f));
        const TraceGen::Trace tr = g.finish();
        const Replay::SessionResult r = Replay::runSession(tr);
        EXPECT_EQ(r.changes.size(), 1u) << "seed " << seed << ": onto the track and staying there";
        EXPECT_GE(r.laps.size(), 3u) << "seed " << seed;
    }
}

namespace {
TraceGen::Track movedBy(const TraceGen::Track& t, double e, double n)
{
    Oval::LocalFrame f;
    f.setOrigin(t.centreLat, t.centreLon);
    TraceGen::Track m = t;
    f.toLatLon(Oval::Vec2{static_cast<float>(e), static_cast<float>(n)}, m.centreLat, m.centreLon);
    return m;
}
} // namespace

TEST(LaneSetting, APausedJoinRestartedOnANeighbourKeepsThePauseOut)
{
    // A lane set while paused near two stored tracks: the nearer (a turned
    // neighbour) is dropped and the visit starts again on the right one.
    // Lap 1 leaves the pause out.
    int restarted = 0;
    for (uint32_t seed = 1; seed <= 6; ++seed) {
        std::mt19937 rng(PortableRandom::seeded(4000u + seed));
        auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
        TraceGen::Track m;
        m.headingDeg = static_cast<float>(uni(0.0, 180.0));
        const double bearing = uni(0.0, 2.0 * 3.14159265358979);
        TraceGen::Track w = movedBy(m, 40.0 * PortableMath::cos(bearing), 40.0 * PortableMath::sin(bearing));
        w.headingDeg = m.headingDeg + (seed % 2 == 0 ? 30.0f : -30.0f);
        const Oval::TrackModel wm = Replay::truthModel(w);
        const Oval::TrackModel mm = Replay::truthModel(m);
        const uint8_t lane  = static_cast<uint8_t>(1 + seed % 8);
        const float   speed = static_cast<float>(uni(3.5, 5.5));
        const TraceGen::Noise n = OvalTest::noisy(static_cast<float>(uni(2.0, 4.0)));
        float from = -1.0f;
        for (int k = 0; k < 40 && from < 0.0f; ++k) {
            TraceGen::Generator probe(m, n, seed);
            probe.begin(0.025f * static_cast<float>(k), lane);
            const TraceGen::Trace pt = probe.finish();
            Oval::LocalFrame f;
            f.setOrigin(pt.startLat, pt.startLon);
            if (Oval::length(f.toLocal(wm.centreLat(), wm.centreLon()))
                < Oval::length(f.toLocal(mm.centreLat(), mm.centreLon())) - 5.0f) {
                from = 0.025f * static_cast<float>(k);
            }
        }
        ASSERT_GE(from, 0.0f);
        Oval::TrackStore store;
        store.saveNew(wm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u);
        const uint8_t mNum = store.saveNew(mm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u).number;
        TraceGen::Generator g(m, n, 5000u + seed);
        g.begin(from, lane);
        g.stand(2.0);
        g.pause();
        g.setLane(lane);
        g.stand(20.0);
        if (seed % 2 == 1) {
            // More Lap presses than the Session keeps actions: the pause
            // stays, and a press restarts lap 1 inside it.
            for (int k = 0; k < 40; ++k) {
                g.lapPress();
            }
        }
        g.stand(40.0);
        g.resume();
        g.runLaps(4.5, OvalTest::runner(speed));
        TraceGen::Trace tr = g.finish();
        tr.startLane = Session::kNoLane;
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        uint32_t dur1 = 0;
        for (const Session::Event& e : r.events) {
            if (e.type == Session::EventType::TrackLap && e.runLap == 1) {
                dur1 = e.track.lapDurationMs; // the refined one last
            }
        }
        if (!r.laps.empty() && r.laps[0].trackNumber == mNum && !tr.crossActiveMs.empty()) {
            ++restarted;
            EXPECT_NEAR(dur1 / 1000.0, tr.crossActiveMs[0] / 1000.0 - 2.0, 5.0) << "seed " << seed;
        }
    }
    EXPECT_GT(restarted, 0) << "some runs restarted on the right track";
}

TEST(LaneSetting, ARestartOnANeighbourKeepsTheLaneChanges)
{
    // As above, with the lane changed soon into the visit: restarted on the
    // right track, lap 1 is measured on each lane in turn, as run.
    int restarted = 0;
    for (uint32_t seed = 1; seed <= 16; ++seed) {
        std::mt19937 rng(PortableRandom::seeded(4000u + seed));
        auto uni = [&](double a, double b) { return PortableRandom::Uniform(a, b)(rng); };
        TraceGen::Track m;
        m.headingDeg = static_cast<float>(uni(0.0, 180.0));
        const double bearing = uni(0.0, 2.0 * 3.14159265358979);
        TraceGen::Track w = movedBy(m, 40.0 * PortableMath::cos(bearing), 40.0 * PortableMath::sin(bearing));
        w.headingDeg = m.headingDeg + (seed % 2 == 0 ? 30.0f : -30.0f);
        const Oval::TrackModel wm = Replay::truthModel(w);
        const Oval::TrackModel mm = Replay::truthModel(m);
        const uint8_t lane  = static_cast<uint8_t>(1 + seed % 8);
        const uint8_t lane2 = static_cast<uint8_t>(lane <= 4 ? lane + 4 : lane - 4);
        const float   speed = static_cast<float>(uni(3.5, 5.5));
        const TraceGen::Noise n = OvalTest::noisy(static_cast<float>(uni(2.0, 4.0)));
        float from = -1.0f;
        for (int k = 0; k < 40 && from < 0.0f; ++k) {
            TraceGen::Generator probe(m, n, seed);
            probe.begin(0.025f * static_cast<float>(k), lane);
            const TraceGen::Trace pt = probe.finish();
            Oval::LocalFrame f;
            f.setOrigin(pt.startLat, pt.startLon);
            if (Oval::length(f.toLocal(wm.centreLat(), wm.centreLon()))
                < Oval::length(f.toLocal(mm.centreLat(), mm.centreLon())) - 5.0f) {
                from = 0.025f * static_cast<float>(k);
            }
        }
        ASSERT_GE(from, 0.0f);
        Oval::TrackStore store;
        store.saveNew(wm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u);
        const uint8_t mNum = store.saveNew(mm, Oval::FitQuality{3.0f, 0.5f}, 1760000000u).number;
        TraceGen::Generator g(m, n, 5000u + seed);
        g.begin(from, lane);
        g.stand(2.0);
        g.setLane(lane);
        g.run(150.0, OvalTest::runner(speed));
        g.changeLane(lane2);
        g.runLaps(4.5, OvalTest::runner(speed));
        TraceGen::Trace tr = g.finish();
        tr.startLane = Session::kNoLane;
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        if (!r.laps.empty() && r.laps[0].trackNumber == mNum && !tr.lapCreditM.empty()) {
            ++restarted;
            EXPECT_NEAR(r.laps[0].track.lapLengthM, tr.lapCreditM[0], 6.0) << "seed " << seed;
        }
    }
    EXPECT_GT(restarted, 0) << "some runs restarted on the right track";
}

TEST(LaneSetting, ALaneFarBeyondAStoredTrackIsAWrongPick)
{
    // The runner on a track that is not stored, a stored one 120 m away:
    // a lane at Start where the stored one is the nearest within
    // storeNearM, but the runner is far beyond its lanes and never on them.
    // A wrong pick: the run carries on as a track that is not stored, laps
    // and all, and never goes to the road.
    int far = 0;
    for (int k = 0; k < 20; ++k) {
        TraceGen::Track m;
        const TraceGen::Track a = movedBy(m, 120.0, 0.0);
        Oval::TrackStore store;
        store.saveNew(Replay::truthModel(a), Oval::FitQuality{3.0f, 0.5f}, 1760000000u);
        TraceGen::Generator g(m, OvalTest::noisy(3.0f), 7000u + static_cast<uint32_t>(k));
        g.begin(0.05f * static_cast<float>(k), 3);
        g.runLaps(3.0, OvalTest::runner(4.5f));
        const TraceGen::Trace tr = g.finish();
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        bool road = false;
        for (uint8_t sg : r.segment) {
            road = road || sg == static_cast<uint8_t>(Session::Segment::Road);
        }
        far += r.runLaps >= 2 ? 1 : 0;
        EXPECT_FALSE(road) << "start " << k;
        EXPECT_GE(r.runLaps, 2u) << "start " << k;
    }
    EXPECT_GT(far, 0);
}

// ---------------------------------------------------------------------------
