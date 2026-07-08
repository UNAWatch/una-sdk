/**
 ******************************************************************************
 * @file    FitWriter_test.cpp
 * @brief   Host tests for the native FIT encoder engine (FitWriter / FitCrc).
 ******************************************************************************
 */

#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitWriter.hpp"
#include "FakeFileSystem.hpp"
#include "fit/FitReader.hpp"

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

TEST(FitWriter, RejectsOversizeArrayField)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("big.fit");
    ASSERT_TRUE(file->open(true, true));

    FitWriter w(*file);
    ASSERT_TRUE(w.begin(1));
    // 64 * 4 bytes = 256 > 255: a field size cannot fit in one byte.
    EXPECT_FALSE(w.defineMessage(0, 20, {{253, BaseType::UInt32, 64}}));
    EXPECT_FALSE(w.ok());
}

// finish() reopens the file for write (override=false) to append the CRC after
// reading it back. That must NOT truncate the streamed data.
TEST(FitWriter, ReopenForAppendPreservesContent)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("a.fit");
    ASSERT_TRUE(file->open(true, true));
    size_t bw = 0;
    ASSERT_TRUE(file->write("ABCDEF", 6, bw));
    ASSERT_TRUE(file->close());

    ASSERT_TRUE(file->open(/*wMode=*/true, /*override=*/false));
    EXPECT_EQ(file->size(), 6u) << "override=false must not truncate existing content";
    ASSERT_TRUE(file->seek(6));
    ASSERT_TRUE(file->write("GH", 2, bw));
    file->close();
    EXPECT_EQ(fs.fileContents("a.fit"), "ABCDEFGH");
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

// --- Crash-safety recovery (FitWriter::recover) -----------------------------

namespace {
// Stream a placeholder header + one definition (global 20: uint32 timestamp +
// uint8 hr) + `nRecords` data records to `path`, WITHOUT calling finish() --
// i.e. the state a file is left in when an activity is interrupted (power loss)
// mid-recording. Returns the data-end offset (getPosition() after the last
// record) that a recovery marker would have persisted as the last
// record-complete boundary.
uint32_t seedUnfinished(SDK::Test::FakeFileSystem& fs, const char* path, int nRecords)
{
    auto file = fs.file(path);
    EXPECT_TRUE(file->open(/*wMode=*/true, /*override=*/true));
    FitWriter w(*file);
    EXPECT_TRUE(w.begin(/*profileVersion=*/0x1234));
    EXPECT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    for (int i = 0; i < nRecords; ++i) {
        EXPECT_TRUE(w.data(0).u32(1000 + static_cast<uint32_t>(i))
                        .u8(static_cast<uint8_t>(60 + i)).write());
    }
    const uint32_t dataEnd = static_cast<uint32_t>(file->getPosition());
    file->close();  // activity interrupted before finish() -- no CRC, placeholder header
    return dataEnd;
}
}  // namespace

// Crash -> reboot -> recover: an unfinished file (placeholder header, no file
// CRC) is turned into a complete, CRC-valid FIT file from only the on-disk
// bytes plus the caller-supplied dataEnd (a fresh handle -- no writer state).
TEST(FitWriter, RecoverCrashedFileIsValid)
{
    SDK::Test::FakeFileSystem fs;
    const uint32_t dataEnd = seedUnfinished(fs, "crash.fit", 5);

    // Simulate a reboot: a brand-new file handle, only disk bytes + dataEnd.
    auto file = fs.file("crash.fit");
    ASSERT_TRUE(FitWriter::recover(*file, dataEnd));

    const std::string s = fs.fileContents("crash.fit");
    const std::vector<uint8_t> b(s.begin(), s.end());
    testfit::FitReader r(b);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid());
    EXPECT_EQ(r.withGlobal(20).size(), 5u);
    EXPECT_EQ(b.size(), dataEnd + 2u);
}

// A torn tail (a partially-written record flushed past the last complete
// record) is trimmed: recover() finalizes exactly [0, dataEnd) + CRC.
TEST(FitWriter, RecoverTrimsTornTail)
{
    SDK::Test::FakeFileSystem fs;
    const uint32_t dataEnd = seedUnfinished(fs, "torn.fit", 4);

    // Append garbage past the last flushed record (a partial post-flush write).
    {
        auto f = fs.file("torn.fit");
        ASSERT_TRUE(f->open(/*wMode=*/true, /*override=*/false));
        ASSERT_TRUE(f->seek(dataEnd));
        const char garbage[] = {0x00, 0x11, 0x22, 0x33, 0x44};
        size_t bw = 0;
        ASSERT_TRUE(f->write(garbage, sizeof(garbage), bw));
        f->close();
    }
    ASSERT_GT(fs.fileContents("torn.fit").size(), static_cast<size_t>(dataEnd));

    auto file = fs.file("torn.fit");
    ASSERT_TRUE(FitWriter::recover(*file, dataEnd));

    const std::string s = fs.fileContents("torn.fit");
    const std::vector<uint8_t> b(s.begin(), s.end());
    testfit::FitReader r(b);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid());
    EXPECT_EQ(r.withGlobal(20).size(), 4u);
    EXPECT_EQ(b.size(), dataEnd + 2u) << "torn tail must be trimmed";
}

