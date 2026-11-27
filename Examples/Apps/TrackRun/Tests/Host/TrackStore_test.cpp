// Oval::TrackStore: numbering, least recently used replacement,
// recognition, blending, and the text format (round trip, truncation at every
// byte, garbage, empty and later versions).

#include "PortableMath.hpp"
#include "PortableRandom.hpp"

#include "Oval/LocalFrame.hpp"
#include "Oval/TrackStore.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {

using Oval::FitQuality;
using Oval::StoreOutcome;
using Oval::TrackModel;
using Oval::TrackRecord;
using Oval::TrackStore;

constexpr double kLat = 55.95;
constexpr double kLon = -3.19;

/** A point east and north of (kLat, kLon), in metres. */
void offsetLatLon(double eastM, double northM, double& lat, double& lon)
{
    Oval::LocalFrame f;
    f.setOrigin(kLat, kLon);
    f.toLatLon(Oval::Vec2{static_cast<float>(eastM), static_cast<float>(northM)}, lat, lon);
}

TrackModel modelAt(double lat, double lon, float heading = 30.0f, float r = 36.5f, bool cw = false,
                   float alt = 50.0f)
{
    return TrackModel::fromGeo(lat, lon, heading, r, cw, alt);
}

TrackModel modelEast(double eastM, float heading = 30.0f)
{
    double lat = 0.0, lon = 0.0;
    offsetLatLon(eastM, 0.0, lat, lon);
    return modelAt(lat, lon, heading);
}

bool sameRecord(const TrackRecord& a, const TrackRecord& b)
{
    auto sameFloat = [](float x, float y) {
        return (std::isnan(x) && std::isnan(y)) || std::memcmp(&x, &y, sizeof x) == 0;
    };
    return std::memcmp(&a.lat, &b.lat, sizeof a.lat) == 0 && std::memcmp(&a.lon, &b.lon, sizeof a.lon) == 0
        && sameFloat(a.headingDeg, b.headingDeg) && sameFloat(a.kerbRadiusM, b.kerbRadiusM)
        && sameFloat(a.altM, b.altM) && sameFloat(a.scaleM, b.scaleM) && sameFloat(a.shapeM, b.shapeM)
        && a.firstSavedS == b.firstSavedS && a.lastUsedS == b.lastUsedS && a.fits == b.fits && a.uses == b.uses
        && a.number == b.number && a.clockwise == b.clockwise;
}

/** A full store of 10 tracks with varied values, 1 km apart. */
TrackStore fullStore()
{
    TrackStore    st;
    std::mt19937  rng(PortableRandom::seeded(42));
    PortableRandom::Uniform u(0.0, 1.0);
    for (int i = 0; i < TrackStore::kMaxTracks; ++i) {
        double lat = 0.0, lon = 0.0;
        offsetLatLon(1000.0 * i, -500.0 * i, lat, lon);
        lat += u(rng) * 1e-6; // not round numbers
        const float alt = (i == 3) ? std::numeric_limits<float>::quiet_NaN() : static_cast<float>(-20.0 + 900.0 * u(rng));
        const float heading = static_cast<float>(179.99 * u(rng)); // one draw a statement
        const float radius  = static_cast<float>(35.0 + 3.0 * u(rng));
        const TrackModel m = modelAt(lat, lon, heading, radius, (i % 3) == 0, alt);
        const FitQuality q{static_cast<float>(5.0 * u(rng)), static_cast<float>(u(rng))};
        const auto r = st.saveNew(m, q, 1760000000u + 1000u * i);
        EXPECT_EQ(r.number, i + 1);
        for (int k = 0; k < i; ++k) {
            st.touch(static_cast<uint8_t>(i + 1), 1760000000u + 1000u * i + k);
        }
    }
    return st;
}

std::string serialise(const TrackStore& st)
{
    std::vector<char> buf(TrackStore::kMaxSerialisedBytes);
    const size_t n = st.serialise(buf.data(), buf.size());
    EXPECT_GT(n, 0u);
    return std::string(buf.data(), n);
}

} // namespace

