// The service's own files with the SDK's in-memory kernel (its host test
// support): Track Run's settings and summary fields, the stored tracks file
// saved atomically, and the GPS trace file.

#include "ActivitySummarySerializer.hpp"
#include "GpsTraceWriter.hpp"
#include "KernelTestDoubles.hpp"
#include "Settings.hpp"
#include "SettingsSerializer.hpp"
#include "TrackStoreFile.hpp"

#include "Oval/TrackModel.hpp"
#include "Oval/TrackStore.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <string>
#include <vector>

using SDK::TestSupport::KernelFixture;

// ---------------------------------------------------------------------------
// Settings and the summary
// ---------------------------------------------------------------------------

TEST(ServiceSettings, TheLaneAndItsShortcutRoundTrip)
{
    KernelFixture fx;
    SettingsSerializer ser(fx.kernel, "settings.json");
    Settings a {};
    EXPECT_EQ(a.lastLane, 1u) << "lane 1 by default";
    EXPECT_TRUE(a.laneHoldEn) << "the shortcut on by default";
    EXPECT_EQ(a.alertDistanceId, Settings::Alerts::Distance::ID_M_400) << "a 400 m auto-lap by default";
    a.lastLane   = 0;   // "No lane"
    a.laneHoldEn = false;
    a.alertDistanceId = Settings::Alerts::Distance::ID_M_1600;
    ASSERT_TRUE(ser.save(a));
    Settings b {};
    ASSERT_TRUE(ser.load(b));
    EXPECT_EQ(b.lastLane, 0u);
    EXPECT_FALSE(b.laneHoldEn);
    EXPECT_EQ(b.alertDistanceId, Settings::Alerts::Distance::ID_M_1600);
    EXPECT_FLOAT_EQ(Settings::Alerts::Distance::toMeters(b.alertDistanceId, true), 1600.0f) << "metres on any watch";
}

TEST(ServiceSettings, ALaneOutOfRangeLoadsAsLaneOne)
{
    KernelFixture fx;
    fx.fileSystem.seedFile("settings.json", "{\"version\":1,\"last_lane\":9}");
    SettingsSerializer ser(fx.kernel, "settings.json");
    Settings s {};
    s.lastLane = 5;
    ASSERT_TRUE(ser.load(s));
    EXPECT_EQ(s.lastLane, 1u);
}

TEST(ServiceSummary, TrackRunFieldsRoundTripAndAnOldFileLoads)
{
    KernelFixture fx;
    ActivitySummarySerializer ser(fx.kernel, "summary.json");
    ActivitySummary a {};
    a.time     = 600;
    a.distance = 2864.5f;
    a.laps.push_back({ 86, 400.0f, 0.21f });
    TrackRunSummary& tr = a.trackRun;
    tr.savedNumber = 3;
    tr.onTrack     = true;
    tr.calLocked   = true;
    tr.laneMin     = 1;
    tr.laneMax     = 3;
    tr.laneLast    = 2;
    tr.roadM       = 1200.5f;
    tr.trackM      = 1664.0f;
    ASSERT_TRUE(ser.save(a));

    ActivitySummary b {};
    ASSERT_TRUE(ser.load(b));
    ASSERT_EQ(b.laps.size(), 1u);
    EXPECT_EQ(b.trackRun.trackNumber, 0u);
    EXPECT_EQ(b.trackRun.savedNumber, 3u);
    EXPECT_TRUE(b.trackRun.onTrack);
    EXPECT_FALSE(b.trackRun.notStandard);
    EXPECT_TRUE(b.trackRun.calLocked);
    EXPECT_EQ(b.trackRun.laneMin, 1u);
    EXPECT_EQ(b.trackRun.laneMax, 3u);
    EXPECT_EQ(b.trackRun.laneLast, 2u);
    EXPECT_FLOAT_EQ(b.trackRun.roadM, 1200.5f);
    EXPECT_FLOAT_EQ(b.trackRun.trackM, 1664.0f);

    // A summary with no Track Run keys: the defaults, not what was there.
    fx.fileSystem.seedFile("summary.json",
        "{\"utc\":1,\"time\":60,\"distance\":400.0,\"lap_count\":1,\"laps\":[{\"dur\":60,\"dist\":400.0,\"pace\":0.15}]}");
    ActivitySummary c {};
    c.trackRun.laneMax = 9;
    ASSERT_TRUE(ser.load(c));
    ASSERT_EQ(c.laps.size(), 1u);
    EXPECT_EQ(c.trackRun.laneMax, 0u);
    EXPECT_FALSE(c.trackRun.onTrack);
}

// ---------------------------------------------------------------------------
// The stored tracks file
// ---------------------------------------------------------------------------

