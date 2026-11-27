// Oval::Session: road and track in one run (docs/OVAL.md).

#include "SessionTestUtil.hpp"

#include <gtest/gtest.h>

#include <cstdio>

using namespace SessionTest;
using Oval::Session;
using TraceGen::Pt;

namespace {

const char* segName(Session::Segment s)
{
    switch (s) {
    case Session::Segment::None:  return "None";
    case Session::Segment::Road:  return "Road";
    case Session::Segment::Track: return "Track";
    }
    return "?";
}

void printEvents(const Replay::SessionResult& r, const TraceGen::Trace& tr)
{
    for (size_t k = 0; k < r.events.size(); ++k) {
        const Session::Event& e = r.events[k];
        const double t = Oval::msDiff(e.tMs, tr.startMs) / 1000.0;
        switch (e.type) {
        case Session::EventType::SegmentChanged:
            std::printf("  %7.1f s  SegmentChanged %s -> %s (track %u) total %.1f\n", t, segName(e.from),
                        segName(e.segment), e.trackNumber, e.totalM);
            break;
        case Session::EventType::TrackJoined:
            std::printf("  %7.1f s  TrackJoined track %u lane %u\n", t, e.trackNumber, e.lane);
            break;
        case Session::EventType::RoadKilometre:
            std::printf("  %7.1f s  RoadKilometre %u\n", t, e.kilometre);
            break;
        case Session::EventType::TrackLap:
            if (e.track.type != Oval::Tracker::EventType::LapTimeRefined) {
                std::printf("  %7.1f s  Lap %u (run %u) %.2f m, %.1f s\n", t, e.track.lapNumber, e.runLap,
                            e.track.lapLengthM, e.track.lapDurationMs / 1000.0);
            }
            break;
        case Session::EventType::TrackEvent:
            break;
        }
    }
}

// The road-and-track session, started with no lane: road to the track, 6
// laps, 4 x 400 m with recoveries (a Lap press at each rep start), and road
// home. runnerSets: the runner sets the lane on arriving at the track (a
// track that is not stored is joined only so); else the run notices a
// stored track by itself.
TraceGen::Trace roadTrackRoadSession(uint32_t seed, const TraceGen::Track& tk, const TraceGen::Noise& nz, uint8_t lane,
                                     bool runnerSets = false, double roadM = 4000.0)
{
    std::mt19937 rng(PortableRandom::seeded(seed * 7919u + 17u));
    TraceGen::Generator g(tk, nz, seed);
    PortableRandom::Uniform u01(0.0, 1.0);
    const double tauIn = 4.0 * u01(rng);
    Pt home;
    const std::vector<Pt> in = routeIn(g, tauIn, lane, roadM, rng, home);
    g.beginAt(home, Session::kNoLane);
    g.road(in, jog(3.3f));
    g.joinTrack(tauIn, lane);
    if (runnerSets) {
        g.setLane(lane);
    }
    g.runLaps(6.0, OvalTest::runner(4.5f));
    g.run(30.0, jog(3.0f));
    for (int k = 0; k < 4; ++k) {
        g.lapPress();
        g.run(g.laneLengthM(lane), OvalTest::steady(6.0f));
        g.run(20.0, OvalTest::steady(2.0f));
        g.stand(60.0);
    }
    g.run(200.0, jog(3.0f));
    g.leaveToRoad();
    g.road(routeOut(g, lane, roadM, rng), jog(3.3f));
    return g.finish();
}

bool verbose()
{
    return std::getenv("TRACKRUN_SESSION_VERBOSE") != nullptr;
}

// The SegmentChanged events, as from-to pairs.
std::vector<std::pair<Session::Segment, Session::Segment>> changes(const Replay::SessionResult& r)
{
    std::vector<std::pair<Session::Segment, Session::Segment>> c;
    for (const Session::Event& e : r.changes) {
        c.push_back({e.from, e.segment});
    }
    return c;
}

// Road splits that stand: those before the next change of segment.
size_t kmsBetween(const Replay::SessionResult& r, uint32_t fromMs, uint32_t toMs)
{
    size_t n = 0;
    for (const Session::Event& e : r.kms) {
        if (Oval::msDiff(e.tMs, fromMs) >= 0 && (toMs == 0 || Oval::msDiff(toMs, e.tMs) > 0)) {
            ++n;
        }
    }
    return n;
}

// The kernel's GPS distance after Start as the Session reads it: each step
// cut to maxSpeedMps from the last time the total moved.
double cappedGps(const TraceGen::Trace& tr, float maxSpeedMps = 12.0f)
{
    bool     have = false;
    float    last = 0.0f;
    uint32_t lastT = 0;
    double   sum = 0.0;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        const Oval::Sample& s = tr.samples[i];
        if (!std::isfinite(s.gpsDistM)) {
            have = false;
            continue;
        }
        if (have) {
            const float d = s.gpsDistM - last;
            if (d != 0.0f) {
                const float cap = maxSpeedMps * 0.001f * static_cast<float>(Oval::msDiff(s.tMs, lastT));
                sum += d > 0.0f ? (d < cap ? d : cap) : 0.0f;
                lastT = s.tMs;
            }
        } else {
            lastT = s.tMs;
        }
        have = true;
        last = s.gpsDistM;
    }
    return sum;
}

