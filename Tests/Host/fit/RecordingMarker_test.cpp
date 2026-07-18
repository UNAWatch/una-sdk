/**
 ******************************************************************************
 * @file    RecordingMarker_test.cpp
 * @brief   Host tests for the shared crash-recovery marker + recover
 *          orchestration (SDK::Fit::RecordingMarker). App-independent: every
 *          FIT-writing app reuses this, so it is tested once here.
 ******************************************************************************
 */

#include "SDK/Fit/RecordingMarker.hpp"

#include "SDK/Fit/FitWriter.hpp"
#include "KernelTestDoubles.hpp"
#include "fit/FitReader.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using SDK::Fit::BaseType;
using SDK::Fit::FitWriter;
using SDK::Fit::RecordingMarker;
using SDK::TestSupport::InMemoryFileSystem;

namespace {

// Seed a torn (streamed-but-never-finished) .fit into the file system and
// return the data-end offset a recovery marker would have persisted -- the
// getPosition() after the last complete record.
uint32_t seedTornFit(InMemoryFileSystem& fs, const char* path)
{
    auto file = fs.file(path);
    EXPECT_TRUE(file->open(/*wMode=*/true, /*override=*/true));
    FitWriter w(*file);
    EXPECT_TRUE(w.begin(/*profileVersion=*/0));
    EXPECT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
    EXPECT_TRUE(w.data(0).u32(1000).u8(60).write());
    EXPECT_TRUE(w.data(0).u32(1001).u8(61).write());
    const uint32_t dataEnd = static_cast<uint32_t>(file->getPosition());
    file->flush();
    file->close();
    return dataEnd;  // no finish() -> torn
}

std::vector<uint8_t> bytesOf(const InMemoryFileSystem& fs, const std::string& path)
{
    const std::string s = fs.readFile(path);
    return std::vector<uint8_t>(s.begin(), s.end());
}

}  // namespace

TEST(RecordingMarker, WriteReadRemoveRoundTrip)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");

    ASSERT_TRUE(m.write("Activity/202606/activity.fit", 1234));
    EXPECT_TRUE(fs.exist("Activity/.recording"));

    std::string path;
    uint32_t    offset = 0;
    ASSERT_TRUE(m.read(path, offset));
    EXPECT_EQ(path, "Activity/202606/activity.fit");
    EXPECT_EQ(offset, 1234u);

    m.remove();
    EXPECT_FALSE(fs.exist("Activity/.recording"));

    std::string p2;
    uint32_t    o2 = 0;
    EXPECT_FALSE(m.read(p2, o2)) << "read after remove must fail";
}

TEST(RecordingMarker, UpdateMovesOffsetKeepsPath)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");

    ASSERT_TRUE(m.write("Activity/x.fit", 100));
    ASSERT_TRUE(m.update(500));

    std::string path;
    uint32_t    offset = 0;
    ASSERT_TRUE(m.read(path, offset));
    EXPECT_EQ(path, "Activity/x.fit");
    EXPECT_EQ(offset, 500u);
}

TEST(RecordingMarker, UpdateWithoutWriteFails)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");
    EXPECT_FALSE(m.update(10)) << "update() before any write() has no path";
    EXPECT_FALSE(fs.exist("Activity/.recording"));
}

TEST(RecordingMarker, RecoverFinalizesTornFit)
{
    InMemoryFileSystem fs;
    const char* fitPath = "Activity/202606/activity.fit";
    const uint32_t dataEnd = seedTornFit(fs, fitPath);

    RecordingMarker m(fs, "Activity");
    ASSERT_TRUE(m.write(fitPath, dataEnd));

    const auto res = m.recover();
    EXPECT_TRUE(res.recovered);
    EXPECT_EQ(res.path, fitPath);
    EXPECT_FALSE(fs.exist("Activity/.recording")) << "marker cleared after recovery";
    EXPECT_EQ(fs.openHandles[fitPath], 0u)
        << "recovered .fit must be closed (a leaked write handle pins a FatFs "
           "lock slot until reboot, blocking sync/delete of the activity)";

    const std::vector<uint8_t> b = bytesOf(fs, fitPath);
    testfit::FitReader r(b);
    EXPECT_TRUE(r.ok()) << "recovered file parses";
    EXPECT_TRUE(r.crcValid()) << "recovered file CRC verifies";
    EXPECT_EQ(b.size(), dataEnd + 2u) << "finalized to [0,dataEnd) + CRC";
}

TEST(RecordingMarker, RecoverNoMarkerIsNoop)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");

    const auto res = m.recover();
    EXPECT_FALSE(res.recovered);
    EXPECT_TRUE(res.path.empty());
    EXPECT_TRUE(fs.files.empty()) << "no marker -> no side effects";
}

TEST(RecordingMarker, RecoverMissingFitGivesUpCleanly)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");
    ASSERT_TRUE(m.write("Activity/gone.fit", 40));  // names a file that never existed

    const auto res = m.recover();
    EXPECT_FALSE(res.recovered);
    EXPECT_FALSE(fs.exist("Activity/.recording")) << "marker cleared even on give-up";
}