namespace {

Oval::TrackStore storeWith(int n)
{
    Oval::TrackStore st;
    for (int i = 0; i < n; ++i) {
        const Oval::TrackModel m = Oval::TrackModel::fromGeo(55.95 + 0.01 * i, -3.19, 20.0f * i, 36.5f, false, 50.0f);
        Oval::FitQuality q;
        q.scaleM = 2.0f;
        st.saveNew(m, q, 1791936000u + i);
    }
    return st;
}

} // namespace

TEST(ServiceTracksFile, SavedAtomicallyAndLoadedBack)
{
    KernelFixture fx;
    TrackStoreFile file(fx.kernel);
    Oval::TrackStore empty;
    const char* from = "x";
    const Oval::TrackStore::LoadReport r0 = file.load(empty, &from);
    EXPECT_EQ(from, nullptr) << "no file yet";
    EXPECT_EQ(r0.loaded, 0u);

    const Oval::TrackStore one = storeWith(1);
    EXPECT_EQ(file.save(one), TrackStoreFile::SaveResult::Saved);
    EXPECT_TRUE(fx.fileSystem.exist(TrackStoreFile::kPath));
    EXPECT_FALSE(fx.fileSystem.exist(TrackStoreFile::kTmpPath));
    EXPECT_FALSE(fx.fileSystem.exist(TrackStoreFile::kBakPath));

    const Oval::TrackStore three = storeWith(3);
    EXPECT_EQ(file.save(three), TrackStoreFile::SaveResult::Saved) << "over an existing file";
    EXPECT_FALSE(fx.fileSystem.exist(TrackStoreFile::kTmpPath));
    EXPECT_FALSE(fx.fileSystem.exist(TrackStoreFile::kBakPath));
    Oval::TrackStore back;
    const Oval::TrackStore::LoadReport r = file.load(back, &from);
    EXPECT_STREQ(from, TrackStoreFile::kPath);
    EXPECT_EQ(r.loaded, 3u);
    EXPECT_EQ(back.count(), 3u);

    // Interrupted after moving the old file aside: the .bak is loaded.
    const std::string text = fx.fileSystem.readFile(TrackStoreFile::kPath);
    fx.fileSystem.remove(TrackStoreFile::kPath);
    fx.fileSystem.seedFile(TrackStoreFile::kBakPath, text);
    Oval::TrackStore rec;
    TrackStoreFile file2(fx.kernel);
    EXPECT_EQ(file2.load(rec, &from).loaded, 3u);
    EXPECT_STREQ(from, TrackStoreFile::kBakPath);
    EXPECT_TRUE(fx.fileSystem.fileExistsAt(TrackStoreFile::kPath)) << "put back in place";
    EXPECT_FALSE(fx.fileSystem.fileExistsAt(TrackStoreFile::kBakPath));

    // Interrupted before the new file took its place: the .tmp (written in
    // full first) is newer than the .bak, and is put in place at once, so
    // the next save (which rewrites the .tmp) cannot lose it.
    const Oval::TrackStore four = storeWith(4);
    char buf[Oval::TrackStore::kMaxSerialisedBytes];
    const size_t n4 = four.serialise(buf, sizeof buf);
    fx.fileSystem.remove(TrackStoreFile::kPath);
    fx.fileSystem.seedFile(TrackStoreFile::kBakPath, text);
    fx.fileSystem.seedFile(TrackStoreFile::kTmpPath, std::string(buf, n4));
    Oval::TrackStore rec2;
    TrackStoreFile file3(fx.kernel);
    EXPECT_EQ(file3.load(rec2, &from).loaded, 4u);
    EXPECT_STREQ(from, TrackStoreFile::kTmpPath);
    EXPECT_TRUE(fx.fileSystem.fileExistsAt(TrackStoreFile::kPath));
    EXPECT_FALSE(fx.fileSystem.fileExistsAt(TrackStoreFile::kTmpPath));
    Oval::TrackStore again;
    TrackStoreFile file3b(fx.kernel);
    EXPECT_EQ(file3b.load(again, &from).loaded, 4u);
    EXPECT_STREQ(from, TrackStoreFile::kPath);

    // A damaged tracks.txt gives way to a whole file.
    fx.fileSystem.seedFile(TrackStoreFile::kPath, "garbage\n");
    Oval::TrackStore rec3;
    TrackStoreFile file4(fx.kernel);
    EXPECT_EQ(file4.load(rec3, &from).loaded, 3u);
    EXPECT_STREQ(from, TrackStoreFile::kBakPath);
    EXPECT_FALSE(fx.fileSystem.fileExistsAt(TrackStoreFile::kBakPath)) << "the damaged file gave way to it";
    Oval::TrackStore rec3b;
    TrackStoreFile file4b(fx.kernel);
    EXPECT_EQ(file4b.load(rec3b, &from).loaded, 3u);
    EXPECT_STREQ(from, TrackStoreFile::kPath);

    // A whole tracks.txt with no track (the last one forgotten) is taken as
    // it is, never passed over for an older backup.
    const Oval::TrackStore none;
    const size_t n0 = none.serialise(buf, sizeof buf);
    ASSERT_GT(n0, 0u);
    fx.fileSystem.seedFile(TrackStoreFile::kPath, std::string(buf, n0));
    fx.fileSystem.seedFile(TrackStoreFile::kBakPath, text);
    Oval::TrackStore rec4;
    TrackStoreFile file5(fx.kernel);
    EXPECT_EQ(file5.load(rec4, &from).loaded, 0u);
    EXPECT_STREQ(from, TrackStoreFile::kPath);

    // No whole file: the one with the most tracks, not the first found.
    fx.fileSystem.seedFile(TrackStoreFile::kPath, "garbage\n");
    fx.fileSystem.seedFile(TrackStoreFile::kBakPath, text.substr(0, text.size() - 10)); // the last line cut off
    Oval::TrackStore rec5;
    TrackStoreFile file6(fx.kernel);
    EXPECT_EQ(file6.load(rec5, &from).loaded, 2u);
    EXPECT_STREQ(from, TrackStoreFile::kBakPath);
}

