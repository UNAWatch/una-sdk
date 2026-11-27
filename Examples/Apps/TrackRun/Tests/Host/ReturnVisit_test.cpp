// Oval::Tracker on a stored track: lining the track up with the
// day's GPS (offsets of 0, 5, 10 and 20 m in eight directions, starts round
// the lap), the running direction, and the visit check (blend, replace, a
// wrong pick, a run that never locks).
//
// The day's offset is made by storing the true track moved by minus the
// offset (Replay::shiftedModel): the same as today's GPS, primed fixes
// included, sitting that far from where the track was stored.

#include "OvalTestUtil.hpp"

#include "Oval/LaneTable.hpp"
#include "Oval/TrackStore.hpp"
#include "Oval/Tracker.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

using namespace OvalTest;
using Oval::StoreOutcome;
using Oval::Tracker;

namespace {

constexpr double kPi = 3.14159265358979;

// Laps one for one with the truth, each ending near the true pass of the
// start line and credited exactly its lane length; the total never went down.
::testing::AssertionResult lapsRight(const TraceGen::Trace& tr, const Replay::TrackerResult& r, uint32_t endTolMs)
{
    if (r.decreases != 0) {
        return ::testing::AssertionFailure() << "total went down " << r.decreases << " times";
    }
    if (r.laps.size() != tr.crossMs.size()) {
        return ::testing::AssertionFailure() << "counted " << r.laps.size() << " laps, truth " << tr.crossMs.size();
    }
    for (size_t k = 0; k < r.laps.size(); ++k) {
        const double endErr = std::fabs(static_cast<double>(Oval::msDiff(r.laps[k].tMs, tr.crossMs[k])));
        if (endErr > endTolMs) {
            return ::testing::AssertionFailure() << "lap " << k + 1 << " ends " << endErr << " ms from the truth";
        }
        if (std::fabs(r.laps[k].lapLengthM - tr.lapCreditM[k]) > 0.01) {
            return ::testing::AssertionFailure() << "lap " << k + 1 << " credited " << r.laps[k].lapLengthM
                                                 << " m, expected " << tr.lapCreditM[k];
        }
    }
    return ::testing::AssertionSuccess();
}

size_t modeChanges(const Replay::TrackerResult& r)
{
    size_t n = 0;
    for (const auto& e : r.events) {
        n += e.type == Tracker::EventType::ModeChanged ? 1 : 0;
    }
    return n;
}

double mean(const std::vector<double>& v)
{
    double s = 0.0;
    for (double x : v) {
        s += x;
    }
    return v.empty() ? 0.0 : s / v.size();
}

} // namespace

// ---------------------------------------------------------------------------
// Lining up with the day's GPS
// ---------------------------------------------------------------------------