// The checks every road-track-road run started with no lane must pass.
// autoJoins: each visit was a known track the run joined by itself
// (TrackJoined, on number); else the runner set the lane on arrival.
void checkRoadTrackRoad(const TraceGen::Trace& tr, const Replay::SessionResult& r, uint8_t lane, uint8_t number,
                        const char* what, bool autoJoins = true)
{
    SCOPED_TRACE(what);
    const float laneLen = Oval::LaneTable::outdoor().lapLengthM(lane);
    EXPECT_EQ(r.decreases, 0u) << "worst " << r.worstDecrease << " m";
    if (autoJoins) {
        ASSERT_EQ(r.joins.size(), tr.visits.size());
        for (const Session::Event& j : r.joins) {
            EXPECT_EQ(j.trackNumber, number);
        }
    } else {
        EXPECT_TRUE(r.joins.empty()) << "the runner's lane is no join noticed";
    }
    // Road from Start, then Road to Track and back for each visit.
    const auto c = changes(r);
    ASSERT_EQ(c.size(), 2u * tr.visits.size());
    for (size_t v = 0; v < tr.visits.size(); ++v) {
        EXPECT_EQ(c[2 * v].first, Session::Segment::Road);
        EXPECT_EQ(c[2 * v].second, Session::Segment::Track);
        EXPECT_EQ(c[2 * v + 1].second, Session::Segment::Road);
        // The join and the exit are where the runner really came on and
        // left, give or take the GPS (a few seconds).
        const double joinErr = Oval::msDiff(r.changes[2 * v].tMs, tr.visits[v].joinMs) / 1000.0;
        const double exitErr = Oval::msDiff(r.changes[2 * v + 1].tMs, tr.visits[v].exitMs) / 1000.0;
        EXPECT_NEAR(joinErr, 0.0, autoJoins ? 15.0 : 1.5) << "join of visit " << v;
        EXPECT_NEAR(exitErr, 0.0, 15.0) << "exit of visit " << v;
        // Laps in the visit: every one exact, as many as were run from the
        // join.
        const SegLaps s = lapsBetween(r, r.changes[2 * v].tMs, r.changes[2 * v + 1].tMs, laneLen);
        EXPECT_EQ(s.laps, tr.visits[v].laps) << "visit " << v;
        EXPECT_EQ(s.exact, s.laps) << "visit " << v;
    }
    // Run lap numbers carry on across visits.
    for (size_t k = 0; k < r.laps.size(); ++k) {
        EXPECT_EQ(r.laps[k].runLap, k + 1);
    }
    EXPECT_EQ(r.runLaps, r.laps.size());

    // Nothing counted twice or lost at a transition. Where the Session puts
    // a join or an exit a few seconds from the true one, the stretch between
    // is road for one and track for the other: road is then off by its GPS
    // distance, track by its track distance, and the total by the
    // difference. Each transition may also lose the few metres run straight
    // across the lanes, which are neither.
    Allowance a = allowance(tr, r);
    EXPECT_NEAR(r.finalRoad, tr.gpsRoadM.back(), 15.0 + a.gpsM) << "road against the GPS distance on the road";
    // Each Lap press, and each exit, closes a part-lap with the part-lap error
    // (docs/OVAL.md: 7 to 9 m, 95th percentile, at 4 to 5 m noise).
    size_t presses = 0;
    for (const TraceGen::Command& cmd : tr.commands) {
        presses += cmd.kind == TraceGen::CmdKind::LapPress ? 1u : 0u;
    }
    EXPECT_NEAR(r.finalTrack, tr.trueTrackM.back(), 10.0 + a.trackM + 8.0 * (presses + tr.visits.size()))
        << "track against the truth";
    EXPECT_NEAR(r.finalTotal, tr.gpsRoadM.back() + tr.trueTrackM.back(),
                10.0 + 10.0 * a.transitions + a.diffM + 8.0 * presses)
        << "total against the truth";
    // No jump at a transition beyond the GPS distance run since the time it
    // took effect (an exit credits the road since the runner left, which the
    // display held meanwhile): a double count would add the stretch again.
    for (size_t k = 0; k < r.changes.size(); ++k) {
        const size_t i = r.changeSample[k];
        if (i == 0) {
            continue;
        }
        double since = 0.0;
        for (size_t j = sampleAtOrAfter(tr, r.changes[k].tMs); j <= i; ++j) {
            since += gpsStep(tr, j);
        }
        EXPECT_LE(r.total[i] - r.total[i - 1], since + 10.0) << "the total jumped at change " << k;
    }

    // Road splits: one a kilometre of each road segment, none voided.
    uint32_t from = tr.startMs;
    float    roadBefore = 0.0f;
    for (size_t v = 0; v <= tr.visits.size(); ++v) {
        const bool     more  = v < tr.visits.size();
        const uint32_t to    = more ? r.changes[2 * v].tMs : 0;
        const float    roadM = (more ? r.road[r.changeSample[2 * v]] : r.finalRoad) - roadBefore;
        const size_t   want  = static_cast<size_t>(std::floor(roadM / 1000.0f));
        const size_t   got   = kmsBetween(r, from, to);
        const bool     edge  = std::fabs(roadM - 1000.0f * std::round(roadM / 1000.0f)) < 5.0f;
        if (!edge) {
            EXPECT_EQ(got, want) << "road splits in road segment " << v << " (" << roadM << " m)";
        }
        if (more) {
            // The next road segment starts at the exit: the road before it
            // is what stood while on the track.
            from       = r.changes[2 * v + 1].tMs;
            roadBefore = r.road[r.changeSample[2 * v + 1] - 1];
        }
    }

    // Elevation held on the track only.
    size_t holdOnRoad = 0;
    for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
        if (r.hold[i] && r.segment[i] != static_cast<uint8_t>(Session::Segment::Track)) {
            ++holdOnRoad;
        }
        if (!tr.onTrack[i] && r.segment[i] == static_cast<uint8_t>(Session::Segment::Road) && r.hold[i]) {
            ++holdOnRoad;
        }
    }
    EXPECT_EQ(holdOnRoad, 0u);
}