TEST(TrackStore, AddUpdateTouchForgetAndList)
{
    TrackStore st;
    EXPECT_EQ(st.count(), 0);
    const auto a = st.saveNew(modelEast(0.0), FitQuality{3.0f, 0.4f}, 1000u);
    const auto b = st.saveNew(modelEast(2000.0), FitQuality{2.0f, 0.3f}, 2000u);
    const auto c = st.saveNew(modelEast(4000.0), FitQuality{2.5f, 0.2f}, 3000u);
    EXPECT_EQ(a.number, 1);
    EXPECT_EQ(b.number, 2);
    EXPECT_EQ(c.number, 3);
    EXPECT_EQ(a.replaced, 0);
    EXPECT_EQ(st.count(), 3);

    const TrackRecord* r2 = st.find(2);
    ASSERT_NE(r2, nullptr);
    EXPECT_EQ(r2->uses, 1);
    EXPECT_EQ(r2->fits, 1);
    EXPECT_EQ(r2->firstSavedS, 2000u);
    EXPECT_EQ(r2->lastUsedS, 2000u);
    EXPECT_NEAR(r2->scaleM, 2.0f, 1e-6f);

    EXPECT_TRUE(st.touch(2, 5000u));
    EXPECT_EQ(st.find(2)->uses, 2);
    EXPECT_EQ(st.find(2)->lastUsedS, 5000u);
    EXPECT_FALSE(st.touch(7, 5000u));

    // update replaces the model and keeps the number and first date.
    EXPECT_TRUE(st.update(2, modelEast(2000.0, 75.0f), FitQuality{1.0f, 0.1f}, 6000u));
    EXPECT_NEAR(st.find(2)->headingDeg, 75.0f, 1e-4f);
    EXPECT_EQ(st.find(2)->number, 2);
    EXPECT_EQ(st.find(2)->firstSavedS, 2000u);
    EXPECT_EQ(st.find(2)->lastUsedS, 6000u);
    EXPECT_EQ(st.find(2)->uses, 3);

    const TrackRecord* out[TrackStore::kMaxTracks];
    ASSERT_EQ(st.list(out), 3);
    EXPECT_EQ(out[0]->number, 1);
    EXPECT_EQ(out[1]->number, 2);
    EXPECT_EQ(out[2]->number, 3);

    EXPECT_TRUE(st.forget(2));
    EXPECT_FALSE(st.forget(2));
    EXPECT_EQ(st.find(2), nullptr);
    ASSERT_EQ(st.list(out), 2);
    EXPECT_EQ(out[0]->number, 1);
    EXPECT_EQ(out[1]->number, 3);

    // A model with no latitude and longitude cannot be stored.
    TrackModel local;
    local.set(Oval::Vec2(), 0.0f, 36.5f, false);
    EXPECT_EQ(st.saveNew(local, FitQuality(), 7000u).number, 0);
}

TEST(TrackStore, NumbersAreStableAndANewTrackTakesTheLowestFree)
{
    TrackStore st;
    for (int i = 0; i < 5; ++i) {
        st.saveNew(modelEast(1000.0 * i), FitQuality(), 100u + i);
    }
    EXPECT_TRUE(st.forget(3));
    EXPECT_TRUE(st.forget(1));
    // Forgetting does not renumber: Track 5 is still Track 5.
    ASSERT_NE(st.find(5), nullptr);
    EXPECT_EQ(st.saveNew(modelEast(9000.0), FitQuality(), 200u).number, 1);
    EXPECT_EQ(st.saveNew(modelEast(9500.0), FitQuality(), 201u).number, 3);
    EXPECT_EQ(st.saveNew(modelEast(9900.0), FitQuality(), 202u).number, 6);
}

TEST(TrackStore, EleventhTrackReplacesTheLeastRecentlyUsed)
{
    TrackStore st;
    for (int i = 0; i < 10; ++i) {
        st.saveNew(modelEast(1000.0 * i), FitQuality(), 1000u + i);
    }
    // Track 1 is the oldest, but used recently; Track 4 is now the least
    // recently used.
    st.touch(1, 5000u);
    st.touch(2, 5001u);
    st.touch(3, 5002u);
    const auto r = st.saveNew(modelEast(20000.0), FitQuality(), 6000u);
    EXPECT_EQ(r.replaced, 4);
    EXPECT_EQ(r.number, 4);
    EXPECT_EQ(st.count(), 10);
    EXPECT_EQ(st.find(4)->firstSavedS, 6000u);
    EXPECT_EQ(st.find(4)->uses, 1);
    // The old Track 4 is no longer recognised; the new one is.
    EXPECT_EQ(st.findNear(modelEast(3000.0).centreLat(), modelEast(3000.0).centreLon()), nullptr);
    const TrackRecord* n = st.findNear(modelEast(20000.0).centreLat(), modelEast(20000.0).centreLon());
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->number, 4);

    // Next: Track 5 (equal last use goes to the earliest saved).
    EXPECT_EQ(st.saveNew(modelEast(30000.0), FitQuality(), 6001u).replaced, 5);
}