TEST(ReturnVisit, DailyOffsetsAndStartsRoundTheLap)
{
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    const double offsets[] = {0.0, 5.0, 10.0, 20.0};
    const double starts[]  = {0.0, 100.0, 200.0, 300.0, 142.0}; // 142 m: the middle of the first bend
    std::vector<double> endM[4], endS[4], t0Err[4], during[4], after[4], lap1[4], lap1Ref[4], residual[4];
    int run = 0;
    for (int oi = 0; oi < 4; ++oi) {
        for (int b = 0; b < 8; ++b) {
            for (double startM : starts) {
                ++run;
                TraceGen::Track track;
                track.headingDeg  = static_cast<float>(37.0 * run);
                track.kerbRadiusM = static_cast<float>(35.0 + 0.1 * (run % 30));
                const uint8_t lane  = static_cast<uint8_t>(1 + run % 8);
                const float   sigma = 2.0f + 0.75f * static_cast<float>(run % 5); // 2 to 5 m
                const double  ang   = b * kPi / 4.0;
                const double  e     = offsets[oi] * PortableMath::cos(ang);
                const double  n     = offsets[oi] * PortableMath::sin(ang);
                const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(track), -e, -n);

                TraceGen::Generator g(track, metresHdop(noisy(sigma)), 1000u + run);
                g.begin(static_cast<float>(startM / g.laneLengthM(lane)), lane);
                g.runLaps(4, runner(3.5f + 0.1f * (run % 20)));
                g.run(40.0, runner());
                const TraceGen::Trace tr = g.finish();
                const Replay::TrackerResult r = Replay::runTracker(tr, table, Oval::Config(), &stored);

                const std::string where = "offset " + std::to_string(offsets[oi]) + " m bearing " +
                                          std::to_string(45 * b) + " start " + std::to_string(startM) + " sigma " +
                                          std::to_string(sigma);
                EXPECT_TRUE(lapsRight(tr, r, 5000)) << where;
                EXPECT_EQ(modeChanges(r), 0u) << "no off-track drop: " << where;
                EXPECT_TRUE(r.alignment.done) << where;
                EXPECT_FALSE(r.alignment.abandoned) << where;
                EXPECT_EQ(r.alignment.flips, 0) << where;

                endM[oi].push_back(r.alignment.endM);
                endS[oi].push_back(Oval::msDiff(r.alignment.endTMs, tr.startMs) / 1000.0);
                // How far the start line should have moved: the along-track
                // part of the day's offset at the start point.
                for (size_t i = tr.startIndex; i < tr.samples.size(); ++i) {
                    if (r.mode[i] != static_cast<uint8_t>(Tracker::Mode::Calibrated)) {
                        continue;
                    }
                    (i <= r.alignedAt ? during : after)[oi].push_back(std::fabs(r.total[i] - tr.truePhysM[i]));
                }
                if (!r.laps.empty() && r.laps[0].type == Tracker::EventType::Lap) {
                    lap1[oi].push_back(std::fabs(static_cast<double>(r.laps[0].lapDurationMs) - tr.crossActiveMs[0]) / 1000.0);
                    lap1Ref[oi].push_back(std::fabs(static_cast<double>(r.refinedDurMs[0]) - tr.crossActiveMs[0]) / 1000.0);
                }
                // The offset found against the true one (the day's offset plus
                // the GPS drift at the end of lining up).
                if (r.alignedAt != SIZE_MAX) {
                    const double de = r.alignment.offset.x - (e + tr.offsetE[r.alignedAt]);
                    const double dn = r.alignment.offset.y - (n + tr.offsetN[r.alignedAt]);
                    residual[oi].push_back(std::sqrt(de * de + dn * dn));
                }
                t0Err[oi].push_back(std::fabs(r.alignment.t0MoveM));
            }
        }
    }
    for (int oi = 0; oi < 4; ++oi) {
        char label[80];
        std::printf("[ visit    ] offset %.0f m: lined up after %.0f m (%.0f to %.0f), %.0f s on average\n", offsets[oi],
                    mean(endM[oi]), percentile(endM[oi], 0.0), percentile(endM[oi], 1.0), mean(endS[oi]));
        std::snprintf(label, sizeof label, "offset %.0f m: offset error at end (m)", offsets[oi]);
        printStats(label, residual[oi]);
        std::snprintf(label, sizeof label, "offset %.0f m: start line moved (m)", offsets[oi]);
        printStats(label, t0Err[oi]);
        std::snprintf(label, sizeof label, "offset %.0f m: part-lap aligning (m)", offsets[oi]);
        printStats(label, during[oi]);
        std::snprintf(label, sizeof label, "offset %.0f m: part-lap after (m)", offsets[oi]);
        printStats(label, after[oi]);
        std::snprintf(label, sizeof label, "offset %.0f m: lap 1 time (s)", offsets[oi]);
        printStats(label, lap1[oi]);
        std::snprintf(label, sizeof label, "offset %.0f m: lap 1 refined (s)", offsets[oi]);
        printStats(label, lap1Ref[oi]);
    }
}

