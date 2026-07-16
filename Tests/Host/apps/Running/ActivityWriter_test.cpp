/**
 ******************************************************************************
 * @file    ActivityWriter_test.cpp
 * @brief   Host smoke test for the Running ActivityWriter on the native encoder.
 ******************************************************************************
 */

#include "ActivityWriter.hpp"
#include "KernelTestDoubles.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "fit/FitReader.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace fit = SDK::Fit;

namespace {

std::string findFitPath(const SDK::TestSupport::InMemoryFileSystem& fs)
{
    for (const auto& kv : fs.files) {
        const std::string& path = kv.first;
        if (kv.second.exists && path.size() > 4
            && path.compare(path.size() - 4, 4, ".fit") == 0) {
            return path;
        }
    }
    return {};
}

std::vector<uint8_t> findFitFile(const SDK::TestSupport::InMemoryFileSystem& fs)
{
    const std::string path = findFitPath(fs);
    if (path.empty()) {
        return {};
    }
    const std::string s = fs.readFile(path);
    return std::vector<uint8_t>(s.begin(), s.end());
}

constexpr const char* kMarkerPath = "Activity/.recording";

}  // namespace

TEST(RunningActivityWriter, ProducesValidFitFile)
{
    SDK::TestSupport::KernelFixture fx;
    ActivityWriter w(fx.kernel, "D:");

    ActivityWriter::AppInfo info;
    info.timestamp  = 1782475200;  // 2026-06-26 12:00 UTC
    info.appVersion = 0x00010203;
    info.devID      = "UNA";
    info.appID      = "running";
    w.start(info);

    // Plain record (HR only).
    {
        ActivityWriter::RecordData r;
        r.timestamp = info.timestamp;
        r.set(ActivityWriter::RecordData::Field::HEART_RATE);
        r.heartRate = 120;
        r.set(ActivityWriter::RecordData::Field::CADENCE);
        r.cadenceSpm = 170;
        w.addRecord(r);
    }
    // GPS record.
    {
        ActivityWriter::RecordData r;
        r.timestamp = info.timestamp + 1;
        r.set(ActivityWriter::RecordData::Field::COORDS);
        r.latitude = 51.5074f; r.longitude = -0.1278f;
        r.set(ActivityWriter::RecordData::Field::SPEED);  r.speed = 3.0f;
        r.set(ActivityWriter::RecordData::Field::ALTITUDE); r.altitude = 35.0f;
        r.set(ActivityWriter::RecordData::Field::HEART_RATE); r.heartRate = 130;
        r.hrSource = 1; r.hrOpticalBpm = 130;
        w.addRecord(r);
    }
    // GPS + battery record.
    {
        ActivityWriter::RecordData r;
        r.timestamp = info.timestamp + 2;
        r.set(ActivityWriter::RecordData::Field::COORDS);
        r.latitude = 51.5075f; r.longitude = -0.1278f;
        r.set(ActivityWriter::RecordData::Field::BATTERY);
        r.batteryLevel = 90; r.batteryVoltage = 4100;
        r.set(ActivityWriter::RecordData::Field::HEART_RATE); r.heartRate = 140;
        w.addRecord(r);
    }

    ActivityWriter::LapData lap;
    lap.timestamp = info.timestamp + 2; lap.timeStart = info.timestamp;
    lap.duration = 3; lap.elapsed = 3; lap.distance = 9.0f;
    lap.speedAvg = 3.0f; lap.hrAvg = 130;
    w.addLap(lap);

    ActivityWriter::TrackData track;
    track.timestamp = info.timestamp + 2; track.timeStart = info.timestamp;
    track.duration = 3; track.elapsed = 3; track.distance = 9.0f;
    track.speedAvg = 3.0f; track.hrAvg = 130; track.hrMax = 140;
    w.stop(track);

    const std::vector<uint8_t> bytes = findFitFile(fx.fileSystem);
    ASSERT_FALSE(bytes.empty()) << "a .fit file was produced";

    testfit::FitReader r(bytes);
    EXPECT_TRUE(r.ok()) << "records parse cleanly";
    EXPECT_TRUE(r.crcValid()) << "file CRC verifies";

    const auto fileIds = r.withGlobal(fit::mesgNum(fit::MesgNum::FileId));
    ASSERT_EQ(fileIds.size(), 1u);
    EXPECT_EQ(fileIds[0]->fields.at(1).u(), 351u);  // manufacturer = Una
    EXPECT_EQ(fileIds[0]->fields.at(2).u(), 1u);    // product = UnaWatch
    EXPECT_EQ(                                       // product_name (null-terminated string)
        std::string(reinterpret_cast<const char*>(fileIds[0]->fields.at(8).raw.data())),
        "UNA Watch");
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Event)).size(), 1u);  // start only
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Record)).size(), 3u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Lap)).size(), 1u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Session)).size(), 1u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Activity)).size(), 1u);
    // 5 developer field descriptions emitted.
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::FieldDescription)).size(), 5u);

    // Records carry the hr_source/optical/external developer fields (4/5/6).
    const auto recs = r.withGlobal(fit::mesgNum(fit::MesgNum::Record));
    ASSERT_EQ(recs.size(), 3u);
    EXPECT_EQ(recs[0]->fields.at(3).u(), 120u);              // heart_rate
    EXPECT_EQ(recs[0]->devFields.count(4), 1u);              // hr_source dev field
    // GPS+battery record carries battery developer fields 2 and 3.
    EXPECT_EQ(recs[2]->devFields.count(2), 1u);              // batteryLevel
    EXPECT_EQ(recs[2]->devFields.at(2).u(), 90u);
    EXPECT_EQ(recs[2]->devFields.at(3).u(), 4100u);          // batteryVoltage

    // session sport = running.
    const auto* ses = r.withGlobal(fit::mesgNum(fit::MesgNum::Session)).front();
    EXPECT_EQ(ses->fields.at(5).u(), static_cast<uint64_t>(fit::Sport::Running));
}