TEST(RecordingMarker, RecoverBadOffsetGivesUpWithoutClobber)
{
    InMemoryFileSystem fs;
    const char* fitPath = "Activity/torn.fit";
    const uint32_t dataEnd = seedTornFit(fs, fitPath);
    const std::vector<uint8_t> before = bytesOf(fs, fitPath);

    RecordingMarker m(fs, "Activity");
    ASSERT_TRUE(m.write(fitPath, dataEnd + 100));  // beyond EOF -> FitWriter rejects

    const auto res = m.recover();
    EXPECT_FALSE(res.recovered);
    EXPECT_FALSE(fs.exist("Activity/.recording"));
    EXPECT_EQ(bytesOf(fs, fitPath), before) << "out-of-range offset must not clobber the .fit";
}

// Fix 1: a crash during update() can leave the primary marker torn (empty).
// read()/recover() must fall back to the .bak (the previous, slightly older but
// still record-aligned offset) so the fully-flushed .fit is NOT orphaned/lost.
TEST(RecordingMarker, CrashDuringUpdateFallsBackToBak)
{
    InMemoryFileSystem fs;
    const char* fitPath = "Activity/202606/activity.fit";

    // Seed a torn .fit and capture two record-aligned, flushed offsets.
    uint32_t off1 = 0;
    uint32_t off2 = 0;
    {
        auto file = fs.file(fitPath);
        ASSERT_TRUE(file->open(/*wMode=*/true, /*override=*/true));
        FitWriter w(*file);
        ASSERT_TRUE(w.begin(/*profileVersion=*/0));
        ASSERT_TRUE(w.defineMessage(0, 20, {{253, BaseType::UInt32}, {3, BaseType::UInt8}}));
        ASSERT_TRUE(w.data(0).u32(1000).u8(60).write());
        file->flush();
        off1 = static_cast<uint32_t>(file->getPosition());
        ASSERT_TRUE(w.data(0).u32(1001).u8(61).write());
        file->flush();
        off2 = static_cast<uint32_t>(file->getPosition());
        file->close();
    }
    ASSERT_LT(off1, off2);

    RecordingMarker m(fs, "Activity");
    ASSERT_TRUE(m.write(fitPath, off1));   // first marker: primary = off1, no .bak
    ASSERT_TRUE(m.update(off2));           // rotate: .bak = off1, primary = off2
    ASSERT_TRUE(fs.exist("Activity/.recording"));
    ASSERT_TRUE(fs.exist("Activity/.recording.bak"));

    // Simulate a crash mid-update: the primary is left torn (empty), while the
    // previous good copy survives in .bak.
    fs.seedFile("Activity/.recording", "");

    // read() transparently falls back to the .bak's (previous) offset.
    RecordingMarker m2(fs, "Activity");
    std::string path;
    uint32_t    offset = 0;
    ASSERT_TRUE(m2.read(path, offset)) << "torn primary must fall back to .bak";
    EXPECT_EQ(path, fitPath);
    EXPECT_EQ(offset, off1) << ".bak holds the previous flush offset";

    // recover() finalizes the .fit at the recovered offset: the activity is
    // preserved (not lost) and the marker set is fully cleared afterwards.
    const auto res = m2.recover();
    EXPECT_TRUE(res.recovered);
    EXPECT_EQ(res.path, fitPath);
    EXPECT_FALSE(fs.exist("Activity/.recording"));
    EXPECT_FALSE(fs.exist("Activity/.recording.bak"));
    EXPECT_EQ(fs.openHandles[fitPath], 0u) << "recovered .fit must be closed";

    const std::vector<uint8_t> b = bytesOf(fs, fitPath);
    testfit::FitReader r(b);
    EXPECT_TRUE(r.ok()) << "recovered-from-.bak file parses";
    EXPECT_TRUE(r.crcValid()) << "recovered-from-.bak file CRC verifies";
    EXPECT_EQ(b.size(), off1 + 2u) << "finalized to the .bak offset [0,off1) + CRC";
}

// Fix 1: remove() must clear the whole marker set -- primary, .bak and any
// stray .tmp left by an interrupted publish.
TEST(RecordingMarker, RemoveCleansPrimaryBakAndTmp)
{
    InMemoryFileSystem fs;
    RecordingMarker m(fs, "Activity");

    ASSERT_TRUE(m.write("Activity/a.fit", 100));
    ASSERT_TRUE(m.update(200));                          // creates the .bak
    fs.seedFile("Activity/.recording.tmp", "stale");     // stray temp from a crash

    ASSERT_TRUE(fs.exist("Activity/.recording"));
    ASSERT_TRUE(fs.exist("Activity/.recording.bak"));
    ASSERT_TRUE(fs.exist("Activity/.recording.tmp"));

    m.remove();
    EXPECT_FALSE(fs.exist("Activity/.recording"));
    EXPECT_FALSE(fs.exist("Activity/.recording.bak"));
    EXPECT_FALSE(fs.exist("Activity/.recording.tmp"));
}