// A large offset is found as soon as it can be: across the first bend.
// Settling by noise level, for the report.
TEST(ReturnVisit, AlignmentSettling)
{
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    for (float sigma : {2.0f, 3.5f, 5.0f}) {
        for (double offset : {0.0, 5.0, 10.0, 20.0}) {
            std::vector<double> settleM, err;
            for (int b = 0; b < 8; ++b) {
                TraceGen::Track track;
                track.headingDeg = static_cast<float>(23.0 * b + offset);
                const double ang = b * kPi / 4.0 + 0.3;
                const double e = offset * PortableMath::cos(ang), n = offset * PortableMath::sin(ang);
                const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(track), -e, -n);
                TraceGen::Generator g(track, noisy(sigma), 77u + b);
                g.begin(0.05f * b, 3);
                g.runLaps(2, runner(4.5f));
                const TraceGen::Trace tr = g.finish();
                // Settled: the offset in use within 1.5 m of the true one, and staying so.
                size_t settled = SIZE_MAX;
                const Replay::TrackerResult r = Replay::runTracker(tr, table, Oval::Config(), &stored,
                    [&](size_t i, Tracker& t) {
                        const double de = t.driftOffset().x - (e + tr.offsetE[i]);
                        const double dn = t.driftOffset().y - (n + tr.offsetN[i]);
                        const bool   ok = std::sqrt(de * de + dn * dn) <= 1.5;
                        if (ok && settled == SIZE_MAX) {
                            settled = i;
                        } else if (!ok && t.alignment().aligning) {
                            settled = SIZE_MAX;
                        }
                    });
                EXPECT_EQ(modeChanges(r), 0u);
                if (settled != SIZE_MAX) {
                    settleM.push_back(tr.truePhysM[settled]);
                }
                ASSERT_NE(r.alignedAt, SIZE_MAX) << "lining up finished";
                err.push_back(std::sqrt(std::pow(r.alignment.offset.x - (e + tr.offsetE[r.alignedAt]), 2.0)
                                        + std::pow(r.alignment.offset.y - (n + tr.offsetN[r.alignedAt]), 2.0)));
            }
            std::printf("[ settle   ] noise %.1f m, offset %4.1f m: within 1.5 m after %5.0f m (worst %5.0f m, %zu of 8); error at the end of lining up: mean %.2f m, worst %.2f m\n",
                        sigma, offset, mean(settleM), percentile(settleM, 1.0), settleM.size(), mean(err),
                        percentile(err, 1.0));
        }
    }
}

// ---------------------------------------------------------------------------
// Running direction
// ---------------------------------------------------------------------------

TEST(ReturnVisit, StoredTrackRunTheOtherWayRound)
{
    // The generator runs anticlockwise: storing the track as clockwise is
    // the same as the runner going round the other way on the day.
    const Oval::LaneTable table = Oval::LaneTable::outdoor();
    for (uint32_t seed : {3u, 5u, 8u, 13u}) {
        for (int runUp = 0; runUp < 2; ++runUp) {
            TraceGen::Track track;
            track.headingDeg = 30.0f * seed;
            const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(track), 3.0, -4.0).reversed();
            TraceGen::Generator g(track, noisy(3.0f), seed);
            g.begin(0.1f * seed, 2);
            if (runUp) {
                // Jog 35 m back behind the line first, then run the laps.
                g.shuttle(20.0, 35.0f, 3.5f, true);
            }
            g.runLaps(4, runner());
            g.run(40.0, runner());
            const TraceGen::Trace tr = g.finish();
            const Replay::TrackerResult r = Replay::runTracker(tr, table, Oval::Config(), &stored);
            EXPECT_TRUE(lapsRight(tr, r, 5000)) << "seed " << seed << " run-up " << runUp;
            EXPECT_GE(r.alignment.flips, 1) << "seed " << seed;
            EXPECT_TRUE(r.alignment.done) << "seed " << seed;
            EXPECT_EQ(modeChanges(r), 0u) << "seed " << seed;
            std::printf("[ reverse  ] seed %u%s: %d flip(s), %zu laps of %zu, final %.1f m against %.1f m\n", seed,
                        runUp ? " with a 35 m run-up" : "", r.alignment.flips, r.laps.size(), tr.crossMs.size(),
                        r.finalTotal, tr.truePhysM.back());
        }
    }
}

// ---------------------------------------------------------------------------
// The visit check
// ---------------------------------------------------------------------------