// The .fit is flushed at start, then only when a record crosses a >=30 s
// boundary (and on every lap). Records at +0/+10/+20/+35/+70 s cross the
// boundary twice.
TEST(RunningActivityWriter, FlushCadence)
{
    SDK::TestSupport::KernelFixture fx;
    ActivityWriter w(fx.kernel, "Activity");

    ActivityWriter::AppInfo info;
    info.timestamp = 1000;  // small base keeps the flush deltas obvious
    info.appID     = "running";
    w.start(info);

    const std::string fitPath = findFitPath(fx.fileSystem);
    ASSERT_FALSE(fitPath.empty());
    const size_t afterStart = fx.fileSystem.flushCounts[fitPath];
    EXPECT_EQ(afterStart, 1u) << "start() flushes the .fit once (header + defs)";

    const std::time_t offsets[] = {0, 10, 20, 35, 70};
    for (std::time_t off : offsets) {
        ActivityWriter::RecordData rec;
        rec.timestamp = info.timestamp + off;
        rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
        rec.heartRate = 120;
        w.addRecord(rec);
    }
    EXPECT_EQ(fx.fileSystem.flushCounts[fitPath] - afterStart, 2u)
        << "flush only when crossing the >=30 s boundary (at +35 and +70)";

    const size_t beforeLap = fx.fileSystem.flushCounts[fitPath];
    ActivityWriter::LapData lap;
    lap.timestamp = info.timestamp + 70;
    lap.timeStart = info.timestamp;
    lap.duration  = 70;
    lap.elapsed   = 70;
    w.addLap(lap);
    EXPECT_EQ(fx.fileSystem.flushCounts[fitPath] - beforeLap, 1u) << "each lap flushes";
}

// End-to-end wiring: a crash (no stop()) leaves the marker; a fresh writer's
// recoverInterrupted() finalizes the .fit and clears the marker.
TEST(RunningActivityWriter, RecoverInterruptedThroughApp)
{
    SDK::TestSupport::KernelFixture fx;

    {
        ActivityWriter w(fx.kernel, "Activity");
        ActivityWriter::AppInfo info;
        info.timestamp = 1782475200;  // 2026-06-26 12:00 UTC
        info.appID     = "running";
        w.start(info);

        ActivityWriter::RecordData rec;
        rec.timestamp = info.timestamp;
        rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
        rec.heartRate = 120;
        w.addRecord(rec);
        rec.timestamp = info.timestamp + 1;
        w.addRecord(rec);

        ActivityWriter::LapData lap;  // laps flush -> marker covers all records
        lap.timestamp = info.timestamp + 1;
        lap.timeStart = info.timestamp;
        lap.duration  = 1;
        lap.elapsed   = 1;
        w.addLap(lap);
        // Crash: no stop() -> file left unfinished, marker present.
    }

    ASSERT_TRUE(fx.fileSystem.exist(kMarkerPath));

    ActivityWriter fresh(fx.kernel, "Activity");
    EXPECT_TRUE(fresh.recoverInterrupted());
    EXPECT_FALSE(fx.fileSystem.exist(kMarkerPath)) << "marker cleared after recovery";

    const std::vector<uint8_t> bytes = findFitFile(fx.fileSystem);
    ASSERT_FALSE(bytes.empty());
    testfit::FitReader r(bytes);
    EXPECT_TRUE(r.ok()) << "recovered .fit parses";
    EXPECT_TRUE(r.crcValid()) << "recovered .fit CRC verifies";

    // Second call: no marker left -> nothing to do.
    EXPECT_FALSE(fresh.recoverInterrupted());
}