// recover() on an already-finish()ed file is an idempotent no-op: returns true
// and leaves the bytes byte-for-byte unchanged and still valid.
TEST(FitWriter, RecoverOnFinishedFileIsNoop)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("done.fit");
    ASSERT_TRUE(file->open(/*wMode=*/true, /*override=*/true));
    FitWriter w(*file);
    ASSERT_TRUE(w.begin(0x1234));
    ASSERT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(w.data(0).u32(1000 + static_cast<uint32_t>(i))
                        .u8(static_cast<uint8_t>(60 + i)).write());
    }
    const uint32_t dataEnd = static_cast<uint32_t>(file->getPosition());
    ASSERT_TRUE(w.finish());
    file->close();

    const std::string before = fs.fileContents("done.fit");

    auto f2 = fs.file("done.fit");
    ASSERT_TRUE(FitWriter::recover(*f2, dataEnd));

    const std::string after = fs.fileContents("done.fit");
    EXPECT_EQ(before, after) << "recover() must not touch an already-finalized file";

    const std::vector<uint8_t> b(after.begin(), after.end());
    testfit::FitReader r(b);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid());
    EXPECT_EQ(r.withGlobal(20).size(), 3u);
}

// An already-finalized file (non-zero on-disk dataSize) that carries extra
// trailing bytes -- e.g. an isolated truncate() failure left a torn tail past
// the CRC -- must be treated as already finalized: recover() returns true and
// leaves the bytes UNCHANGED. Re-finalizing at the marker's older crash-time
// dataEnd would rewrite the header/CRC and truncate away the session/activity
// messages that finish() appended -- silently discarding the recorded activity.
TEST(FitWriter, RecoverOnFinalizedFileWithTrailingBytesIsNoop)
{
    SDK::Test::FakeFileSystem fs;
    auto file = fs.file("trailing.fit");
    ASSERT_TRUE(file->open(/*wMode=*/true, /*override=*/true));
    FitWriter w(*file);
    ASSERT_TRUE(w.begin(0x1234));
    ASSERT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    ASSERT_TRUE(w.data(0).u32(1000).u8(60).write());
    file->flush();
    // The offset a crash marker would have persisted (only 1 record so far),
    // BEFORE finish() appends the rest of the session.
    const uint32_t staleDataEnd = static_cast<uint32_t>(file->getPosition());
    for (int i = 1; i < 4; ++i) {
        ASSERT_TRUE(w.data(0).u32(1000 + static_cast<uint32_t>(i))
                        .u8(static_cast<uint8_t>(60 + i)).write());
    }
    ASSERT_TRUE(w.finish());   // patches header dataSize != 0, appends the CRC
    file->close();

    const std::string finalized = fs.fileContents("trailing.fit");

    // Simulate an isolated truncate() failure: a few bytes linger past the CRC.
    {
        auto f = fs.file("trailing.fit");
        ASSERT_TRUE(f->open(/*wMode=*/true, /*override=*/false));
        ASSERT_TRUE(f->seek(f->size()));
        const char trailing[] = {0x55, static_cast<char>(0xAA), 0x12};
        size_t bw = 0;
        ASSERT_TRUE(f->write(trailing, sizeof(trailing), bw));
        f->close();
    }
    const std::string before = fs.fileContents("trailing.fit");
    ASSERT_GT(before.size(), finalized.size()) << "trailing bytes present";

    // Recover with the STALE (older, 1-record) offset. Must be a no-op.
    auto f2 = fs.file("trailing.fit");
    ASSERT_TRUE(FitWriter::recover(*f2, staleDataEnd));
    const std::string after = fs.fileContents("trailing.fit");
    EXPECT_EQ(after, before)
        << "already-finalized file must not be re-finalized at a stale offset";

    // The full 4-record session is still intact -- not truncated back to the
    // marker's 1-record offset. (The finalized prefix still parses cleanly.)
    const std::vector<uint8_t> prefix(after.begin(), after.begin() + finalized.size());
    testfit::FitReader r(prefix);
    EXPECT_TRUE(r.ok());
    EXPECT_TRUE(r.crcValid());
    EXPECT_EQ(r.withGlobal(20).size(), 4u) << "session data preserved, not discarded";
}

// Bad input must be rejected (return false) without corrupting the file.
TEST(FitWriter, RecoverRejectsBadInputWithoutClobbering)
{
    // (a) dataEnd beyond EOF.
    {
        SDK::Test::FakeFileSystem fs;
        const uint32_t dataEnd = seedUnfinished(fs, "oor.fit", 2);
        const std::string before = fs.fileContents("oor.fit");

        auto file = fs.file("oor.fit");
        EXPECT_FALSE(FitWriter::recover(*file, dataEnd + 100));
        EXPECT_EQ(fs.fileContents("oor.fit"), before) << "out-of-range dataEnd must not clobber";
    }
    // (b) non-FIT header.
    {
        SDK::Test::FakeFileSystem fs;
        const std::string junk = "NOTAFITFILE_________________";  // >=14 bytes, no ".FIT" at 8..11
        fs.seedFile("junk.bin", junk);

        auto file = fs.file("junk.bin");
        EXPECT_FALSE(FitWriter::recover(*file, 14));
        EXPECT_EQ(fs.fileContents("junk.bin"), junk) << "non-FIT header must not be clobbered";
    }
}

// recover() on a NON-EXISTENT path must return false and must NOT create the
// file: it inspects read-only first, so a missing target is never conjured into
// a stray 0-byte .fit (a non-truncating write open would create it).
TEST(FitWriter, RecoverOnMissingFileDoesNotCreateIt)
{
    SDK::Test::FakeFileSystem fs;

    auto file = fs.file("ghost.fit");
    EXPECT_FALSE(FitWriter::recover(*file, 100));
    EXPECT_FALSE(fs.hasFile("ghost.fit")) << "recover() must not create a missing file";
}