namespace {

// One visit: laps of the track as the GPS sees it that day, with a stored
// model or none; returns what the tracker made of it.
Replay::TrackerResult visit(const TraceGen::Track& seen, const Oval::TrackModel* stored, float sigma, uint32_t seed,
                            double laps = 6.0)
{
    TraceGen::Generator g(seen, noisy(sigma), seed);
    g.begin(0.1f + 0.07f * (seed % 10), static_cast<uint8_t>(1 + seed % 4));
    g.runLaps(laps, runner());
    g.run(40.0, runner());
    const TraceGen::Trace tr = g.finish();
    return Replay::runTracker(tr, Oval::LaneTable::outdoor(), Oval::Config(), stored);
}

// The day's offset is applied by moving the track the GPS sees: a visit on a
// day with offset (e, n) is a run on the true track moved by (e, n), and the
// store keeps what each visit fitted.
TraceGen::Track movedTrack(const TraceGen::Track& t, double e, double n)
{
    Oval::LocalFrame f;
    f.setOrigin(t.centreLat, t.centreLon);
    TraceGen::Track m = t;
    f.toLatLon(Oval::Vec2{static_cast<float>(e), static_cast<float>(n)}, m.centreLat, m.centreLon);
    return m;
}

} // namespace

TEST(ReturnVisit, SameTrackOnLaterVisitsBlendsInAndSettles)
{
    // Ten sequences of eight visits, each day with its own GPS offset (up to
    // 6 m, as GPS gives day to day). The first locks and saves; each later
    // one agrees and is blended in. The stored centre's error against the
    // true track shrinks, and so does its spread across the sequences.
    std::mt19937 rng(PortableRandom::seeded(99));
    PortableRandom::Normal day(0.0, 3.0);
    const int kVisits = 8;
    std::vector<double> centreErr[kVisits], headErr[kVisits], rErr[kVisits];
    int updated = 0, unchanged = 0, replaced = 0;
    for (int sq = 0; sq < 10; ++sq) {
        TraceGen::Track truth;
        truth.headingDeg  = 11.0f + 17.0f * sq;
        truth.kerbRadiusM = 35.5f + 0.25f * sq;
        Oval::TrackStore store;
        uint8_t number = 0;
        for (int v = 0; v < kVisits; ++v) {
            const double e = day(rng), n = day(rng);
            const TraceGen::Track seen = movedTrack(truth, e, n);
            const Oval::TrackRecord* rec = number ? store.find(number) : nullptr;
            const Oval::TrackModel stored = rec ? rec->model() : Oval::TrackModel();
            const Replay::TrackerResult r = visit(seen, rec ? &stored : nullptr, 2.5f + 0.25f * (v % 4), 300u + 31u * sq + v);
            const Oval::TrackStore::Applied a = store.apply(r.outcome, number, 1760000000u + 86400u * v);
            if (v == 0) {
                ASSERT_EQ(r.outcome.kind, StoreOutcome::Kind::New) << "sequence " << sq;
                number = a.number;
            } else {
                updated += r.outcome.kind == StoreOutcome::Kind::Updated ? 1 : 0;
                unchanged += r.outcome.kind == StoreOutcome::Kind::Unchanged ? 1 : 0;
                replaced += r.outcome.kind == StoreOutcome::Kind::Replaced ? 1 : 0;
                EXPECT_NE(r.outcome.kind, StoreOutcome::Kind::Replaced)
                    << "sequence " << sq << " visit " << v + 1 << ": heading " << r.visit.headingDeg << " R "
                    << r.visit.radiusM << " centre " << r.visit.centreM << " scale ratio " << r.visit.scaleRatio;
                EXPECT_NE(r.outcome.kind, StoreOutcome::Kind::New);
            }
            rec = store.find(number);
            ASSERT_NE(rec, nullptr);
            Oval::LocalFrame f;
            f.setOrigin(truth.centreLat, truth.centreLon);
            centreErr[v].push_back(Oval::length(f.toLocal(rec->lat, rec->lon)));
            double dh = std::fmod(std::fabs(rec->headingDeg - std::fmod(truth.headingDeg, 180.0f)), 180.0);
            headErr[v].push_back(dh > 90.0 ? 180.0 - dh : dh);
            rErr[v].push_back(std::fabs(rec->kerbRadiusM - truth.kerbRadiusM));
        }
    }
    std::printf("[ blend    ] later visits: %d updated, %d unchanged, %d replaced\n", updated, unchanged, replaced);
    for (int v = 0; v < kVisits; ++v) {
        std::printf("[ blend    ] after visit %d: centre error mean %.2f m (worst %.2f), heading %.2f deg, R %.2f m\n",
                    v + 1, mean(centreErr[v]), percentile(centreErr[v], 1.0), mean(headErr[v]), mean(rErr[v]));
    }
    EXPECT_GE(updated, 60) << "most later visits agree and are blended in";
    EXPECT_LT(mean(centreErr[kVisits - 1]), mean(centreErr[0])) << "the stored centre settles";
    EXPECT_LT(percentile(centreErr[kVisits - 1], 1.0), percentile(centreErr[0], 1.0));
    EXPECT_LT(mean(rErr[kVisits - 1]), mean(rErr[0]) + 0.05);
}