TEST(TrackStore, ARunsNewTrackDoesNotTakeTheTrackItUsed)
{
    // A full store. The run found a new track, then used Track 1, the least
    // recently used: Track 1 is touched first, and the new track takes the
    // next least recently used place.
    TrackStore st;
    for (int i = 0; i < 10; ++i) {
        st.saveNew(modelEast(1000.0 * i), FitQuality(), 1000u + i);
    }
    StoreOutcome o[2];
    o[0].kind  = StoreOutcome::Kind::New;
    o[0].model = modelEast(20000.0);
    o[1].kind  = StoreOutcome::Kind::Unchanged;
    const uint8_t numbers[2] = {0, 1};
    TrackStore::Applied a[2];
    st.applyRun(o, numbers, 2, 6000u, a);
    EXPECT_EQ(a[1].kind, StoreOutcome::Kind::Unchanged);
    EXPECT_EQ(a[1].number, 1);
    EXPECT_EQ(a[0].kind, StoreOutcome::Kind::New);
    EXPECT_EQ(a[0].replaced, 2);
    EXPECT_EQ(a[0].number, 2);
    ASSERT_NE(st.find(1), nullptr);
    EXPECT_EQ(st.find(1)->lastUsedS, 6000u);
    const TrackRecord* one = st.findNear(modelEast(0.0).centreLat(), modelEast(0.0).centreLon());
    ASSERT_NE(one, nullptr);
    EXPECT_EQ(one->number, 1);
}

TEST(TrackStore, ARunsNewTrackDoesNotTakeTheTrackItUsedWithTheClockNotSet)
{
    // As above, with the watch clock not set: every date is 0, Track 1 is
    // still the first in line, and the new track takes Track 2's place.
    TrackStore st;
    for (int i = 0; i < 10; ++i) {
        st.saveNew(modelEast(1000.0 * i), FitQuality(), 0u);
    }
    StoreOutcome o[2];
    o[0].kind  = StoreOutcome::Kind::New;
    o[0].model = modelEast(20000.0);
    o[1].kind  = StoreOutcome::Kind::Unchanged;
    const uint8_t numbers[2] = {0, 1};
    TrackStore::Applied a[2];
    st.applyRun(o, numbers, 2, 0u, a);
    EXPECT_EQ(a[1].kind, StoreOutcome::Kind::Unchanged);
    EXPECT_EQ(a[0].kind, StoreOutcome::Kind::New);
    EXPECT_EQ(a[0].replaced, 2);
    const TrackRecord* one = st.findNear(modelEast(0.0).centreLat(), modelEast(0.0).centreLon());
    ASSERT_NE(one, nullptr);
    EXPECT_EQ(one->number, 1) << "the track the run used is still there";
}

TEST(TrackStore, FindNearAtTheEdgeAndWithTwoTracksClose)
{
    TrackStore st;
    st.saveNew(modelEast(0.0), FitQuality(), 1u);
    for (double d : {0.0, 100.0, 149.0}) {
        for (int b = 0; b < 8; ++b) {
            const double a = b * 0.7853981633974483;
            double lat = 0.0, lon = 0.0;
            offsetLatLon(d * PortableMath::cos(a), d * PortableMath::sin(a), lat, lon);
            float dist = -1.0f;
            const TrackRecord* r = st.findNear(lat, lon, &dist);
            ASSERT_NE(r, nullptr) << d << " m at bearing " << b;
            EXPECT_EQ(r->number, 1);
            EXPECT_NEAR(dist, d, 0.05 + d * 1e-4);
        }
    }
    for (int b = 0; b < 8; ++b) {
        const double a = b * 0.7853981633974483;
        double lat = 0.0, lon = 0.0;
        offsetLatLon(151.0 * PortableMath::cos(a), 151.0 * PortableMath::sin(a), lat, lon);
        EXPECT_EQ(st.findNear(lat, lon), nullptr) << "151 m at bearing " << b;
    }

    // Two tracks 120 m apart: the nearer centre wins either side.
    st.saveNew(modelEast(120.0), FitQuality(), 2u);
    double lat = 0.0, lon = 0.0;
    offsetLatLon(55.0, 30.0, lat, lon);
    EXPECT_EQ(st.findNear(lat, lon)->number, 1);
    offsetLatLon(65.0, -30.0, lat, lon);
    EXPECT_EQ(st.findNear(lat, lon)->number, 2);
    offsetLatLon(200.0, 0.0, lat, lon);
    EXPECT_EQ(st.findNear(lat, lon)->number, 2);
}

