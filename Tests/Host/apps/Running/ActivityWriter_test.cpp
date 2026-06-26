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

std::vector<uint8_t> findFitFile(const SDK::TestSupport::InMemoryFileSystem& fs)
{
    for (const auto& kv : fs.files) {
        const std::string& path = kv.first;
        if (path.size() > 4 && path.compare(path.size() - 4, 4, ".fit") == 0) {
            const std::string s = fs.readFile(path);
            return std::vector<uint8_t>(s.begin(), s.end());
        }
    }
    return {};
}

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

    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::FileId)).size(), 1u);
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
