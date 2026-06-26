/**
 ******************************************************************************
 * @file    FitWriter_test.cpp
 * @brief   Host tests for the native FIT encoder engine (FitWriter / FitCrc).
 ******************************************************************************
 */

#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "FakeFileSystem.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using SDK::Fit::BaseType;
using SDK::Fit::FitWriter;

// FIT's file CRC is CRC-16/ARC; its canonical check value over "123456789"
// is 0xBB3D. This is an independent known-answer check on the algorithm.
TEST(FitCrc, KnownAnswerVectors)
{
    EXPECT_EQ(SDK::Fit::fitCrcByte(0, 0x00), 0x0000);
    EXPECT_EQ(SDK::Fit::fitCrcUpdate(0, "\x01", 1), 0xC0C1);
    EXPECT_EQ(SDK::Fit::fitCrcUpdate(0, "123456789", 9), 0xBB3D);
}

TEST(FitWriter, EncodesMinimalFileByteExact)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("activity.fit");
    ASSERT_TRUE(file->open(/*wMode=*/true, /*override=*/true));

    FitWriter w(*file);
    ASSERT_TRUE(w.begin(/*profileVersion=*/0x1234));
    // Synthetic message: global 20, timestamp(field 253, uint32) + hr(field 3, uint8).
    ASSERT_TRUE(w.defineMessage(0, 20,
        {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    ASSERT_TRUE(w.data(0).u32(1000).u8(60).write());
    ASSERT_TRUE(w.finish());
    ASSERT_TRUE(w.ok());
    file->close();

    const std::string s = fs.fileContents("activity.fit");
    const std::vector<uint8_t> b(s.begin(), s.end());

    // Header(14) + definition(12) + data(6) + CRC(2) = 34 bytes.
    ASSERT_EQ(b.size(), 34u);

    std::vector<uint8_t> expectedNoCrc = {
        // --- 14-byte header ---
        14,            // header size
        0x20,          // protocol version (default 2.0)
        0x34, 0x12,    // profile version 0x1234, little-endian
        0x12, 0x00, 0x00, 0x00,  // data size = 18, little-endian
        '.', 'F', 'I', 'T',
        0x00, 0x00,    // header CRC: computed below
        // --- definition record (local 0) ---
        0x40,          // definition, no dev data, local 0
        0x00,          // reserved
        0x00,          // architecture: little-endian
        0x14, 0x00,    // global message number 20
        0x02,          // field count
        253, 4, 0x86,  // timestamp: num 253, size 4, base type uint32
        3,   1, 0x02,  // heart_rate: num 3, size 1, base type uint8
        // --- data record (local 0) ---
        0x00,                    // data header, local 0
        0xE8, 0x03, 0x00, 0x00,  // timestamp = 1000
        0x3C,                    // heart_rate = 60
    };
    ASSERT_EQ(expectedNoCrc.size(), 32u);
    // Header CRC (bytes 12-13) is computed over header bytes 0-11.
    const uint16_t hdrCrc = SDK::Fit::fitCrcUpdate(0, expectedNoCrc.data(), 12);
    expectedNoCrc[12] = static_cast<uint8_t>(hdrCrc & 0xFF);
    expectedNoCrc[13] = static_cast<uint8_t>((hdrCrc >> 8) & 0xFF);
    for (size_t i = 0; i < expectedNoCrc.size(); ++i) {
        EXPECT_EQ(b[i], expectedNoCrc[i]) << "byte " << i;
    }

    // Trailing CRC is little-endian over all preceding bytes.
    const uint16_t crc = SDK::Fit::fitCrcUpdate(0, b.data(), 32);
    EXPECT_EQ(b[32], static_cast<uint8_t>(crc & 0xFF));
    EXPECT_EQ(b[33], static_cast<uint8_t>((crc >> 8) & 0xFF));
}

TEST(FitWriter, RejectsPayloadSizeMismatch)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("bad.fit");
    ASSERT_TRUE(file->open(true, true));

    FitWriter w(*file);
    ASSERT_TRUE(w.begin(1));
    ASSERT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    // Only 4 bytes supplied for a 5-byte definition.
    EXPECT_FALSE(w.data(0).u32(1000).write());
    EXPECT_FALSE(w.ok());
}

TEST(FitWriter, EmitsDeveloperFieldDefinition)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("dev.fit");
    ASSERT_TRUE(file->open(true, true));

    FitWriter w(*file);
    ASSERT_TRUE(w.begin(1));
    // record (global 20) with native hr(field 3, uint8) + one developer field.
    ASSERT_TRUE(w.defineMessage(1, 20,
        {{3, BaseType::UInt8}},
        {{/*fieldNum=*/0, /*sizeBytes=*/1, /*devDataIndex=*/0}}));
    ASSERT_TRUE(w.data(1).u8(70).u8(99).write());  // native hr + 1 dev byte
    ASSERT_TRUE(w.finish());
    file->close();

    const std::string s = fs.fileContents("dev.fit");
    const std::vector<uint8_t> b(s.begin(), s.end());

    // Definition record from index 14:
    //  14 hdr | 15 reserved | 16 arch | 17-18 global(20) | 19 field count
    //  20-22 native field (3,1,uint8) | 23 dev field count | 24-26 dev (0,1,0)
    EXPECT_EQ(b[14], 0x40 | 0x20 | 0x01);  // definition + developer-data flag, local 1
    EXPECT_EQ(b[19], 0x01);                // native field count
    EXPECT_EQ(b[20], 0x03);                // native field number
    EXPECT_EQ(b[23], 0x01);                // developer field count
    EXPECT_EQ(b[24], 0x00);                // dev field number
    EXPECT_EQ(b[25], 0x01);                // dev field size
    EXPECT_EQ(b[26], 0x00);                // developer data index
}
