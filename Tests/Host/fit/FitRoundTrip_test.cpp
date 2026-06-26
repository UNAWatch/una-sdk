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

#include <gtest/gtest.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace fit = SDK::Fit;

namespace {

// A minimal, independent FIT reader: parses the header, walks definition/data
// records, decodes each data message's fields by the active local definition,
// and verifies the trailing CRC.
class FitReader {
public:
    struct FieldVal {
        uint8_t              baseType = 0;
        std::vector<uint8_t> raw;
        uint64_t u() const  // little-endian unsigned interpretation
        {
            uint64_t v = 0;
            for (size_t i = 0; i < raw.size(); ++i) v |= uint64_t(raw[i]) << (8 * i);
            return v;
        }
    };
    struct Message {
        uint16_t                       global = 0;
        std::map<uint8_t, FieldVal>    fields;       // native fields by number
        std::map<uint8_t, FieldVal>    devFields;    // dev fields by number
    };

    explicit FitReader(const std::vector<uint8_t>& b) : mData(b) { parse(); }

    bool ok() const { return mOk; }
    bool crcValid() const { return mCrcValid; }
    uint8_t headerSize() const { return mHeaderSize; }
    const std::vector<Message>& messages() const { return mMsgs; }

    std::vector<const Message*> withGlobal(uint16_t g) const
    {
        std::vector<const Message*> out;
        for (const auto& m : mMsgs) if (m.global == g) out.push_back(&m);
        return out;
    }

private:
    struct FieldDef { uint8_t num, size, baseType; };
    struct Def { uint16_t global; std::vector<FieldDef> fields, devFields; };

    void parse()
    {
        if (mData.size() < 14u + 2u) return;
        mHeaderSize = mData[0];
        if (mHeaderSize != 14) return;
        if (mData[8] != '.' || mData[9] != 'F' || mData[10] != 'I' || mData[11] != 'T') return;

        const uint32_t dataSize = mData[4] | (mData[5] << 8) | (mData[6] << 16) | (uint32_t(mData[7]) << 24);
        const size_t   dataEnd  = mHeaderSize + dataSize;
        if (dataEnd + 2 != mData.size()) return;  // trailing CRC must follow data

        const uint16_t want = mData[dataEnd] | (mData[dataEnd + 1] << 8);
        mCrcValid = (fit::fitCrcUpdate(0, mData.data(), dataEnd) == want);

        std::map<uint8_t, Def> defs;  // by local type
        size_t p = mHeaderSize;
        while (p < dataEnd) {
            const uint8_t hdr = mData[p++];
            if (hdr & 0x80) return;             // compressed timestamp: not produced
            const uint8_t local = hdr & 0x0F;
            if (hdr & 0x40) {                   // definition
                Def d;
                p++;                            // reserved
                p++;                            // architecture (little-endian)
                d.global = mData[p] | (mData[p + 1] << 8); p += 2;
                const uint8_t nf = mData[p++];
                for (uint8_t i = 0; i < nf; ++i) { d.fields.push_back({mData[p], mData[p+1], mData[p+2]}); p += 3; }
                if (hdr & 0x20) {
                    const uint8_t nd = mData[p++];
                    for (uint8_t i = 0; i < nd; ++i) { d.devFields.push_back({mData[p], mData[p+1], mData[p+2]}); p += 3; }
                }
                defs[local] = d;
            } else {                            // data
                auto it = defs.find(local);
                if (it == defs.end()) return;
                Message m;
                m.global = it->second.global;
                for (const auto& f : it->second.fields) {
                    FieldVal v; v.baseType = f.baseType;
                    v.raw.assign(mData.begin() + p, mData.begin() + p + f.size); p += f.size;
                    m.fields[f.num] = v;
                }
                for (const auto& f : it->second.devFields) {
                    FieldVal v; v.baseType = 0;
                    v.raw.assign(mData.begin() + p, mData.begin() + p + f.size); p += f.size;
                    m.devFields[f.num] = v;
                }
                mMsgs.push_back(std::move(m));
            }
        }
        mOk = (p == dataEnd);
    }

    const std::vector<uint8_t>& mData;
    std::vector<Message>        mMsgs;
    bool                        mOk        = false;
    bool                        mCrcValid  = false;
    uint8_t                     mHeaderSize = 0;
};

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
    EXPECT_EQ(fileCrc, 0xD93Bu);
}