TEST(ServiceTracksFile, AFailedSaveLeavesTheOldStoreToLoad)
{
    // A save that fails leaves no newer tracks.tmp for the next start to
    // take: the service keeps the old store, and so does the file.
    KernelFixture fx;
    TrackStoreFile file(fx.kernel);
    ASSERT_EQ(file.save(storeWith(1)), TrackStoreFile::SaveResult::Saved);
    fx.fileSystem.failRenames = true;
    EXPECT_EQ(file.save(storeWith(3)), TrackStoreFile::SaveResult::Failed);
    fx.fileSystem.failRenames = false;
    EXPECT_FALSE(fx.fileSystem.exist(TrackStoreFile::kTmpPath));
    // Lost too: say the start before had been interrupted.
    fx.fileSystem.remove(TrackStoreFile::kPath);
    Oval::TrackStore back;
    const char* from = nullptr;
    TrackStoreFile file2(fx.kernel);
    file2.load(back, &from);
    EXPECT_EQ(back.count(), 0u) << "not the unsaved store";
}

TEST(ServiceTracksFile, AGarbageAltitudeIsStoredAsNone)
{
    // A finite altitude too large to write in the store's digits would make
    // the track's line unreadable: it is kept as none.
    Oval::TrackStore st;
    const Oval::TrackModel m = Oval::TrackModel::fromGeo(55.95, -3.19, 20.0f, 36.5f, false, 3.0e12f);
    Oval::FitQuality q;
    q.scaleM = 2.0f;
    ASSERT_NE(st.saveNew(m, q, 1791936000u).number, 0u);
    char buf[Oval::TrackStore::kMaxSerialisedBytes];
    const size_t n = st.serialise(buf, sizeof buf);
    Oval::TrackStore back;
    EXPECT_EQ(back.load(buf, n).loaded, 1u);
    const Oval::TrackRecord* rec = back.find(1);
    ASSERT_NE(rec, nullptr);
    EXPECT_TRUE(std::isnan(rec->altM)) << "kept as none";
}

TEST(ServiceTracksFile, ANewerVersionIsNeverSavedOver)
{
    KernelFixture fx;
    const Oval::TrackStore one = storeWith(1);
    char buf[Oval::TrackStore::kMaxSerialisedBytes];
    const size_t n = one.serialise(buf, sizeof buf);
    std::string text(buf, n);
    text.replace(text.find("v=1"), 3, "v=2"); // the header's version
    fx.fileSystem.seedFile(TrackStoreFile::kPath, text);
    TrackStoreFile file(fx.kernel);
    Oval::TrackStore st;
    const Oval::TrackStore::LoadReport r = file.load(st);
    EXPECT_TRUE(r.newerVersion);
    EXPECT_TRUE(file.newerVersion());
    EXPECT_EQ(file.save(storeWith(2)), TrackStoreFile::SaveResult::SkippedNewer);
    EXPECT_EQ(fx.fileSystem.readFile(TrackStoreFile::kPath), text) << "untouched";
}

// ---------------------------------------------------------------------------
// The GPS trace file
// ---------------------------------------------------------------------------

