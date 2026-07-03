/**
 * Host unit tests for the simulator's POSIX Mock::FileSystem.
 *
 * The mock is Windows-only elsewhere in the tree, so this suite is POSIX-guarded
 * and runs on the Linux host-tests CI job (which is where the SDK host tests
 * already run). The Windows path is covered separately.
 */
#ifndef _WIN32

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include <sys/resource.h>

#include <gtest/gtest.h>

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Simulator/Kernel/Mock/FileSystem.hpp"

using SDK::Simulator::Mock::FileSystem;
namespace I = SDK::Interface;

namespace {

class MockFileSystemTest : public ::testing::Test {
protected:
    std::string root;

    void SetUp() override
    {
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        const std::filesystem::path dir =
            std::filesystem::temp_directory_path() /
            (std::string("una_fsmock_") + info->name());
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);
        root = dir.string() + "/"; // FileSystem builds paths as prefix + path
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(root, ec);
    }
};

// Loop-open a handle without closing it under a lowered fd limit; RAII must
// reclaim each one, otherwise this exhausts the table and open() fails.
bool opensWithoutLeaking(const std::function<bool()> &openOnce)
{
    struct rlimit orig{};
    if (::getrlimit(RLIMIT_NOFILE, &orig) != 0) {
        return true; // cannot probe; don't fail the suite
    }
    struct rlimit low = orig;
    low.rlim_cur = 96;
    if (::setrlimit(RLIMIT_NOFILE, &low) != 0) {
        return true; // not permitted to lower; skip the probe
    }

    bool ok = true;
    for (int i = 0; i < 1024; ++i) {
        if (!openOnce()) { ok = false; break; }
    }

    ::setrlimit(RLIMIT_NOFILE, &orig); // restore before returning/asserting
    return ok;
}

} // namespace

TEST_F(MockFileSystemTest, MkdirNestedAndExist)
{
    FileSystem fs(root.c_str());
    EXPECT_TRUE(fs.mkdir("x/y/z"));
    EXPECT_TRUE(fs.exist("x/y/z"));
    EXPECT_FALSE(fs.exist("x/y/nope"));
}

TEST_F(MockFileSystemTest, WriteReadSize)
{
    FileSystem fs(root.c_str());
    auto w = fs.file("data.bin");
    size_t bw = 0;
    ASSERT_TRUE(w->open(true, true));
    ASSERT_TRUE(w->write("hello", 5, bw));
    EXPECT_EQ(bw, 5u);
    ASSERT_TRUE(w->close());

    auto r = fs.file("data.bin");
    EXPECT_EQ(r->size(), 5u);
    ASSERT_TRUE(r->open(false));
    char buf[16] = {0};
    size_t br = 0;
    ASSERT_TRUE(r->read(buf, sizeof(buf), br));
    EXPECT_EQ(br, 5u);
    EXPECT_EQ(std::string(buf, 5), "hello");
    r->close();
}

// The single-position contract: seek() then write() overwrites in place; it must
// NOT append (the fstream append-mode bug this fd rewrite replaces).
TEST_F(MockFileSystemTest, SeekWriteOverwritesNotAppends)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("data.bin"); size_t bw = 0;
      ASSERT_TRUE(f->open(true, true));
      ASSERT_TRUE(f->write("AAAAAAAAAA", 10, bw));
      ASSERT_TRUE(f->close()); }

    { auto f = fs.file("data.bin"); size_t bw = 0;
      ASSERT_TRUE(f->open(true, false)); // write, no override
      ASSERT_TRUE(f->seek(3));
      EXPECT_EQ(f->getPosition(), 3u);
      ASSERT_TRUE(f->write("XYZ", 3, bw));
      EXPECT_EQ(bw, 3u);
      ASSERT_TRUE(f->close()); }

    auto f = fs.file("data.bin");
    EXPECT_EQ(f->size(), 10u); // overwrote in place, did not append
    ASSERT_TRUE(f->open(false));
    char buf[16] = {0};
    size_t br = 0;
    ASSERT_TRUE(f->read(buf, sizeof(buf), br));
    EXPECT_EQ(br, 10u);
    EXPECT_EQ(std::string(buf, 10), "AAAXYZAAAA");
    f->close();
}

TEST_F(MockFileSystemTest, Truncate)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("t.bin"); size_t bw = 0;
      ASSERT_TRUE(f->open(true, true));
      ASSERT_TRUE(f->write("ABCDEFGH", 8, bw));
      ASSERT_TRUE(f->close()); }
    { auto f = fs.file("t.bin");
      ASSERT_TRUE(f->open(true, false));
      EXPECT_TRUE(f->truncate(3));
      ASSERT_TRUE(f->close()); }

    auto f = fs.file("t.bin");
    EXPECT_EQ(f->size(), 3u);
    ASSERT_TRUE(f->open(false));
    char buf[16] = {0};
    size_t br = 0;
    ASSERT_TRUE(f->read(buf, sizeof(buf), br));
    f->close();
    EXPECT_EQ(br, 3u);
    EXPECT_EQ(std::string(buf, 3), "ABC");
}