TEST(TrackStore, BlendIsWeightedByVisits)
{
    Oval::Config cfg;
    cfg.storeBlendMaxWeight = 3;
    TrackStore st(cfg);
    st.saveNew(modelAt(kLat, kLon, 10.0f, 36.0f), FitQuality{2.0f, 0.2f}, 1u);

    // Second visit 4 m east, heading 12, R 37: half and half.
    double lat = 0.0, lon = 0.0;
    offsetLatLon(4.0, 0.0, lat, lon);
    ASSERT_TRUE(st.blend(1, modelAt(lat, lon, 12.0f, 37.0f, true), FitQuality{4.0f, 0.4f}, 2u));
    const TrackRecord* r = st.find(1);
    double e = 0.0, n = 0.0;
    {
        Oval::LocalFrame f;
        f.setOrigin(kLat, kLon);
        const Oval::Vec2 p = f.toLocal(r->lat, r->lon);
        e = p.x;
        n = p.y;
    }
    EXPECT_NEAR(e, 2.0, 0.01);
    EXPECT_NEAR(n, 0.0, 0.01);
    EXPECT_NEAR(r->headingDeg, 11.0f, 1e-3f);
    EXPECT_NEAR(r->kerbRadiusM, 36.5f, 1e-3f);
    EXPECT_NEAR(r->scaleM, 3.0f, 1e-3f);
    EXPECT_TRUE(r->clockwise); // the direction last run
    EXPECT_EQ(r->fits, 2);
    EXPECT_EQ(r->uses, 2);
    EXPECT_EQ(r->lastUsedS, 2u);

    // Headings wrap at 180: 179 and 1 blend to 0 (or 180), not 90.
    st.saveNew(modelAt(10.0, 10.0, 179.0f), FitQuality(), 3u);
    ASSERT_TRUE(st.blend(2, modelAt(10.0, 10.0, 1.0f), FitQuality(), 4u));
    const float h = st.find(2)->headingDeg;
    EXPECT_TRUE(h < 0.01f || h > 179.99f) << h;

    // The weight stops growing at storeBlendMaxWeight: a visit always counts
    // at least 1 / (3 + 1).
    for (int i = 0; i < 6; ++i) {
        st.blend(1, modelAt(r->lat, r->lon, r->headingDeg, 36.5f), FitQuality(), 5u);
    }
    EXPECT_EQ(st.find(1)->fits, 8);
    st.blend(1, modelAt(r->lat, r->lon, r->headingDeg, 38.5f), FitQuality(), 6u);
    EXPECT_NEAR(st.find(1)->kerbRadiusM, 36.5f + 2.0f / 4.0f, 1e-3f);
}

TEST(TrackStore, ApplyOutcomes)
{
    TrackStore   st;
    StoreOutcome o;
    o.model   = modelEast(0.0);
    o.quality = FitQuality{2.0f, 0.2f};

    o.kind = StoreOutcome::Kind::None;
    EXPECT_FALSE(st.apply(o, 0, 1u).changed);
    EXPECT_EQ(st.count(), 0);

    o.kind = StoreOutcome::Kind::New;
    auto a = st.apply(o, 0, 1u);
    EXPECT_TRUE(a.changed);
    EXPECT_EQ(a.number, 1);

    o.kind = StoreOutcome::Kind::Unchanged;
    a = st.apply(o, 1, 2u);
    EXPECT_TRUE(a.changed);
    EXPECT_EQ(st.find(1)->uses, 2);
    EXPECT_EQ(st.find(1)->fits, 1);

    o.kind = StoreOutcome::Kind::Updated;
    a = st.apply(o, 1, 3u);
    EXPECT_EQ(a.kind, StoreOutcome::Kind::Updated);
    EXPECT_EQ(st.find(1)->fits, 2);

    o.kind  = StoreOutcome::Kind::Replaced;
    o.model = modelEast(0.0, 80.0f);
    a = st.apply(o, 1, 4u);
    EXPECT_EQ(a.kind, StoreOutcome::Kind::Replaced);
    EXPECT_EQ(st.find(1)->fits, 1);
    EXPECT_NEAR(st.find(1)->headingDeg, 80.0f, 1e-3f);
    EXPECT_EQ(st.count(), 1);

    // The supplied track was forgotten meanwhile: a locked fit is kept as new.
    st.forget(1);
    o.kind = StoreOutcome::Kind::Updated;
    a = st.apply(o, 1, 5u);
    EXPECT_EQ(a.kind, StoreOutcome::Kind::New);
    EXPECT_EQ(st.count(), 1);
    o.kind = StoreOutcome::Kind::Unchanged;
    EXPECT_FALSE(st.apply(o, 7, 6u).changed);
}