TEST(ServiceTrace, PairsWithTheFitFileAndHoldsTheStore)
{
    KernelFixture fx;
    GpsTraceWriter tw(fx.kernel);
    Run::TraceFormat::Row pre;
    pre.tMs = 1000u; pre.fix = true; pre.lat = 55.9; pre.lon = -3.2; pre.hdop = 4.1f; pre.mode = "pre"; pre.lane = 2;
    tw.preRun(pre);
    TrackStoreFile sf(fx.kernel);
    const Oval::TrackStore st = storeWith(2);
    const char* text = nullptr;
    const size_t len = sf.serialise(st, text);
    ASSERT_TRUE(tw.open("Activity/202610/activity_20261013T150108.fit", 1500u, 2, "1.0.0-test", text, len, 0.25f));
    EXPECT_STREQ(tw.path(), "Activity/202610/trace_20261013T150108.csv");
    Run::TraceFormat::Row r = pre;
    r.tMs = 2000u; r.mode = "count"; r.trackDistM = 3.5f; r.lap = 0; r.segment = "track";
    tw.row(r);
    tw.action("lap_press", 2500u);
    tw.close();
    const std::string csv = fx.fileSystem.readFile("Activity/202610/trace_20261013T150108.csv");
    EXPECT_EQ(csv.rfind("# trackrun-gps-trace v2\n", 0), 0u) << csv;
    EXPECT_NE(csv.find("# start_ms=1500\n"), std::string::npos);
    EXPECT_NE(csv.find("# hdop_scale=0.2500\n"), std::string::npos);
    size_t stores = 0;
    for (size_t p = csv.find("# store=track "); p != std::string::npos; p = csv.find("# store=track ", p + 1)) {
        ++stores;
    }
    EXPECT_EQ(stores, 2u);
    EXPECT_NE(csv.find("t_ms,fix,lat,lon,alt_m,hdop,gps_dist_m,track_dist_m,lap,mode,lane,segment\n"), std::string::npos);
    EXPECT_NE(csv.find("1000,1,55.9000000,-3.2000000,,4.10,,,,pre,2,\n"), std::string::npos) << csv;
    EXPECT_NE(csv.find("2000,1,55.9000000,-3.2000000,,4.10,,3.50,0,count,2,track\n"), std::string::npos) << csv;
    EXPECT_NE(csv.find("# lap_press=2500\n"), std::string::npos);
}

TEST(ServiceTrace, ADiscardedRunLeavesNoTrace)
{
    KernelFixture fx;
    GpsTraceWriter tw(fx.kernel);
    ASSERT_TRUE(tw.open("Activity/202610/activity_20261013T150108.fit", 1500u, 2, "1.0.0-test", nullptr, 0, 0.0f));
    Run::TraceFormat::Row r;
    r.tMs = 2000u; r.fix = true; r.lat = 55.9; r.lon = -3.2; r.mode = "count"; r.lane = 2;
    tw.row(r);
    ASSERT_TRUE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T150108.csv"));
    tw.discard();
    EXPECT_FALSE(tw.isOpen());
    EXPECT_FALSE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T150108.csv"));
}

TEST(ServiceTrace, ADiscardAfterAWriteErrorStillDeletesTheTrace)
{
    KernelFixture fx;
    Run::TraceFormat::Row r;
    r.tMs = 2000u; r.fix = true; r.lat = 55.9; r.lon = -3.2; r.mode = "count"; r.lane = 2;

    // A first run's trace, closed at its stop.
    GpsTraceWriter tw(fx.kernel);
    ASSERT_TRUE(tw.open("Activity/202610/activity_20261013T150108.fit", 1500u, 2, "1.0.0-test", nullptr, 0, 0.0f));
    tw.row(r);
    tw.close();
    ASSERT_TRUE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T150108.csv"));

    // The next run's trace: the card fills, a row fails and the writer stops.
    ASSERT_TRUE(tw.open("Activity/202610/activity_20261013T160000.fit", 1500u, 2, "1.0.0-test", nullptr, 0, 0.0f));
    fx.fileSystem.failWritesAfterBytes = fx.fileSystem.bytesWritten;
    for (int i = 0; i < 200; ++i) {
        r.tMs += 1000u;
        tw.row(r);
    }
    fx.fileSystem.failWritesAfterBytes = static_cast<size_t>(-1);
    EXPECT_FALSE(tw.isOpen()) << "stopped by the write error";
    ASSERT_TRUE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T160000.csv"));
    // That run is discarded: its partial trace goes with it.
    tw.discard();
    EXPECT_FALSE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T160000.csv"));
    // A third run with no trace of its own, discarded: the first run's stays.
    tw.discard();
    EXPECT_TRUE(fx.fileSystem.fileExistsAt("Activity/202610/trace_20261013T150108.csv"));
}
