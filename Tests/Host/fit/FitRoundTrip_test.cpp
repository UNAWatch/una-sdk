/**
 ******************************************************************************
 * @file    FitRoundTrip_test.cpp
 * @brief   End-to-end validation of the native FIT encoder.
 *
 * Encodes a realistic activity (file_id, timer events, records with a developer
 * field, lap, session, activity) with FitWriter + the FIT profile, then decodes
 * it with a minimal independent reader and asserts the structure, decoded field
 * values, developer-field plumbing, and the trailing file CRC. This is the
 * strongest correctness check available without importing into a FIT consumer.
 ******************************************************************************
 */

#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "FakeFileSystem.hpp"
#include "fit/FitReader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace fit = SDK::Fit;
using testfit::FitReader;

namespace {

std::vector<uint8_t> encodeActivity()
{
    static SDK::Test::FakeFileSystem fs;
    auto file = fs.file("act.fit");
    EXPECT_TRUE(file->open(true, true));

    fit::FitWriter w(*file);
    EXPECT_TRUE(w.begin(/*profileVersion=*/2147));

    using namespace fit::field;
    // file_id (local 0)
    EXPECT_TRUE(w.defineMessage(0, fit::mesgNum(fit::MesgNum::FileId),
        {FileId::Type, FileId::Manufacturer, FileId::Product, FileId::SerialNumber, FileId::TimeCreated}));
    EXPECT_TRUE(w.data(0)
        .u8(static_cast<uint8_t>(fit::File::Activity))
        .u16(static_cast<uint16_t>(fit::Manufacturer::Development))
        .u16(1).u32(12345u).u32(1000u).write());

    // developer_data_id (local 1) + field_description (local 2) for hr_optical
    EXPECT_TRUE(w.defineMessage(1, fit::mesgNum(fit::MesgNum::DeveloperDataId),
        {DeveloperDataId::ApplicationId, DeveloperDataId::DeveloperDataIndex}));
    uint8_t appId[16] = {0x55,0x4e,0x41}; // "UNA..."
    EXPECT_TRUE(w.data(1).bytes(appId, sizeof(appId)).u8(0).write());

    const char* name = "hr_optical";
    const char* units = "bpm";
    const uint8_t nameLen = 11, unitsLen = 4;
    EXPECT_TRUE(w.defineMessage(2, fit::mesgNum(fit::MesgNum::FieldDescription),
        {FieldDescription::DeveloperDataIndex, FieldDescription::FieldDefinitionNumber,
         FieldDescription::FitBaseTypeId,
         {FieldDescription::kFieldNameNum, fit::BaseType::String, nameLen},
         {FieldDescription::kUnitsNum, fit::BaseType::String, unitsLen}}));
    EXPECT_TRUE(w.data(2).u8(0).u8(0).u8(fit::baseTypeId(fit::BaseType::UInt8))
        .str(name, nameLen).str(units, unitsLen).write());

    // event start (local 3)
    EXPECT_TRUE(w.defineMessage(3, fit::mesgNum(fit::MesgNum::Event),
        {Event::Timestamp, Event::EventField, Event::EventType}));
    EXPECT_TRUE(w.data(3).u32(1000u)
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(fit::EventType::Start)).write());

    // record (local 4) with one developer field (hr_optical, dev index 0, field 0)
    EXPECT_TRUE(w.defineMessage(4, fit::mesgNum(fit::MesgNum::Record),
        {Record::Timestamp, Record::PositionLat, Record::PositionLong,
         Record::HeartRate, Record::Cadence, Record::EnhancedSpeed},
        {{/*fieldNum=*/0, /*size=*/1, /*devIndex=*/0}}));
    for (uint32_t i = 0; i < 3; ++i) {
        EXPECT_TRUE(w.data(4)
            .u32(1000u + i)
            .i32(0x10000000 + int32_t(i))
            .i32(-0x10000000 + int32_t(i))
            .u8(static_cast<uint8_t>(60 + i))   // heart_rate
            .u8(85)                              // cadence
            .u32(3000u)                          // enhanced_speed (scaled m/s)
            .u8(static_cast<uint8_t>(61 + i))    // dev hr_optical
            .write());
    }

    // event stop
    EXPECT_TRUE(w.data(3).u32(1003u)
        .u8(static_cast<uint8_t>(fit::Event::Timer))
        .u8(static_cast<uint8_t>(fit::EventType::Stop)).write());