TEST(TrackStore, FullStoreRoundTripsExactly)
{
    const TrackStore  st   = fullStore();
    const std::string text = serialise(st);
    std::printf("[ store    ] a full store is %zu bytes; first lines:\n%.*s", text.size(),
                static_cast<int>(text.find('\n', text.find('\n') + 1) + 1), text.c_str());
    EXPECT_LE(text.size(), TrackStore::kMaxSerialisedBytes);

    TrackStore back;
    const TrackStore::LoadReport rep = back.load(text.data(), text.size());
    EXPECT_TRUE(rep.headerOk);
    EXPECT_EQ(rep.version, 1);
    EXPECT_FALSE(rep.newerVersion);
    EXPECT_EQ(rep.expected, 10);
    EXPECT_EQ(rep.loaded, 10);
    EXPECT_EQ(rep.dropped(), 0);
    for (uint8_t n = 1; n <= TrackStore::kMaxTracks; ++n) {
        ASSERT_NE(back.find(n), nullptr);
        EXPECT_TRUE(sameRecord(*st.find(n), *back.find(n))) << "track " << int(n);
    }
    // And again: the text itself is stable.
    EXPECT_EQ(serialise(back), text);

    // Latitude and longitude keep 9 decimal places: under a millimetre.
    const TrackModel m = back.find(5)->model();
    EXPECT_NEAR(m.centreLat(), st.find(5)->lat, 1e-12);

    // Too small a buffer writes nothing.
    std::vector<char> small(text.size() - 1);
    EXPECT_EQ(st.serialise(small.data(), small.size()), 0u);
}

TEST(TrackStore, ValuesRoundToTheStoredDecimalsOnEntry)
{
    TrackStore st;
    st.saveNew(modelAt(55.123456789123, -3.987654321987, 12.345678f, 36.123456f), FitQuality{3.14159f, 0.271828f}, 1u);
    const TrackRecord* r = st.find(1);
    EXPECT_NEAR(r->lat, 55.123456789, 1e-12);
    EXPECT_NEAR(r->lon, -3.987654322, 1e-12);
    EXPECT_NEAR(r->headingDeg, 12.3457f, 1e-5f);
    EXPECT_NEAR(r->kerbRadiusM, 36.1235f, 1e-5f);
    EXPECT_NEAR(r->scaleM, 3.142f, 1e-6f);
}

TEST(TrackStore, TruncationAtEveryByteNeverCrashesAndLoadsWhatItCan)
{
    const TrackStore  st   = fullStore();
    const std::string text = serialise(st);
    // Where each line ends.
    std::vector<size_t> ends;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') {
            ends.push_back(i + 1);
        }
    }
    ASSERT_EQ(ends.size(), 11u);
    for (size_t cut = 0; cut <= text.size(); ++cut) {
        TrackStore back;
        const TrackStore::LoadReport rep = back.load(text.data(), cut);
        // Every whole track line before the cut loads; the cut one is dropped.
        size_t whole = 0;
        while (whole < ends.size() && ends[whole] <= cut) {
            ++whole;
        }
        const size_t tracks = whole > 0 ? whole - 1 : 0;
        ASSERT_EQ(rep.loaded, tracks) << "cut at byte " << cut;
        const bool partLine = cut > (whole > 0 ? ends[whole - 1] : 0);
        if (partLine && whole > 0) {
            EXPECT_EQ(rep.truncated, 1) << "cut at byte " << cut;
        }
        for (uint8_t n = 1; n <= tracks; ++n) {
            ASSERT_NE(back.find(n), nullptr);
            EXPECT_TRUE(sameRecord(*st.find(n), *back.find(n)));
        }
        EXPECT_EQ(rep.headerOk, cut >= ends[0]);
    }
}