TEST(ReturnVisit, ChangedTrackReplacesTheStoredOne)
{
    // Resurfacing with a different bend radius, or a poor first calibration
    // (heading 6 degrees out): the visit's locked fit replaces the stored
    // one, keeping its number, and the rest of the run uses the new fit.
    for (int c = 0; c < 2; ++c) {
        for (uint32_t seed : {41u, 42u, 43u}) {
            TraceGen::Track truth;
            truth.headingDeg  = 20.0f + 10.0f * seed;
            truth.kerbRadiusM = c == 0 ? 38.0f : 36.5f;
            Oval::TrackModel stored = Replay::truthModel(truth);
            stored = c == 0 ? Oval::TrackModel::fromGeo(stored.centreLat(), stored.centreLon(), stored.headingDeg(),
                                                        35.0f, false, 50.0f)
                            : Oval::TrackModel::fromGeo(stored.centreLat(), stored.centreLon(),
                                                        stored.headingDeg() + 6.0f, 36.5f, false, 50.0f);
            const Replay::TrackerResult r = visit(truth, &stored, 3.0f, seed);
            EXPECT_EQ(r.outcome.kind, StoreOutcome::Kind::Replaced)
                << (c == 0 ? "R changed" : "heading out") << " seed " << seed << ": heading " << r.visit.headingDeg
                << " R " << r.visit.radiusM << " centre " << r.visit.centreM << " scale " << r.visit.scaleRatio;
            EXPECT_TRUE(r.outcome.model.hasGeo());
            EXPECT_NEAR(r.outcome.model.kerbRadiusM(), truth.kerbRadiusM, 1.2f);
            EXPECT_EQ(r.decreases, 0u);
            std::printf("[ replace  ] %s seed %u: heading %.2f deg, R %.2f m, centre %.2f m, scale ratio %.2f\n",
                        c == 0 ? "R 35 stored, 38 real" : "heading 6 deg out", seed, r.visit.headingDeg,
                        r.visit.radiusM, r.visit.centreM, r.visit.scaleRatio);

            // The store keeps the number.
            Oval::TrackStore st;
            const uint8_t n = st.saveNew(stored, Oval::FitQuality{3.0f, 0.5f}, 1u).number;
            const auto a = st.apply(r.outcome, n, 2u);
            EXPECT_EQ(a.kind, StoreOutcome::Kind::Replaced);
            EXPECT_EQ(a.number, n);
            EXPECT_EQ(st.count(), 1);
        }
    }
}

TEST(ReturnVisit, WrongPickOfANearbyTrack)
{
    // Two tracks whose centres are within 150 m: the stored one is picked,
    // but the runner is on the other. Lining up fails, the run calibrates
    // from scratch, and its locked fit is a new track. 40 m apart and turned
    // 50 degrees it is another track beside it (a warm-up track beside the
    // main one), not the picked one badly stored, and both are kept. A pick
    // that fits with the same heading still
    // replaces it.
    for (double apart : {120.0, 40.0}) {
        for (uint32_t seed : {7u, 8u, 9u}) {
            TraceGen::Track real;
            real.headingDeg = 15.0f * seed;
            TraceGen::Track other = movedTrack(real, apart * PortableMath::cos(0.7 * seed), apart * PortableMath::sin(0.7 * seed));
            other.headingDeg = real.headingDeg + 50.0f;
            const Oval::TrackModel stored = Replay::truthModel(other);
            const Replay::TrackerResult r = visit(real, &stored, 3.0f, seed);
            EXPECT_TRUE(r.alignment.abandoned) << apart << " m seed " << seed;
            EXPECT_EQ(r.finalStage, Tracker::CalStage::Locked) << apart << " m seed " << seed;
            EXPECT_EQ(r.outcome.kind, StoreOutcome::Kind::New) << apart << " m seed " << seed;
            EXPECT_EQ(r.decreases, 0u);
            std::printf("[ wrongpick] %.0f m apart, seed %u: abandoned after %.0f m (offset %.1f m, scale %.1f m), %zu laps\n",
                        apart, seed, r.alignment.endM, Oval::length(r.alignment.offset), r.alignment.scaleM,
                        r.laps.size());
        }
    }
}