    // lap (local 5)
    EXPECT_TRUE(w.defineMessage(5, fit::mesgNum(fit::MesgNum::Lap),
        {Lap::MessageIndex, Lap::Timestamp, Lap::StartTime, Lap::TotalElapsedTime,
         Lap::TotalDistance, Lap::AvgHeartRate}));
    EXPECT_TRUE(w.data(5).u16(0).u32(1003u).u32(1000u).u32(3000u).u32(5000u).u8(61).write());

    // session (local 6)
    EXPECT_TRUE(w.defineMessage(6, fit::mesgNum(fit::MesgNum::Session),
        {Session::MessageIndex, Session::Timestamp, Session::StartTime, Session::Sport,
         Session::SubSport, Session::TotalDistance, Session::NumLaps, Session::AvgHeartRate}));
    EXPECT_TRUE(w.data(6).u16(0).u32(1003u).u32(1000u)
        .u8(static_cast<uint8_t>(fit::Sport::Running))
        .u8(static_cast<uint8_t>(fit::SubSport::Generic))
        .u32(5000u).u16(1).u8(61).write());

    // activity (local 7)
    EXPECT_TRUE(w.defineMessage(7, fit::mesgNum(fit::MesgNum::Activity),
        {Activity::Timestamp, Activity::TotalTimerTime, Activity::NumSessions,
         Activity::Type, Activity::LocalTimestamp}));
    EXPECT_TRUE(w.data(7).u32(1003u).u32(3000u).u16(1)
        .u8(static_cast<uint8_t>(fit::ActivityType::Manual)).u32(1003u).write());

    EXPECT_TRUE(w.finish());
    EXPECT_TRUE(w.ok());
    file->close();

    const std::string s = fs.fileContents("act.fit");
    return std::vector<uint8_t>(s.begin(), s.end());
}

}  // namespace

TEST(FitRoundTrip, FullActivityDecodesAndVerifies)
{
    const std::vector<uint8_t> bytes = encodeActivity();
    FitReader r(bytes);

    ASSERT_TRUE(r.ok()) << "reader walked all records cleanly";
    EXPECT_TRUE(r.crcValid()) << "trailing file CRC verifies";
    EXPECT_EQ(r.headerSize(), 14);

    // Message inventory.
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::FileId)).size(), 1u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Event)).size(), 2u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Record)).size(), 3u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Lap)).size(), 1u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Session)).size(), 1u);
    EXPECT_EQ(r.withGlobal(fit::mesgNum(fit::MesgNum::Activity)).size(), 1u);

    // file_id values round-trip.
    const auto* fid = r.withGlobal(fit::mesgNum(fit::MesgNum::FileId)).front();
    EXPECT_EQ(fid->fields.at(0).u(), static_cast<uint64_t>(fit::File::Activity));
    EXPECT_EQ(fid->fields.at(1).u(), 255u);   // manufacturer development
    EXPECT_EQ(fid->fields.at(3).u(), 12345u); // serial

    // Records: heart_rate and developer hr_optical increment per sample.
    const auto recs = r.withGlobal(fit::mesgNum(fit::MesgNum::Record));
    ASSERT_EQ(recs.size(), 3u);
    for (uint32_t i = 0; i < 3; ++i) {
        EXPECT_EQ(recs[i]->fields.at(253).u(), 1000u + i);          // timestamp
        EXPECT_EQ(recs[i]->fields.at(3).u(), 60u + i);              // heart_rate
        EXPECT_EQ(recs[i]->fields.at(73).u(), 3000u);              // enhanced_speed
        ASSERT_EQ(recs[i]->devFields.count(0), 1u);                // dev field present
        EXPECT_EQ(recs[i]->devFields.at(0).u(), 61u + i);          // hr_optical
    }

    // session sport/sub_sport enums.
    const auto* ses = r.withGlobal(fit::mesgNum(fit::MesgNum::Session)).front();
    EXPECT_EQ(ses->fields.at(5).u(), static_cast<uint64_t>(fit::Sport::Running));
    EXPECT_EQ(ses->fields.at(6).u(), static_cast<uint64_t>(fit::SubSport::Generic));

    // Byte-stability anchor: encoding is deterministic, so the total size and
    // trailing file CRC are fixed. Guards against accidental output changes.
    const uint16_t fileCrc = SDK::Fit::fitCrcUpdate(0, bytes.data(), bytes.size() - 2);
    EXPECT_EQ(bytes.size(), 369u);
    EXPECT_EQ(fileCrc, 0x96C0u);
}