TEST(TrackStore, GarbledEmptyAndGarbageFiles)
{
    const TrackStore st   = fullStore();
    std::string      text = serialise(st);

    // Empty.
    TrackStore back;
    TrackStore::LoadReport rep = back.load("", 0);
    EXPECT_EQ(rep.loaded, 0);
    EXPECT_FALSE(rep.headerOk);
    rep = back.load(nullptr, 100);
    EXPECT_EQ(back.count(), 0);

    // Random bytes, including NULs and newlines.
    std::mt19937 rng(PortableRandom::seeded(7));
    for (int trial = 0; trial < 200; ++trial) {
        std::string g(static_cast<size_t>(rng() % 3000), '\0');
        for (char& c : g) {
            c = static_cast<char>(rng() % 256);
        }
        rep = back.load(g.data(), g.size());
        EXPECT_EQ(rep.loaded, 0);
    }

    // One flipped character in each track line drops that line only.
    for (int line = 1; line <= 10; ++line) {
        std::string bad = text;
        size_t      pos = 0;
        for (int k = 0; k < line; ++k) {
            pos = bad.find('\n', pos) + 1;
        }
        const size_t at = pos + 20 + static_cast<size_t>(line); // inside the latitude or longitude
        bad[at] = bad[at] == '5' ? '6' : '5';
        rep = back.load(bad.data(), bad.size());
        EXPECT_EQ(rep.loaded, 9) << "line " << line;
        EXPECT_EQ(rep.badChecksum, 1);
        EXPECT_EQ(back.find(static_cast<uint8_t>(line)), nullptr);
    }

    // Windows line endings still load.
    std::string crlf;
    for (char c : text) {
        if (c == '\n') {
            crlf += '\r';
        }
        crlf += c;
    }
    rep = back.load(crlf.data(), crlf.size());
    EXPECT_EQ(rep.loaded, 10);

    // A duplicated line is loaded once.
    const size_t l1 = text.find('\n') + 1;
    const size_t l2 = text.find('\n', l1) + 1;
    std::string dup = text + text.substr(l1, l2 - l1);
    rep = back.load(dup.data(), dup.size());
    EXPECT_EQ(rep.loaded, 10);
    EXPECT_EQ(rep.duplicate, 1);

    // No header: the tracks still load, and the report says so.
    std::string noHeader = text.substr(l1);
    rep = back.load(noHeader.data(), noHeader.size());
    EXPECT_FALSE(rep.headerOk);
    EXPECT_EQ(rep.loaded, 10);
}

TEST(TrackStore, LaterVersionLoadsWhatItCan)
{
    // A version 2 file: extra keys, an unknown kind of line, more decimals.
    // A line whose CRC matches is loaded with the keys this version knows.
    auto line = [](const std::string& body) {
        // CRC-32 as the store computes it.
        uint32_t c = 0xFFFFFFFFu;
        for (char ch : body) {
            c ^= static_cast<uint8_t>(ch);
            for (int k = 0; k < 8; ++k) {
                c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
            }
        }
        char hex[9];
        std::snprintf(hex, sizeof hex, "%08x", ~c);
        return body + " crc=" + hex + "\n";
    };
    std::string text = "trackrun-tracks v=2 count=3 name=club\n";
    text += line("track v=2 n=4 lat=55.9500123456 lon=-3.19 hdg=12.5 r=36.6 cw=0 alt=40.5 lanes=8 surface=poly");
    text += "photo id=17 data=ABCDEF\n";
    text += line("track v=2 n=7 lat=51.5 lon=-0.12 hdg=90.0 r=37.0 cw=1 firstlap=12");
    text += line("track v=2 n=8 lat=51.5 lon=-0.12 hdg=90.0 cw=1"); // no r: dropped
    TrackStore back;
    const TrackStore::LoadReport rep = back.load(text.data(), text.size());
    EXPECT_TRUE(rep.headerOk);
    EXPECT_EQ(rep.version, 2);
    EXPECT_TRUE(rep.newerVersion);
    EXPECT_EQ(rep.loaded, 2);
    EXPECT_EQ(rep.badField, 1);
    EXPECT_EQ(rep.otherLines, 1);
    ASSERT_NE(back.find(4), nullptr);
    EXPECT_NEAR(back.find(4)->lat, 55.950012346, 1e-12); // rounded to 9 decimals
    EXPECT_EQ(back.find(4)->version, 2);
    EXPECT_EQ(back.find(4)->fits, 1);
    ASSERT_NE(back.find(7), nullptr);
    EXPECT_TRUE(back.find(7)->clockwise);
    EXPECT_TRUE(std::isnan(back.find(7)->altM));
}