void report(const char* what, const TraceGen::Trace& tr, const Replay::SessionResult& r)
{
    std::printf("[ session  ] %-26s total %8.1f (truth %8.1f)  road %7.1f (GPS %7.1f, true %7.1f)  "
                "track %7.1f (truth %7.1f)  laps %u\n",
                what, r.finalTotal, tr.gpsRoadM.back() + tr.trueTrackM.back(), r.finalRoad, tr.gpsRoadM.back(),
                tr.trueRoadM.back(), r.finalTrack, tr.trueTrackM.back(), r.runLaps);
    if (verbose()) {
        printEvents(r, tr);
    }
}

} // namespace

TEST(Session, RoadTrackRoadOnAStoredTrack)
{
    // 4 km of road, 6 laps and 4 x 400 m on a stored track, 4 km home, with
    // no input: the track is joined and left by itself.
    for (uint32_t seed : {1u, 2u, 3u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        const TraceGen::Trace tr = roadTrackRoadSession(seed, tk, OvalTest::noisy(2.0f + seed), 1);
        Oval::TrackStore store;
        const uint8_t number = storeTrack(store, tk, 2.0, -1.5);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        char what[64];
        std::snprintf(what, sizeof what, "road-track-road, stored, seed %u", seed);
        report(what, tr, r);
        checkRoadTrackRoad(tr, r, 1, number, what);
        EXPECT_EQ(r.runLaps, 10u);
        EXPECT_EQ(r.outcomeNumber, number);
        EXPECT_EQ(r.autoJoins, 1u);
        // Road against the route's true length: the GPS model's own over-read.
        const double over = r.finalRoad / tr.trueRoadM.back() - 1.0;
        const double model = tr.gpsRoadM.back() / tr.trueRoadM.back() - 1.0;
        EXPECT_NEAR(over, model, 0.005) << "road distance is the GPS distance";
    }
}

TEST(Session, RoadTrackRoadOnATrackNotStored)
{
    // The same on a track the watch has never stored: the runner sets the
    // lane on arriving. Laps count from there, the track is learnt (locked at
    // lap 4, stored as new) and left by itself.
    for (uint32_t seed : {11u, 12u, 13u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        const TraceGen::Trace tr = roadTrackRoadSession(seed, tk, OvalTest::noisy(2.0f + (seed - 10)), 1, true);
        const Replay::SessionResult r = Replay::runSession(tr);
        char what[64];
        std::snprintf(what, sizeof what, "road-track-road, not stored, seed %u", seed);
        report(what, tr, r);
        checkRoadTrackRoad(tr, r, 1, 0, what, false);
        EXPECT_EQ(r.runLaps, 10u);
        EXPECT_EQ(r.outcome.kind, Oval::StoreOutcome::Kind::New) << "locked at lap 4 and stored as new";
        EXPECT_TRUE(r.outcome.model.isValid() && r.outcome.model.hasGeo());
        EXPECT_EQ(r.outcome.lane, 1u);
    }
}

TEST(Session, ALaneSetOnTheRoadJoinsTheStoredTrackThere)
{
    // A lane set on arriving at a stored track, before the run notices it:
    // the stored track, from there, with the lane given.
    for (uint32_t seed : {4u, 5u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        const TraceGen::Trace tr = roadTrackRoadSession(seed, tk, OvalTest::noisy(3.0f), 3, true);
        Oval::TrackStore store;
        const uint8_t number = storeTrack(store, tk, 2.0, -1.5, 1);
        uint8_t onTrack = 0xFF;
        const Replay::SessionResult r = Replay::runSession(
            tr, Oval::LaneTable::outdoor(), Oval::Config(), &store, [&](size_t, Session& s) {
                if (s.segment() == Session::Segment::Track && onTrack == 0xFF) {
                    onTrack = s.trackNumber();
                    EXPECT_EQ(s.lane(), 3u) << "the runner's lane, not the one stored";
                }
            });
        char what[64];
        std::snprintf(what, sizeof what, "lane set at a stored track, seed %u", seed);
        report(what, tr, r);
        EXPECT_EQ(onTrack, number) << what;
        checkRoadTrackRoad(tr, r, 3, number, what, false);
        EXPECT_EQ(r.outcomeNumber, number);
        EXPECT_EQ(r.outcome.lane, 3u);
    }
}

TEST(Session, NoLaneOnATrackIsTheRoadUntilAway)
{
    // On a stored track, "No lane" mid-lap: the road from there, and 2 more
    // laps on the track (a warm-down) are not joined again. Once away (400 m)
    // and back, the track is joined by itself again.
    for (uint32_t seed : {101u, 102u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.5f), seed);
        const uint8_t lane = 2;
        const double  tauIn = 0.6;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1500.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.runLaps(3.0, OvalTest::runner(4.5f));
        g.run(100.0, OvalTest::runner(4.5f));
        g.setLane(Session::kNoLane);
        g.runLaps(2.0, jog(3.0f));
        g.leaveToRoad();
        const double tauOut = g.tauNow();
        g.road(routeOut(g, lane, 400.0, rng), jog(3.3f));
        g.stand(30.0);
        const double tauRe = g.tauAfter(tauOut, lane, 60.0);
        g.road({g.trackPointAt(tauRe, lane, 60.0), g.trackPointAt(g.tauAfter(tauRe, lane, -12.0), lane, 20.0),
                g.trackPointAt(tauRe, lane, 0.0)},
               jog(3.3f));
        g.joinTrack(tauRe, lane);
        g.runLaps(2.0, OvalTest::runner(4.5f));
        g.run(100.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        g.road(routeOut(g, lane, 800.0, rng), jog(3.3f));
        const TraceGen::Trace tr = g.finish();

        Oval::TrackStore store;
        const uint8_t number = storeTrack(store, tk, 1.0, -1.0, lane);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        char what[64];
        std::snprintf(what, sizeof what, "no lane on a track, seed %u", seed);
        report(what, tr, r);
        EXPECT_EQ(r.decreases, 0u) << what;
        ASSERT_EQ(r.joins.size(), 2u) << what << ": joined on arrival and on coming back, not in between";
        EXPECT_EQ(r.joins[0].trackNumber, number);
        EXPECT_EQ(r.joins[1].trackNumber, number);
        const auto c = changes(r);
        ASSERT_EQ(c.size(), 4u) << what;
        // The first visit ends at the "No lane", not where the runner left.
        uint32_t noLaneMs = 0;
        for (const TraceGen::Command& cmd : tr.commands) {
            if (cmd.kind == TraceGen::CmdKind::SetLane && cmd.lane == Session::kNoLane) {
                noLaneMs = cmd.tMs;
            }
        }
        EXPECT_NEAR(Oval::msDiff(r.changes[1].tMs, noLaneMs) / 1000.0, 0.0, 1.5) << what;
        EXPECT_EQ(r.runLaps, 5u) << what << ": 3 laps, none in the warm-down, then 2";
    }
}

TEST(Session, NoLaneAtStartIsTheRoad)
{
    // Start with no lane on a track that is not stored, and 4 laps: the run
    // never takes the loop for a track by itself. The GPS distance, no laps.
    for (uint32_t seed : {25u, 26u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(3.0f), seed);
        g.begin(0.2f, 2);
        g.runLaps(4.0, OvalTest::runner(5.0f));
        TraceGen::Trace tr = g.finish();
        tr.startLane = Session::kNoLane;

        const Replay::SessionResult r = Replay::runSession(tr);
        EXPECT_TRUE(r.laps.empty()) << "seed " << seed;
        EXPECT_TRUE(r.changes.empty()) << "seed " << seed;
        EXPECT_EQ(r.finalTrack, 0.0f);
        EXPECT_NEAR(r.finalTotal, cappedGps(tr), 0.05) << "the GPS distance";
        EXPECT_EQ(r.segment.back(), static_cast<uint8_t>(Session::Segment::Road));
    }
}

TEST(Session, StartWithALaneOnATrackNotStoredMatchesTracker)
{
    // No store, a lane at Start: counting from lap 1 exactly as a Tracker on
    // its own, reading for reading, with a pause and a lane change.
    for (uint32_t seed : {21u, 22u, 23u, 24u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(3.0f), seed);
        g.begin(0.1f * static_cast<float>(seed % 10), 2);
        g.runLaps(3.3, OvalTest::runner(5.0f));
        g.pause();
        g.stand(20.0);
        g.resume();
        g.changeLane(4);
        g.runLaps(5.2, OvalTest::runner(4.0f));
        const TraceGen::Trace tr = g.finish();

        const Replay::SessionResult s = Replay::runSession(tr);
        const Replay::TrackerResult t = Replay::runTracker(tr);
        ASSERT_EQ(s.laps.size(), t.laps.size()) << "seed " << seed;
        for (size_t k = 0; k < t.laps.size(); ++k) {
            EXPECT_EQ(s.laps[k].track.tMs, t.laps[k].tMs);
            EXPECT_EQ(s.laps[k].track.lapLengthM, t.laps[k].lapLengthM);
            EXPECT_EQ(s.laps[k].track.type, t.laps[k].type);
        }
        size_t differ = 0;
        for (size_t i = 0; i < tr.samples.size(); ++i) {
            differ += s.total[i] != t.total[i] ? 1u : 0u;
        }
        EXPECT_EQ(differ, 0u) << "the total, reading for reading";
        EXPECT_TRUE(s.changes.empty()) << "a track from Start";
        EXPECT_TRUE(s.joins.empty());
        EXPECT_EQ(s.finalRoad, 0.0f);
        EXPECT_EQ(s.segment.back(), static_cast<uint8_t>(Session::Segment::Track));
    }
}

TEST(Session, RoadOnlyNeverFindsATrack)
{
    // From home with no lane and no track: a park loop 4 times, laps of a
    // roundabout, a figure of eight, an out-and-back and laps of a car park.
    // Distance is the GPS distance, no laps, no track.
    for (uint32_t seed : {31u, 32u, 33u, 34u, 35u, 36u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.0f + 0.5f * static_cast<float>(seed - 31)), seed);
        const Pt home{-2500.0, 1800.0};
        g.beginAt(home, Session::kNoLane);
        std::vector<Pt> route{{-2200.0, 1800.0}, {-2200.0, 1450.0}};
        g.road(route, jog(3.3f));
        g.road(loopShape(parkLoop(400.0), g.here(), 4), jog(3.5f));
        g.road({{-1900.0, 1450.0}}, jog(3.3f));
        g.road(loopShape(circle(20.0), g.here(), 5), jog(3.0f));
        g.road({{-1600.0, 1450.0}, {-1600.0, 1150.0}}, jog(3.3f));
        g.road(loopShape(figureEight(45.0), g.here(), 3), jog(3.5f));
        g.road({{-1300.0, 1150.0}}, jog(3.3f));
        for (int k = 0; k < 2; ++k) {
            g.road({{-1000.0, 1150.0}, {-1300.0, 1150.0}}, jog(3.3f));
        }
        g.road({{-1300.0, 850.0}}, jog(3.3f));
        g.road(loopShape(carPark(120.0, 80.0), g.here(), 4), jog(3.5f));
        g.road({{-2500.0, 850.0}, home}, jog(3.3f));
        const TraceGen::Trace tr = g.finish();

        const Replay::SessionResult r = Replay::runSession(tr);
        char what[64];
        std::snprintf(what, sizeof what, "road only, seed %u", seed);
        report(what, tr, r);
        EXPECT_EQ(r.decreases, 0u);
        EXPECT_TRUE(r.joins.empty()) << what;
        EXPECT_TRUE(r.laps.empty()) << what;
        EXPECT_EQ(r.runLaps, 0u);
        EXPECT_NEAR(r.finalTotal, cappedGps(tr), 0.05) << "the GPS distance";
        EXPECT_EQ(r.segment.back(), static_cast<uint8_t>(Session::Segment::Road));
    }
}

TEST(Session, PathAlongsideTheTrackIsNotJoined)
{
    // A path 15 to 25 m outside lane 8's outer edge, past a stored track
    // along a straight and on round a bend: never taken for the track.
    size_t runs = 0;
    for (double outside : {15.0, 20.0, 25.0}) {
        for (uint32_t seed : {41u, 42u, 43u, 44u}) {
            std::mt19937 rng(PortableRandom::seeded(seed));
            TraceGen::Track tk = randomTrack(rng);
            TraceGen::Generator g(tk, OvalTest::noisy(2.0f + 0.75f * static_cast<float>(seed - 41)), seed);
            const double out = 9.76 - 0.30 + outside; // from lane 1's line
            std::vector<Pt> path;
            for (int i = 0; i <= 60; ++i) {
                path.push_back(g.trackPointAt(std::fmod(3.6 + 0.4 * i / 60.0, 4.0), 1, out)); // in along the second bend's end
            }
            for (int i = 0; i <= 40; ++i) {
                path.push_back(g.trackPointAt(std::fmod(0.0 + 2.0 * i / 40.0, 4.0), 1, out)); // straight and bend
            }
            Pt home;
            std::vector<Pt> in = routeIn(g, 3.6, 1, 1500.0, rng, home);
            in.pop_back(); // not onto the track: onto the path instead
            in.pop_back();
            g.beginAt(home, Session::kNoLane);
            g.road(in, jog(3.3f));
            g.road(path, jog(3.5f));
            g.road(legsAway(g.here(), outwardAt(g, 2.0, 1), 1500.0, rng), jog(3.3f));
            const TraceGen::Trace tr = g.finish();

            Oval::TrackStore store;
            storeTrack(store, tk, 0.0, 0.0);
            const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
            EXPECT_TRUE(r.joins.empty()) << outside << " m outside, seed " << seed;
            EXPECT_TRUE(r.laps.empty());
            EXPECT_EQ(r.decreases, 0u);
            ++runs;
        }
    }
    std::printf("[ session  ] path alongside: %zu runs, none joined\n", runs);
}

TEST(Session, ToiletDetourIsRoadAndLapsRestart)
{
    // A 3 minute, 300 m detour mid-session: road distance, then a rejoin,
    // from which laps count again; the run's lap count carries on.
    for (uint32_t seed : {51u, 52u, 53u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.0f + static_cast<float>(seed - 51)), seed);
        const uint8_t lane = 2;
        const double  tauIn = 0.3;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1500.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.runLaps(3.0, OvalTest::runner(4.5f));
        g.run(150.0, OvalTest::runner(4.5f));
        // Off to the toilet: across the lanes, 150 m away, a minute there,
        // and back onto the track somewhere else.
        g.leaveToRoad();
        const double tauOut = g.tauNow();
        std::vector<Pt> away = routeOut(g, lane, 150.0, rng);
        g.road(away, OvalTest::steady(1.5f));
        g.stand(60.0);
        const double tauRe = g.tauAfter(tauOut, lane, 90.0);
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
        const uint8_t number = storeTrack(store, tk, -1.0, 2.0, lane);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        char what[64];
        std::snprintf(what, sizeof what, "toilet detour, seed %u", seed);
        report(what, tr, r);
        checkRoadTrackRoad(tr, r, lane, number, what);
        EXPECT_EQ(r.runLaps, 6u);
    }
}

TEST(Session, AJoinAfterAPauseOfHoursIsTimedRight)
{
    // A road run paused for two hours 30 m before a stored track, then on
    // onto it: the join is timed from the ring's points either side of the
    // pause, and must be timed where it happened, never after the reading
    // that raised it.
    const uint32_t seed = 71u;
    std::mt19937 rng(PortableRandom::seeded(seed));
    const TraceGen::Track tk = randomTrack(rng);
    TraceGen::Generator g(tk, OvalTest::noisy(2.0f), seed);
    const uint8_t lane = 1;
    const double  tauIn = 0.3;
    Pt home;
    const std::vector<Pt> in = routeIn(g, tauIn, lane, 1200.0, rng, home);
    g.beginAt(home, Session::kNoLane);
    std::vector<Pt> approach(in.begin(), in.end() - 1);
    approach.push_back(g.trackPointAt(tauIn - 0.03, lane, 25.0)); // 25 m out, a little before the entry
    g.road(approach, jog(3.3f));
    g.pause();
    g.stand(2.0 * 3600.0);
    g.resume();
    g.road({g.trackPointAt(tauIn, lane, 0.0)}, jog(3.3f));
    g.joinTrack(tauIn, lane);
    g.runLaps(3.0, OvalTest::runner(4.5f));
    g.leaveToRoad();
    g.road(routeOut(g, lane, 600.0, rng), jog(3.3f));
    const TraceGen::Trace tr = g.finish();

    Oval::TrackStore store;
    storeTrack(store, tk, 1.0, -1.0);
    const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
    ASSERT_FALSE(r.changes.empty());
    ASSERT_FALSE(tr.visits.empty());
    for (size_t k = 0; k < r.changes.size(); ++k) {
        EXPECT_LE(Oval::msDiff(r.changes[k].tMs, tr.samples[r.changeSample[k]].tMs), 0)
            << "change " << k << " timed after the reading that raised it";
    }
    // The join, timed where the runner came onto the track.
    size_t join = r.changes.size();
    for (size_t k = 0; k < r.changes.size() && join == r.changes.size(); ++k) {
        join = r.changes[k].segment == Session::Segment::Track ? k : join;
    }
    ASSERT_LT(join, r.changes.size()) << "joined";
    EXPECT_NEAR(static_cast<double>(Oval::msDiff(r.changes[join].tMs, tr.visits[0].joinMs)), 0.0, 30000.0);
    EXPECT_EQ(r.runLaps, 3u);
}

TEST(Session, APauseJustAfterArrivingStaysOutOfTheLaps)
{
    // Road onto a stored track, a 5 minute pause 20 m on (before the run has
    // noticed the track), then on. The join is replayed from where the
    // runner came on: the pause is no lap's running time.
    for (uint32_t seed : {81u, 82u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.0f), seed);
        const uint8_t lane = 1;
        const double  tauIn = 0.3;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1200.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.run(20.0, OvalTest::runner(4.5f));
        g.pause();
        g.stand(300.0);
        g.resume();
        g.runLaps(4.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        g.road(routeOut(g, lane, 600.0, rng), jog(3.3f));
        const TraceGen::Trace tr = g.finish();
        Oval::TrackStore store;
        storeTrack(store, tk, 1.0, -1.0);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        ASSERT_EQ(r.joins.size(), 1u) << "seed " << seed << ": the track was joined";
        ASSERT_FALSE(r.laps.empty()) << "seed " << seed;
        for (const Session::Event& e : r.laps) {
            EXPECT_LT(e.track.lapDurationMs, 200000u) << "seed " << seed << ": a lap of about 90 s holds the pause";
        }
    }
}

TEST(Session, OneWildFixFarAwayChangesNothing)
{
    // A single fix 5 km away (the receiver's wild reading) just before a
    // stored track: the run goes on as without it, the track joined and
    // every lap counted.
    const uint32_t seed = 2u;
    std::mt19937 rng(PortableRandom::seeded(seed));
    const TraceGen::Track tk = randomTrack(rng);
    TraceGen::Trace tr = roadTrackRoadSession(seed, tk, OvalTest::noisy(2.5f), 1);
    Oval::TrackStore store;
    storeTrack(store, tk, 2.0, -1.5);
    const Replay::SessionResult clean = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
    ASSERT_FALSE(tr.visits.empty());
    const size_t k = tr.visits[0].joinSample > 20 ? tr.visits[0].joinSample - 20 : tr.startIndex + 1;
    tr.samples[k].lat += 0.045; // about 5 km north
    const Replay::SessionResult wild = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
    EXPECT_EQ(wild.runLaps, clean.runLaps);
    EXPECT_EQ(wild.joins.size(), clean.joins.size());
    EXPECT_NEAR(wild.finalTotal, clean.finalTotal, 30.0f);
}

TEST(Session, APauseOffTheTrackKeepsTheVisit)
{
    // Paused mid-lap, the runner walks 60 m off to a tap, stands, walks back
    // to where they stopped and resumes. Nothing leaves the track while
    // paused: one visit, the lap in progress carries on, every lap counted.
    for (uint32_t seed : {61u, 62u, 63u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.5f), seed);
        const uint8_t lane = 1;
        const double  tauIn = 0.3;
        Pt home;
        const std::vector<Pt> in = routeIn(g, tauIn, lane, 1500.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(tauIn, lane);
        g.runLaps(2.0, OvalTest::runner(4.5f));
        g.run(150.0, OvalTest::runner(4.5f));
        g.pause();
        g.leaveToRoad();
        const double tauOut = g.tauNow();
        g.road(routeOut(g, lane, 60.0, rng), OvalTest::steady(1.2f));
        g.stand(30.0);
        g.road({g.trackPointAt(tauOut, lane, 20.0), g.trackPointAt(tauOut, lane, 0.0)}, OvalTest::steady(1.2f));
        g.joinTrack(tauOut, lane);
        g.resume();
        g.run(250.0, OvalTest::runner(4.5f));
        g.runLaps(2.0, OvalTest::runner(4.5f));
        g.run(100.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        g.road(routeOut(g, lane, 1500.0, rng), jog(3.3f));
        const TraceGen::Trace tr = g.finish();

        Oval::TrackStore store;
        storeTrack(store, tk, 1.0, -1.0);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        char what[64];
        std::snprintf(what, sizeof what, "pause off the track, seed %u", seed);
        report(what, tr, r);
        EXPECT_EQ(r.joins.size(), 1u) << what << ": one visit";
        EXPECT_EQ(r.runLaps, 5u) << what;
    }
}

TEST(Session, LaneOnArrival)
{
    std::mt19937 rng(PortableRandom::seeded(61));
    const TraceGen::Track tk = randomTrack(rng);
    // Road with no lane onto the track in trackLane; changeTo, if not 0, is
    // set 80 m after arriving (after the notice).
    auto build = [&](uint32_t seed, uint8_t trackLane, uint8_t changeTo) {
        std::mt19937 r2(PortableRandom::seeded(seed));
        TraceGen::Generator g(tk, OvalTest::noisy(3.0f), seed);
        Pt home;
        const std::vector<Pt> in = routeIn(g, 1.5, trackLane, 1500.0, r2, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.3f));
        g.joinTrack(1.5, trackLane);
        if (changeTo != 0) {
            g.run(80.0, OvalTest::runner(4.5f));
            g.setLane(changeTo);
            g.runLaps(5.0, OvalTest::runner(4.5f));
        } else {
            g.runLaps(5.0, OvalTest::runner(4.5f));
        }
        g.run(200.0, OvalTest::runner(4.5f));
        g.leaveToRoad();
        g.road(routeOut(g, trackLane, 500.0, r2), jog(3.3f));
        return g.finish();
    };

    // A stored track last run in lane 3, run in lane 3: joined on lane 3
    // with no input.
    {
        const TraceGen::Trace tr = build(62, 3, 0);
        Oval::TrackStore store;
        const uint8_t number = storeTrack(store, tk, 1.0, 1.0, 3);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        ASSERT_EQ(r.joins.size(), 1u);
        EXPECT_EQ(r.joins[0].lane, 3u) << "the lane last run there";
        checkRoadTrackRoad(tr, r, 3, number, "lane 3, no input");
        EXPECT_EQ(r.outcome.lane, 3u);
    }
    // A stored track last run in lane 1; today lane 5: joined on lane 1, the
    // runner sets lane 5 after the notice, and it stands from there.
    {
        const TraceGen::Trace tr = build(63, 5, 5);
        Oval::TrackStore store;
        storeTrack(store, tk, 1.0, 1.0, 1);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        report("stored, lane 5 set after", tr, r);
        ASSERT_EQ(r.joins.size(), 1u);
        EXPECT_EQ(r.joins[0].lane, 1u) << "joined on the lane last run there";
        EXPECT_EQ(r.runLaps, 5u);
        EXPECT_EQ(r.outcome.lane, 5u) << "the runner's lane from there on";
        EXPECT_EQ(r.decreases, 0u);
    }
    // A stored track last run in lane 1, run today in lane 8 with the day's
    // GPS 5 m outward, and no lane set: lane 8 is on the track, so it is one
    // visit, with every lap counted. Leaving is placed later than on the
    // runner's own lane (no reading is close to lane 1's line), within 30 s.
    for (uint32_t seed : {65u, 66u, 67u}) {
        const TraceGen::Trace tr = build(seed, 8, 0);
        Oval::TrackStore store;
        storeTrack(store, tk, 3.5, 3.5, 1);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        report("stored lane 1, run in lane 8", tr, r);
        ASSERT_EQ(r.joins.size(), 1u) << "seed " << seed << ": one visit";
        EXPECT_EQ(r.joins[0].lane, 1u);
        ASSERT_EQ(r.changes.size(), 2u) << "seed " << seed << ": no exit until the runner leaves";
        EXPECT_NEAR(Oval::msDiff(r.changes[1].tMs, tr.visits[0].exitMs) / 1000.0, 0.0, 30.0) << "seed " << seed;
        EXPECT_EQ(r.runLaps, tr.visits[0].laps) << "seed " << seed;
        EXPECT_EQ(r.decreases, 0u);
    }
}

TEST(Session, LapPressRestartsTheLap)
{
    // On a stored track from Start: a Lap press at 1.3 laps, and the next lap
    // ends a full lap from the press.
    for (uint32_t seed : {71u, 72u, 73u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(3.0f), seed);
        g.begin(0.25f, 1);
        g.runLaps(1.3, OvalTest::runner(5.0f));
        g.lapPress();
        g.runLaps(1.0, OvalTest::runner(5.0f));
        g.run(40.0, OvalTest::runner(5.0f));
        const TraceGen::Trace tr = g.finish();

        Oval::TrackStore store;
        storeTrack(store, tk, 0.0, 0.0);
        const Replay::SessionResult r = Replay::runSession(tr, Oval::LaneTable::outdoor(), Oval::Config(), &store);
        ASSERT_EQ(tr.crossMs.size(), 2u);
        ASSERT_EQ(r.laps.size(), 2u) << "seed " << seed;
        for (size_t k = 0; k < 2; ++k) {
            EXPECT_NEAR(r.laps[k].track.lapLengthM, 400.0f, 0.01f);
            EXPECT_NEAR(Oval::msDiff(r.laps[k].track.tMs, tr.crossMs[k]) / 1000.0, 0.0, 2.5) << "lap " << k;
        }
        EXPECT_EQ(r.decreases, 0u);
        EXPECT_NEAR(r.finalTotal, tr.trueTrackM.back(), 10.0);
    }
}

TEST(Session, TrackNotStoredInLane8WithGpsDistanceOver)
{
    // Lane 8 (453 m) with GPS distance 12% over, jogged slowly, set on
    // arrival at a track that is not stored: every lap exact, left by
    // itself.
    for (uint32_t seed : {81u, 82u, 83u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Noise n = OvalTest::noisy(3.0f, 0.1f, 0.3f);
        n.distScale = 1.12f;
        TraceGen::Generator g(tk, n, seed);
        Pt home;
        const std::vector<Pt> in = routeIn(g, 2.5, 8, 1200.0, rng, home);
        g.beginAt(home, Session::kNoLane);
        g.road(in, jog(3.0f));
        g.joinTrack(2.5, 8);
        g.setLane(8);
        g.runLaps(4.0, jog(2.6f));
        g.run(150.0, jog(2.6f));
        g.leaveToRoad();
        g.road(routeOut(g, 8, 400.0, rng), jog(3.0f));
        const TraceGen::Trace tr = g.finish();
        const double over = (tr.gpsDistM / (tr.trueRoadM.back() + 4.0 * 453.03 + 150.0)) - 1.0;

        const Replay::SessionResult r = Replay::runSession(tr);
        char what[64];
        std::snprintf(what, sizeof what, "lane 8, GPS %+.0f%%, seed %u", 100.0 * over, seed);
        report(what, tr, r);
        checkRoadTrackRoad(tr, r, 8, 0, what, false);
    }
}

TEST(Session, NoLaneFromACarParkThenALaneOnTheTrack)
{
    // Start pressed with no lane in a car park about 100 m from a track that
    // is not stored, a jog onto the track, the lane set there, then 6 laps.
    for (uint32_t seed : {91u, 92u, 93u, 94u}) {
        std::mt19937 rng(PortableRandom::seeded(seed));
        const TraceGen::Track tk = randomTrack(rng);
        TraceGen::Generator g(tk, OvalTest::noisy(2.0f + 0.75f * static_cast<float>(seed - 91)), seed);
        const double tauIn = 0.2;
        const Pt carPark = g.trackPointAt(0.5, 1, 100.0);
        g.beginAt(carPark, Session::kNoLane);
        g.stand(20.0);
        g.road({g.trackPointAt(0.1, 1, 30.0), g.trackPointAt(g.tauAfter(tauIn, 1, -12.0), 1, 20.0),
                g.trackPointAt(tauIn, 1, 0.0)},
               jog(2.5f));
        g.joinTrack(tauIn, 1);
        g.setLane(1);
        g.runLaps(6.0, OvalTest::runner(4.5f));
        g.run(150.0, OvalTest::runner(4.0f));
        const TraceGen::Trace tr = g.finish();

        const Replay::SessionResult r = Replay::runSession(tr);
        char what[64];
        std::snprintf(what, sizeof what, "car park, no lane, seed %u", seed);
        report(what, tr, r);
        EXPECT_EQ(r.decreases, 0u);
        EXPECT_EQ(r.segment.back(), static_cast<uint8_t>(Session::Segment::Track)) << what;
        ASSERT_EQ(r.changes.size(), 1u);
        const Session::Event& toTrack = r.changes.back();
        ASSERT_EQ(toTrack.segment, Session::Segment::Track);
        EXPECT_NEAR(Oval::msDiff(toTrack.tMs, tr.visits[0].joinMs) / 1000.0, 0.0, 1.5) << what;
        const SegLaps s = lapsBetween(r, toTrack.tMs, 0, 400.0f);
        EXPECT_EQ(s.laps, 6u) << what;
        EXPECT_EQ(s.exact, s.laps);
        EXPECT_NEAR(r.finalTrack, tr.trueTrackM.back(), 25.0);
    }
}