TEST(ReturnVisit, RunThatNeverLocksChangesNothing)
{
    TraceGen::Track truth;
    truth.headingDeg = 70.0f;
    const Oval::TrackModel stored = Replay::shiftedModel(Replay::truthModel(truth), 4.0, 2.0);
    // Two laps: lined up and used, but the visit's calibration never locks.
    Replay::TrackerResult r = visit(truth, &stored, 3.0f, 5u, 2.0);
    EXPECT_TRUE(r.alignment.done);
    EXPECT_FALSE(r.visit.done);
    EXPECT_EQ(r.outcome.kind, StoreOutcome::Kind::Unchanged);
    EXPECT_FALSE(r.outcome.model.isValid());

    Oval::TrackStore st;
    const uint8_t n = st.saveNew(stored, Oval::FitQuality{3.0f, 0.5f}, 100u).number;
    const Oval::TrackRecord before = *st.find(n);
    const auto a = st.apply(r.outcome, n, 200u);
    EXPECT_EQ(a.kind, StoreOutcome::Kind::Unchanged);
    const Oval::TrackRecord& after = *st.find(n);
    EXPECT_EQ(after.lat, before.lat);
    EXPECT_EQ(after.lon, before.lon);
    EXPECT_EQ(after.headingDeg, before.headingDeg);
    EXPECT_EQ(after.kerbRadiusM, before.kerbRadiusM);
    EXPECT_EQ(after.fits, before.fits);
    EXPECT_EQ(after.uses, before.uses + 1);
    EXPECT_EQ(after.lastUsedS, 200u);

    // A wrong pick that never locks: nothing at all.
    TraceGen::Track other = movedTrack(truth, 120.0, 0.0);
    const Oval::TrackModel far = Replay::truthModel(other);
    r = visit(truth, &far, 3.0f, 6u, 2.0);
    EXPECT_TRUE(r.alignment.abandoned);
    EXPECT_EQ(r.outcome.kind, StoreOutcome::Kind::None);

    // No stored track and no lock: nothing either.
    r = visit(truth, nullptr, 3.0f, 7u, 2.0);
    EXPECT_EQ(r.outcome.kind, StoreOutcome::Kind::None);
}

TEST(ReturnVisit, PrimedPositionFindsTheStoredTrack)
{
    // Recognition uses the averaged pre-run position.
    TraceGen::Track truth;
    TraceGen::Generator g(truth, noisy(3.0f), 12);
    g.begin(0.3f, 1);
    g.runLaps(0.2, runner());
    const TraceGen::Trace tr = g.finish();
    Oval::Tracker t;
    double lat = 0.0, lon = 0.0;
    EXPECT_FALSE(t.primedPosition(lat, lon));
    for (size_t i = 0; i < tr.startIndex; ++i) {
        t.prime(tr.samples[i]);
    }
    ASSERT_TRUE(t.primedPosition(lat, lon));
    Oval::LocalFrame f;
    f.setOrigin(tr.startLat, tr.startLon);
    EXPECT_LT(Oval::length(f.toLocal(lat, lon)), 6.0f) << "the averaged primed fixes sit near the start point";

    Oval::TrackStore st;
    st.saveNew(Replay::truthModel(truth), Oval::FitQuality(), 1u);
    st.saveNew(Replay::truthModel(movedTrack(truth, 2000.0, 0.0)), Oval::FitQuality(), 2u);
    const Oval::TrackRecord* rec = st.findNear(lat, lon);
    ASSERT_NE(rec, nullptr);
    EXPECT_EQ(rec->number, 1);
}