// Every ObjectInfo field must be populated; poison the struct first so a
// left-uninitialized field is caught.
TEST_F(MockFileSystemTest, ObjectInfoInitializesAllFields)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("f.txt"); size_t bw = 0; f->open(true, true); f->write("hi", 2, bw); f->close(); }

    I::IFileSystem::ObjectInfo info;
    std::memset(&info, 0xAB, sizeof(info));
    ASSERT_TRUE(fs.objectInfo("f.txt", info));
    EXPECT_STREQ(info.name, "f.txt");
    EXPECT_FALSE(info.isDir);
    EXPECT_FALSE(info.isHidden);
    EXPECT_FALSE(info.isSystem);
    EXPECT_EQ(info.size, 2u);
}

TEST_F(MockFileSystemTest, ObjectInfoDirAndTrailingSlash)
{
    FileSystem fs(root.c_str());
    ASSERT_TRUE(fs.mkdir("sub/inner"));

    I::IFileSystem::ObjectInfo a;
    std::memset(&a, 0xAB, sizeof(a));
    ASSERT_TRUE(fs.objectInfo("sub/inner", a));
    EXPECT_TRUE(a.isDir);

    I::IFileSystem::ObjectInfo b;
    std::memset(&b, 0xAB, sizeof(b));
    ASSERT_TRUE(fs.objectInfo("sub/inner/", b)); // trailing slash -> leaf "inner"
    EXPECT_STREQ(b.name, "inner");
    EXPECT_TRUE(b.isDir);
}

// readNext is a one-entry-per-call cursor that skips "." and ".."; reset rewinds
// without reading (the contract rryles pinned in IFileSystem.hpp).
TEST_F(MockFileSystemTest, DirectoryEnumeration)
{
    FileSystem fs(root.c_str());
    ASSERT_TRUE(fs.mkdir("list"));
    { auto f = fs.file("list/one.txt"); size_t bw = 0; f->open(true, true); f->write("1", 1, bw); f->close(); }
    { auto f = fs.file("list/two.txt"); size_t bw = 0; f->open(true, true); f->write("22", 2, bw); f->close(); }
    ASSERT_TRUE(fs.mkdir("list/sub"));

    auto d = fs.dir("list");
    ASSERT_TRUE(d->open());

    int files = 0, dirs = 0, dots = 0;
    I::IFileSystem::ObjectInfo it;
    for (;;) {
        std::memset(&it, 0xAB, sizeof(it));
        if (!d->readNext(it)) break;
        if (!std::strcmp(it.name, ".") || !std::strcmp(it.name, "..")) {
            ++dots;
        } else if (it.isDir) {
            ++dirs;
        } else {
            ++files;
        }
    }
    EXPECT_EQ(dots, 0);
    EXPECT_EQ(files, 2);
    EXPECT_EQ(dirs, 1);

    // reset rewinds only (returns true, leaves item alone); next reads re-enumerate.
    EXPECT_TRUE(d->readNext(it, /*reset=*/true));
    int again = 0;
    while (d->readNext(it)) ++again;
    EXPECT_EQ(again, 3);

    d->close();
}

TEST_F(MockFileSystemTest, RenameCopyRemove)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("a.txt"); size_t bw = 0; f->open(true, true); f->write("data", 4, bw); f->close(); }

    EXPECT_TRUE(fs.rename("a.txt", "b.txt"));
    EXPECT_TRUE(fs.exist("b.txt"));
    EXPECT_FALSE(fs.exist("a.txt"));

    EXPECT_TRUE(fs.copy("b.txt", "c.txt"));
    EXPECT_TRUE(fs.exist("c.txt"));

    EXPECT_TRUE(fs.remove("c.txt"));
    EXPECT_FALSE(fs.exist("c.txt"));
}

TEST_F(MockFileSystemTest, FileDestructorReleasesFd)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("seed"); size_t bw = 0; f->open(true, true); f->write("x", 1, bw); f->close(); }

    EXPECT_TRUE(opensWithoutLeaking([&] {
        auto f = fs.file("seed");
        return f->open(false); // no close(): rely on ~File()
    }));
}

TEST_F(MockFileSystemTest, DirectoryDestructorReleasesHandle)
{
    FileSystem fs(root.c_str());
    ASSERT_TRUE(fs.mkdir("d"));

    EXPECT_TRUE(opensWithoutLeaking([&] {
        auto dir = fs.dir("d");
        return dir->open(); // no close(): rely on ~Directory()
    }));
}

// Reopening the same object without an intervening close() must release the
// previous handle; otherwise each open() leaks a descriptor and the lowered
// fd limit is exhausted.
TEST_F(MockFileSystemTest, FileReopenReleasesPreviousFd)
{
    FileSystem fs(root.c_str());
    { auto f = fs.file("seed"); size_t bw = 0; f->open(true, true); f->write("x", 1, bw); f->close(); }

    auto f = fs.file("seed");
    EXPECT_TRUE(opensWithoutLeaking([&] {
        return f->open(false); // no close() between iterations: open() must drop the prior fd
    }));
    f->close();
}

TEST_F(MockFileSystemTest, DirectoryReopenReleasesPreviousHandle)
{
    FileSystem fs(root.c_str());
    ASSERT_TRUE(fs.mkdir("d"));

    auto dir = fs.dir("d");
    EXPECT_TRUE(opensWithoutLeaking([&] {
        return dir->open(); // no close() between iterations: open() must drop the prior DIR*
    }));
    dir->close();
}

#endif // !_WIN32
