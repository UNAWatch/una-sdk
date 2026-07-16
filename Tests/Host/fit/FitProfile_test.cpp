/**
 ******************************************************************************
 * @file    FitProfile_test.cpp
 * @brief   Host tests for the FIT profile constants (SDK::Fit::field / enums).
 ******************************************************************************
 */

#include "SDK/Fit/FitProfile.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "FakeFileSystem.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace fit = SDK::Fit;

TEST(FitProfile, GlobalMessageNumbers)
{
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::FileId), 0u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Session), 18u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Lap), 19u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Record), 20u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Event), 21u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Workout), 26u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::WorkoutStep), 27u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::Activity), 34u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::FieldDescription), 206u);
    EXPECT_EQ(fit::mesgNum(fit::MesgNum::DeveloperDataId), 207u);
}

TEST(FitProfile, SampleFieldDefinitions)
{
    EXPECT_EQ(fit::field::Record::Timestamp.fieldDefNum, 253);
    EXPECT_EQ(fit::field::Record::EnhancedSpeed.fieldDefNum, 73);
    EXPECT_EQ(fit::field::Record::EnhancedAltitude.fieldDefNum, 78);
    EXPECT_EQ(fit::field::Record::StepLength.fieldDefNum, 85);
    EXPECT_EQ(fit::field::Record::StepLength.baseType, fit::BaseType::UInt16);
    EXPECT_EQ(fit::field::Session::AvgSpeed.fieldDefNum, 14);
    EXPECT_EQ(fit::field::Lap::AvgSpeed.fieldDefNum, 13);   // lap/session differ
    EXPECT_EQ(fit::field::DeveloperDataId::ApplicationId.count, 16);
    EXPECT_EQ(fit::field::DeveloperDataId::ApplicationId.baseType, fit::BaseType::Byte);
}

TEST(FitProfile, EnumValues)
{
    EXPECT_EQ(static_cast<uint8_t>(fit::Sport::Running), 1);
    EXPECT_EQ(static_cast<uint8_t>(fit::Sport::Hiking), 17);
    EXPECT_EQ(static_cast<uint8_t>(fit::SubSport::Treadmill), 1);
    EXPECT_EQ(static_cast<uint8_t>(fit::Event::Timer), 0);
    EXPECT_EQ(static_cast<uint8_t>(fit::EventType::Stop), 1);
    EXPECT_EQ(static_cast<uint8_t>(fit::File::Activity), 4);
    EXPECT_EQ(static_cast<uint16_t>(fit::Manufacturer::Development), 255);
    EXPECT_EQ(static_cast<uint16_t>(fit::Manufacturer::Una), 351);
    EXPECT_EQ(static_cast<uint8_t>(fit::WktStepDuration::Open), 5);
}

// Encode a record using profile constants and confirm the definition carries
// the right global number and field triples.
TEST(FitProfile, EncodesRecordViaProfileConstants)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("rec.fit");
    ASSERT_TRUE(file->open(true, true));

    fit::FitWriter w(*file);
    ASSERT_TRUE(w.begin(1));
    ASSERT_TRUE(w.defineMessage(0, fit::mesgNum(fit::MesgNum::Record),
        {fit::field::Record::Timestamp, fit::field::Record::HeartRate}));
    ASSERT_TRUE(w.data(0).u32(1000u).u8(60).write());
    ASSERT_TRUE(w.finish());
    file->close();

    const std::string s = fs.fileContents("rec.fit");
    const std::vector<uint8_t> b(s.begin(), s.end());

    // Definition record begins at byte 14.
    EXPECT_EQ(b[14], 0x40);          // definition, local 0
    EXPECT_EQ(b[17], 20);            // global message number (record) LSB
    EXPECT_EQ(b[18], 0);             // MSB
    EXPECT_EQ(b[19], 2);             // field count
    EXPECT_EQ(b[20], 253);           // timestamp field number
    EXPECT_EQ(b[21], 4);             // size (uint32)
    EXPECT_EQ(b[22], 0x86);          // base type uint32
    EXPECT_EQ(b[23], 3);             // heart_rate field number
    EXPECT_EQ(b[24], 1);             // size (uint8)
    EXPECT_EQ(b[25], 0x02);          // base type uint8
}