// #40: stop() returns whether the activity was durably saved.
TEST(RunningActivityWriter, StopReturnsTrueOnSuccess)
{
    SDK::TestSupport::KernelFixture fx;
    ActivityWriter w(fx.kernel, "Activity");

    ActivityWriter::AppInfo info;
    info.timestamp = 1782475200;
    info.appID     = "running";
    w.start(info);

    ActivityWriter::RecordData rec;
    rec.timestamp = info.timestamp;
    rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
    rec.heartRate = 120;
    w.addRecord(rec);

    ActivityWriter::TrackData track;
    track.timestamp = info.timestamp + 1;
    track.timeStart = info.timestamp;
    track.duration  = 1;
    track.elapsed   = 1;
    EXPECT_TRUE(w.stop(track));
    EXPECT_FALSE(fx.fileSystem.exist(kMarkerPath)) << "success clears the marker";
}

TEST(RunningActivityWriter, StopReturnsFalseOnWriteFailure)
{
    SDK::TestSupport::KernelFixture fx;
    ActivityWriter w(fx.kernel, "Activity");

    ActivityWriter::AppInfo info;
    info.timestamp = 1782475200;
    info.appID     = "running";
    w.start(info);

    ActivityWriter::RecordData rec;
    rec.timestamp = info.timestamp;
    rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
    rec.heartRate = 120;
    w.addRecord(rec);

    // Fail every write from here on: the session/activity/finish writes fail.
    fx.fileSystem.failWritesAfterBytes = fx.fileSystem.bytesWritten;

    ActivityWriter::TrackData track;
    track.timestamp = info.timestamp + 1;
    track.timeStart = info.timestamp;
    track.duration  = 1;
    track.elapsed   = 1;
    EXPECT_FALSE(w.stop(track));
    EXPECT_TRUE(fx.fileSystem.exist(kMarkerPath))
        << "on failure the marker survives so the next boot recovers the .fit";
}

// Decoupling: the FIT finishes/flushes/closes durably but the auxiliary .json
// summary cannot be persisted. stop() must still report success (FIT durability
// is the save contract -- the kernel registers the .fit on close), clear the
// marker, and leave a valid CRC-good .fit. A summary-only failure must never
// suppress the activity's registration.
TEST(RunningActivityWriter, StopSucceedsWhenOnlySummaryFails)
{
    SDK::TestSupport::KernelFixture fx;
    ActivityWriter w(fx.kernel, "Activity");

    ActivityWriter::AppInfo info;
    info.timestamp = 1782475200;
    info.appID     = "running";
    w.start(info);

    ActivityWriter::RecordData rec;
    rec.timestamp = info.timestamp;
    rec.set(ActivityWriter::RecordData::Field::HEART_RATE);
    rec.heartRate = 120;
    w.addRecord(rec);

    // Fail ONLY the summary: the .json open() fails, which happens after the
    // .fit has been durably finished, flushed and closed. The FIT writes are
    // untouched.
    fx.fileSystem.failWriteOpenSuffix = ".json";

    ActivityWriter::TrackData track;
    track.timestamp = info.timestamp + 1;
    track.timeStart = info.timestamp;
    track.duration  = 1;
    track.elapsed   = 1;

    EXPECT_TRUE(w.stop(track))
        << "FIT durable -> stop() succeeds despite the summary failure";
    EXPECT_FALSE(fx.fileSystem.exist(kMarkerPath)) << "marker cleared: FIT durably saved";

    // No .json persisted, but the .fit is present and CRC-valid.
    const std::vector<uint8_t> fit = findFitFile(fx.fileSystem);
    ASSERT_FALSE(fit.empty()) << ".fit present on disk";
    testfit::FitReader r(fit);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid()) << "recovered .fit CRC is valid";
}
