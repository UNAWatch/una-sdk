/**
 ******************************************************************************
 * @file    KernelTestDoubles_test.cpp
 * @brief   Tests for the shared host-test doubles' instrumentation, so the
 *          assertions other suites build on it stay trustworthy.
 ******************************************************************************
 */

#include "KernelTestDoubles.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using SDK::TestSupport::InMemoryFileSystem;

TEST(InMemoryFileSystem, InstrumentationReadsTheSameForEitherSpellingOfAPath)
{
    InMemoryFileSystem fs;
    auto f = fs.file("/Activity/a.fit");
    ASSERT_TRUE(f->open(/*wMode=*/true));
    ASSERT_TRUE(f->flush());

    EXPECT_EQ(fs.openHandles["/Activity/a.fit"], 1u);
    EXPECT_EQ(fs.openHandles["Activity/a.fit"], 1u);
    EXPECT_EQ(fs.openHandles.at("/Activity/a.fit/"), 1u);
    EXPECT_EQ(fs.flushCounts.count("/Activity/a.fit"), 1u);
    EXPECT_EQ(fs.flushCounts["Activity/a.fit"], 1u);
}

// The open-handle instrumentation is keyed by path: an IFile::rename() on an
// OPEN handle must migrate the count to the new path. Without the migration
// the old bucket stays nonzero forever (a phantom leak) and the eventual
// close() drains the new bucket it never incremented.
TEST(InMemoryFileSystem, RenameWhileOpenMigratesOpenHandleCount)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/false));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 1u);

    ASSERT_TRUE(f->rename("dir/b.txt"));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u) << "old bucket released";
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 1u) << "count follows the handle";

    EXPECT_TRUE(f->close());
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 0u) << "no underflow after close";
}

// A rename of a CLOSED handle must leave the instrumentation untouched.
TEST(InMemoryFileSystem, RenameWhileClosedLeavesHandleCountsAlone)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->rename("dir/b.txt"));
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u);
    EXPECT_EQ(fs.openHandles["dir/b.txt"], 0u);
}

TEST(InMemoryFileSystem, SetPathOnAnOpenFileClosesItRatherThanRetargeting)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "AAA");
    fs.seedFile("b.txt", "BBB");

    auto f = fs.file("a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/true));
    ASSERT_EQ(fs.openHandles["a.txt"], 1u);

    f->setPath("b.txt");
    EXPECT_FALSE(f->isOpen()) << "the handle no longer refers to what it opened";
    EXPECT_EQ(fs.openHandles["a.txt"], 0u) << "the old bucket is released, not stranded";
    EXPECT_EQ(fs.openHandles["b.txt"], 0u) << "and the new path was never opened";

    size_t bw = 0;
    EXPECT_FALSE(f->write("ZZ", 2, bw)) << "a closed handle cannot write";
    EXPECT_EQ(fs.readFile("b.txt"), "BBB") << "b.txt must not be touched";
    EXPECT_EQ(fs.readFile("a.txt"), "AAA");

    EXPECT_FALSE(f->close()) << "already closed";
    EXPECT_EQ(fs.openHandles["b.txt"], 0u) << "no size_t underflow to SIZE_MAX";
    EXPECT_EQ(fs.openHandles["a.txt"], 0u);
}

TEST(InMemoryFileSystem, ClosingWhatWasNeverOpenedFails)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");
    ASSERT_TRUE(fs.mkdir("d"));

    EXPECT_FALSE(fs.file("a.txt")->close()) << "never opened";
    EXPECT_FALSE(fs.file("never-seeded.txt")->close());
    EXPECT_FALSE(fs.dir("d")->close()) << "never opened";
    EXPECT_FALSE(fs.dir("does-not-exist")->close());

    auto f = fs.file("a.txt");
    ASSERT_TRUE(f->open());
    EXPECT_TRUE(f->close()) << "an open handle closes";
    EXPECT_FALSE(f->close()) << "but not twice";

    auto d = fs.dir("d");
    ASSERT_TRUE(d->open());
    EXPECT_TRUE(d->close());
    EXPECT_FALSE(d->close()) << "but not twice";
}

// The closeGate fault hook fails an open handle's close() and leaves it open
// (FatFs f_close semantics: a failed sync keeps the FIL valid and its
// lock-table entry held); once the gate is lifted, close() drains normally.
TEST(InMemoryFileSystem, CloseGateFailsCloseAndKeepsHandleOpen)
{
    InMemoryFileSystem fs;
    fs.seedFile("dir/a.txt", "payload");

    auto f = fs.file("dir/a.txt");
    ASSERT_TRUE(f->open(/*wMode=*/false));

    fs.closeGate = [](const std::string&) { return false; };
    EXPECT_FALSE(f->close());
    EXPECT_TRUE(f->isOpen()) << "a failed close keeps the handle open";
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 1u) << "lock still held";

    fs.closeGate = nullptr;
    EXPECT_TRUE(f->close());
    EXPECT_EQ(fs.openHandles["dir/a.txt"], 0u);
}

namespace {

std::vector<std::string> listDir(InMemoryFileSystem& fs, const char* path)
{
    std::vector<std::string> out;
    auto dir = fs.dir(path);
    if (!dir || !dir->open()) {
        return out;
    }
    SDK::Interface::IFileSystem::ObjectInfo item{};
    while (dir->readNext(item)) {
        out.push_back(std::string(item.name) + (item.isDir ? "/" : ""));
    }
    dir->close();
    return out;
}

} // namespace

TEST(InMemoryDirectory, ListsDirectChildrenOnly)
{
    InMemoryFileSystem fs;
    fs.seedFile("Activity/morning.fit", "aaa");
    fs.seedFile("Activity/evening.fit", "bb");
    fs.seedFile("Settings/app.json", "x");

    EXPECT_EQ(listDir(fs, "Activity"),
              (std::vector<std::string>{ "morning.fit", "evening.fit" }));
}

TEST(InMemoryDirectory, ReportsAnImpliedSubdirectoryWithIsDirSet)
{
    InMemoryFileSystem fs;
    fs.seedFile("root/file.txt", "x");
    fs.seedFile("root/nested/deep.txt", "y");

    EXPECT_EQ(listDir(fs, "root"), (std::vector<std::string>{ "file.txt", "nested/" }));
}

TEST(InMemoryDirectory, DoesNotFlattenNestedFilesIntoTheParent)
{
    InMemoryFileSystem fs;
    fs.seedFile("a/b/c/deep.txt", "x");

    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "b/" }));
    EXPECT_EQ(listDir(fs, "a/b"), (std::vector<std::string>{ "c/" }));
    EXPECT_EQ(listDir(fs, "a/b/c"), (std::vector<std::string>{ "deep.txt" }));
}

TEST(InMemoryDirectory, RootCanBeSpelledSlashOrEmpty)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");
    fs.seedFile("sub/inner.txt", "z");

    const std::vector<std::string> expected{ "a.txt", "sub/" };
    EXPECT_EQ(listDir(fs, "/"), expected);
    EXPECT_EQ(listDir(fs, ""), expected) << R"("" and "/" name the same place)";
}

TEST(InMemoryDirectory, AbsoluteAndRelativeSpellingsShareTheRoot)
{
    InMemoryFileSystem fs;
    fs.seedFile("/leading-slash.txt", "x");
    fs.seedFile("bare.txt", "y");

    EXPECT_EQ(listDir(fs, "/"),
              (std::vector<std::string>{ "leading-slash.txt", "bare.txt" }));
}

TEST(InMemoryDirectory, SpellingsAgreeAtEveryDepthAndForLookupToo)
{
    InMemoryFileSystem fs;
    fs.seedFile("/App/nested.txt", "x");

    const std::vector<std::string> expected{ "nested.txt" };
    EXPECT_EQ(listDir(fs, "App"), expected);
    EXPECT_EQ(listDir(fs, "/App"), expected);
    EXPECT_EQ(listDir(fs, "/App/"), expected);

    EXPECT_TRUE(fs.exist("App/nested.txt"));
    EXPECT_TRUE(fs.exist("/App/nested.txt"));
    EXPECT_EQ(fs.readFile("App/nested.txt"), "x");
}

TEST(InMemoryDirectory, AListedNameIsOpenableAsSlashPlusName)
{
    InMemoryFileSystem fs;
    const std::string contents = "seeded without a leading slash";
    fs.seedFile("bare.txt", contents);

    auto dir = fs.dir("/");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    dir->close();

    const std::string path = std::string("/") + item.name;
    EXPECT_TRUE(fs.exist(path.c_str())) << path << " came out of a listing of \"/\"";
    auto file = fs.file(path.c_str());
    ASSERT_TRUE(file && file->open());
    EXPECT_EQ(file->size(), contents.size());
    file->close();
    EXPECT_EQ(fs.readFile(path), contents);
}

TEST(InMemoryDirectory, ListingGuaranteesStopAtTheNameCapacity)
{
    constexpr size_t kNameCapacity = sizeof(SDK::Interface::IFileSystem::ObjectInfo::name);
    InMemoryFileSystem fs;
    const std::string fits(kNameCapacity - 1, 'a');
    const std::string tooLong(kNameCapacity, 'b');
    fs.seedFile("d/" + fits, "x");
    fs.seedFile("d/" + tooLong, "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    size_t joinable = 0;
    size_t clipped = 0;
    while (dir->readNext(item)) {
        const std::string name(item.name);
        EXPECT_EQ(name.size(), kNameCapacity - 1) << "both clip or fit at the capacity";
        if (fs.exist(("d/" + name).c_str())) {
            ++joinable;
        } else {
            ++clipped;
        }
    }
    ASSERT_TRUE(dir->close());
    EXPECT_EQ(joinable, 1u) << "the name that fits joins back onto its parent";
    EXPECT_EQ(clipped, 1u) << "the one that does not names nothing";

    InMemoryFileSystem twins;
    twins.seedFile("d/" + std::string(kNameCapacity, 'c') + "ONE", "1");
    twins.seedFile("d/" + std::string(kNameCapacity, 'c') + "TWO", "2");
    auto twinDir = twins.dir("d");
    ASSERT_TRUE(twinDir->open());
    std::vector<std::string> seen;
    while (twinDir->readNext(item)) {
        seen.push_back(item.name);
    }
    ASSERT_TRUE(twinDir->close());
    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[0], seen[1]) << "the one case where a listing repeats a name";
}

TEST(InMemoryDirectory, AListingNeverReportsTheSameNameTwice)
{
    InMemoryFileSystem fs;
    fs.seedFile("/App/a.txt", "x");
    fs.seedFile("App/b.txt", "y");
    ASSERT_TRUE(fs.mkdir("App"));
    fs.seedFile("/solo.txt", "z");
    fs.seedFile("solo.txt", "z");

    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "App/", "solo.txt" }));
    EXPECT_EQ(listDir(fs, "App"), (std::vector<std::string>{ "a.txt", "b.txt" }));
}

TEST(InMemoryDirectory, DotSegmentsAreNotResolved)
{
    InMemoryFileSystem fs;
    fs.seedFile("a/b/c.txt", "x");

    EXPECT_FALSE(fs.exist("a/./b/c.txt"));
    EXPECT_FALSE(fs.exist("a/b/../b/c.txt"));

    fs.seedFile("a/./d.txt", "y");
    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "b/", "./" }));
    EXPECT_EQ(listDir(fs, "a/."), (std::vector<std::string>{ "d.txt" }));
}

TEST(InMemoryDirectory, DotPrefixedNamesReportAsHidden)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/.hidden", "x");
    fs.seedFile("d/plain.txt", "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, ".hidden");
    EXPECT_TRUE(item.isHidden);
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "plain.txt");
    EXPECT_FALSE(item.isHidden);
    dir->close();

    ASSERT_TRUE(fs.objectInfo("d/.hidden", item));
    EXPECT_TRUE(item.isHidden);
}

TEST(InMemoryDirectory, CreateDoesNotInventParents)
{
    InMemoryFileSystem fs;
    EXPECT_FALSE(fs.dir("x/y/z")->create()) << "parent \"x/y\" does not exist";
    EXPECT_FALSE(fs.exist("x"));

    EXPECT_TRUE(fs.dir("x")->create());
    EXPECT_TRUE(fs.dir("x/y")->create());
    EXPECT_TRUE(fs.exist("x/y"));

    EXPECT_TRUE(fs.mkdir("p/q/r")) << "mkdir creates parents";
    EXPECT_TRUE(fs.exist("p/q"));

    EXPECT_TRUE(fs.dir("/")->create()) << "the root exists, so creating it succeeds";
    EXPECT_TRUE(fs.dir("")->create());
}

TEST(InMemoryDirectory, MkdirRefusesANameAFileAlreadyHolds)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_FALSE(fs.mkdir("a.txt"));
    EXPECT_FALSE(fs.dir("a.txt")->create());
    EXPECT_FALSE(fs.dir("a.txt")->open()) << "still just a file";
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" }));

    EXPECT_FALSE(fs.mkdir("a.txt/below"));
}

TEST(InMemoryDirectory, AFileCannotShadowADirectory)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("d/inner.txt", "child");

    EXPECT_FALSE(fs.file("d")->open(/*wMode=*/true)) << "\"d\" is a directory";
    EXPECT_FALSE(fs.seedFile("d", "clobber")) << "and seeding cannot do it either";

    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "d/" }));
    EXPECT_EQ(listDir(fs, "d"), (std::vector<std::string>{ "inner.txt" }));
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(fs.objectInfo("d", item));
    EXPECT_TRUE(item.isDir);

    InMemoryFileSystem implied;
    implied.seedFile("imp/child.txt", "x");
    EXPECT_FALSE(implied.file("imp")->open(/*wMode=*/true));
    EXPECT_FALSE(implied.seedFile("imp", "clobber"));

    EXPECT_FALSE(fs.file("/")->open(/*wMode=*/true));
}

TEST(InMemoryDirectory, NothingCanBeCreatedUnderAFile)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_FALSE(fs.mkdir("a.txt/below"));
    EXPECT_FALSE(fs.dir("a.txt/below")->create());
    EXPECT_FALSE(fs.file("a.txt/below/deep.bin")->open(/*wMode=*/true))
        << "an ancestor is a file";
    EXPECT_FALSE(fs.seedFile("a.txt/below/deep.bin", "y"));

    EXPECT_EQ(fs.readFile("a.txt"), "x") << "the file is untouched";
    EXPECT_FALSE(fs.dir("a.txt")->open()) << "and never became a directory";
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" }));
}

TEST(InMemoryDirectory, SetPathInvalidatesAScanInProgress)
{
    InMemoryFileSystem fs;
    fs.seedFile("one/a.txt", "x");
    fs.seedFile("two/b.txt", "y");

    auto dir = fs.dir("one");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "a.txt");

    dir->setPath("two");
    EXPECT_FALSE(dir->isOpen()) << "the handle no longer refers to what it opened";
    EXPECT_FALSE(dir->readNext(item)) << "and must not keep serving \"one\"";

    ASSERT_TRUE(dir->open());
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "b.txt");
}

TEST(InMemoryFileSystem, ANullPathYieldsNoHandleForEitherKind)
{
    InMemoryFileSystem fs;
    EXPECT_EQ(fs.file(nullptr), nullptr);
    EXPECT_EQ(fs.dir(nullptr), nullptr) << "dir() and file() agree on a null path";
}

TEST(InMemoryFileSystem, ExistSeparatesTheRootFromAnEmptyPath)
{
    InMemoryFileSystem fs;
    EXPECT_TRUE(fs.exist("/")) << "the root exists even when nothing is seeded";
    EXPECT_FALSE(fs.exist(""))
        << "an empty path names nothing";
    EXPECT_FALSE(fs.exist(nullptr));

    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(fs.objectInfo("", item));
    EXPECT_FALSE(fs.objectInfo(nullptr, item));
}

TEST(InMemoryDirectory, WhatCodeCreatesThroughTheInterfaceListsInCreationOrder)
{
    InMemoryFileSystem fs;
    const auto write = [&fs](const char* path, const char* text) {
        auto f = fs.file(path);
        ASSERT_TRUE(f->open(/*wMode=*/true, /*override=*/true));
        size_t written = 0;
        ASSERT_TRUE(f->write(text, std::strlen(text), written));
        ASSERT_TRUE(f->close());
    };
    write("d/zulu.txt", "z");
    write("d/alpha.txt", "a");
    ASSERT_TRUE(fs.dir("d/sub")->create());
    ASSERT_TRUE(fs.copy("d/alpha.txt", "d/copy.txt"));
    EXPECT_EQ(listDir(fs, "d"),
              (std::vector<std::string>{ "zulu.txt", "alpha.txt", "sub/", "copy.txt" }));

    write("d/zulu.txt", "again");
    ASSERT_TRUE(fs.copy("d/zulu.txt", "d/alpha.txt"));
    ASSERT_TRUE(fs.dir("d/sub")->create());
    EXPECT_EQ(listDir(fs, "d"),
              (std::vector<std::string>{ "zulu.txt", "alpha.txt", "sub/", "copy.txt" }))
        << "an overwrite, a copy onto a file and creating an existing directory keep their place";

    auto held = fs.file("d/alpha.txt");
    ASSERT_TRUE(held->open(/*wMode=*/true));
    ASSERT_TRUE(fs.remove("d/alpha.txt"));
    size_t written = 0;
    ASSERT_TRUE(held->write("a", 1, written));
    ASSERT_TRUE(held->close());
    EXPECT_EQ(listDir(fs, "d"),
              (std::vector<std::string>{ "zulu.txt", "sub/", "copy.txt", "alpha.txt" }))
        << "a write through a handle whose file was removed creates it again";

    ASSERT_TRUE(fs.remove("d/zulu.txt"));
    ASSERT_TRUE(fs.remove("d/copy.txt"));
    ASSERT_TRUE(fs.copy("d/alpha.txt", "d/copy.txt"));
    write("d/zulu.txt", "z");
    EXPECT_EQ(listDir(fs, "d"),
              (std::vector<std::string>{ "sub/", "alpha.txt", "copy.txt", "zulu.txt" }))
        << "a re-created file goes last";
}

TEST(InMemoryDirectory, AnImpliedDirectorySitsWhereItsOldestContentDoes)
{
    InMemoryFileSystem fs;
    fs.seedFile("r/a/x.txt", "x");
    fs.seedFile("r/b.txt", "b");
    for (int i = 0; i < 16; ++i) {
        fs.seedFile("r/a/y" + std::to_string(i) + ".txt", "y");
    }
    EXPECT_EQ(listDir(fs, "r"), (std::vector<std::string>{ "a/", "b.txt" }));

    ASSERT_TRUE(fs.remove("r/a/x.txt"));
    EXPECT_EQ(listDir(fs, "r"), (std::vector<std::string>{ "b.txt", "a/" }));

    ASSERT_TRUE(fs.mkdir("r/c/deeper"));
    EXPECT_EQ(listDir(fs, "r"), (std::vector<std::string>{ "b.txt", "a/", "c/" }))
        << "a directory beneath counts as content";
}

TEST(InMemoryFileSystem, AWriteCannotRecreateAFileWhereADirectoryNowIs)
{
    InMemoryFileSystem fs;
    auto f = fs.file("a");
    ASSERT_TRUE(f->open(/*wMode=*/true));
    ASSERT_TRUE(fs.remove("a"));
    ASSERT_TRUE(fs.mkdir("a"));

    size_t written = 0;
    EXPECT_FALSE(f->write("z", 1, written));
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a/" }));
    EXPECT_FALSE(fs.files.count("a") != 0 && fs.files.at("a").exists);
}

TEST(InMemoryDirectory, MakingAnImpliedDirectoryExplicitKeepsItsPlace)
{
    for (const bool viaCreate : { false, true }) {
        InMemoryFileSystem fs;
        fs.seedFile("a/x.txt", "x");
        fs.seedFile("b.txt", "b");
        if (viaCreate) {
            ASSERT_TRUE(fs.dir("a")->create());
        } else {
            ASSERT_TRUE(fs.mkdir("a"));
        }
        ASSERT_TRUE(fs.remove("a/x.txt"));
        EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a/", "b.txt" })) << viaCreate;
    }
    InMemoryFileSystem fs;
    fs.seedFile("a/x.txt", "x");
    ASSERT_TRUE(fs.dir("a/b")->create());
    fs.seedFile("z.txt", "z");
    ASSERT_TRUE(fs.remove("a/x.txt"));
    ASSERT_TRUE(fs.mkdir("a"));
    ASSERT_TRUE(fs.remove("a/b"));
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a/", "z.txt" }))
        << "held only by a directory beneath it";
}

TEST(InMemoryDirectory, EnumerationIsInCreationOrderAndReproducible)
{
    InMemoryFileSystem fs;
    for (const char* name : { "zulu", "alpha", "mike", "bravo", "yankee" }) {
        fs.seedFile(std::string("d/") + name + ".txt", "x");
    }

    const std::vector<std::string> expected{
        "zulu.txt", "alpha.txt", "mike.txt", "bravo.txt", "yankee.txt"
    };
    EXPECT_EQ(listDir(fs, "d"), expected);
    EXPECT_EQ(listDir(fs, "d"), expected) << "and stable across repeated listings";

    fs.seedFile("d/alpha.txt", "overwritten");
    EXPECT_EQ(listDir(fs, "d"), expected) << "an overwrite keeps its place";

    ASSERT_TRUE(fs.rename("d/zulu.txt", "d/xray.txt"));
    ASSERT_TRUE(fs.remove("d/mike.txt"));
    fs.seedFile("d/mike.txt", "x");
    ASSERT_TRUE(fs.mkdir("d/sub"));
    EXPECT_EQ(listDir(fs, "d"),
              (std::vector<std::string>{ "alpha.txt", "bravo.txt", "yankee.txt", "xray.txt",
                                         "mike.txt", "sub/" }))
        << "a rename, a re-creation and a new directory each go last";
}

TEST(InMemoryDirectory, ReportsEntrySize)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", std::string(1234, 'x'));

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt");
    EXPECT_EQ(item.size, 1234u);
    EXPECT_FALSE(item.isDir);
}

TEST(InMemoryDirectory, MkdirMakesAnEmptyDirectoryEnumerable)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("Debug"));

    EXPECT_TRUE(fs.exist("Debug"));
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "Debug/" }));

    auto created = fs.dir("Debug");
    ASSERT_TRUE(created->open()) << "an mkdir'd directory must be openable";
    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(created->readNext(item)) << "created, but empty";
    EXPECT_TRUE(created->close());
}

TEST(InMemoryDirectory, MkdirCreatesParentsAndSucceedsWhenAlreadyPresent)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("a/b/c"));
    EXPECT_TRUE(fs.exist("a"));
    EXPECT_TRUE(fs.exist("a/b"));
    EXPECT_TRUE(fs.exist("a/b/c"));

    EXPECT_TRUE(fs.mkdir("a/b/c"));

    EXPECT_FALSE(fs.mkdir(nullptr));
}

TEST(InMemoryDirectory, OpeningAMissingDirectoryFails)
{
    InMemoryFileSystem fs;
    auto dir = fs.dir("nope");
    ASSERT_TRUE(dir != nullptr);
    EXPECT_FALSE(dir->open());
    EXPECT_FALSE(dir->exist());
}

TEST(InMemoryDirectory, ReadNextOnAClosedDirectoryFails)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    SDK::Interface::IFileSystem::ObjectInfo item{};
    EXPECT_FALSE(dir->readNext(item)) << "not open yet";

    ASSERT_TRUE(dir->open());
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->close());
    EXPECT_FALSE(dir->readNext(item)) << "closed again";
}

TEST(InMemoryDirectory, ResetRewindsWithoutConsumingAnEntry)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");
    fs.seedFile("d/b.txt", "y");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "a.txt");
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_STREQ(item.name, "b.txt");
    EXPECT_FALSE(dir->readNext(item)) << "exhausted";

    SDK::Interface::IFileSystem::ObjectInfo untouched{};
    std::strncpy(untouched.name, "SENTINEL", sizeof(untouched.name) - 1);
    EXPECT_TRUE(dir->readNext(untouched, /*reset=*/true));
    EXPECT_STREQ(untouched.name, "SENTINEL") << "a reset must not write into item";

    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt") << "and the cursor is back at the start";
}

TEST(InMemoryDirectory, SnapshotIsTakenAtOpenNotLive)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    ASSERT_TRUE(dir->open());

    fs.seedFile("d/b.txt", "y");

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "a.txt");
    EXPECT_FALSE(dir->readNext(item)) << "mid-scan additions do not appear";

    ASSERT_TRUE(dir->readNext(item, /*reset=*/true));
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->readNext(item));
    EXPECT_STREQ(item.name, "b.txt");
}

TEST(InMemoryFileSystem, RemoveReportsWhetherAnythingWasRemoved)
{
    InMemoryFileSystem fs;
    fs.seedFile("a.txt", "x");

    EXPECT_TRUE(fs.remove("a.txt"));
    EXPECT_FALSE(fs.remove("a.txt")) << "already gone";
    EXPECT_FALSE(fs.remove("never-existed.txt"));
    EXPECT_FALSE(fs.remove(nullptr));
}

TEST(InMemoryFileSystem, RenameAndCopyRefuseAnAlreadyRemovedSource)
{
    InMemoryFileSystem fs;
    fs.seedFile("gone.txt", "x");
    fs.seedFile("live.txt", "IMPORTANT");
    ASSERT_TRUE(fs.remove("gone.txt"));

    EXPECT_FALSE(fs.rename("gone.txt", "live.txt")) << "a tombstone is not a source";
    EXPECT_EQ(fs.readFile("live.txt"), "IMPORTANT") << "and must not destroy the destination";
    EXPECT_TRUE(fs.exist("live.txt"));

    EXPECT_FALSE(fs.rename("gone.txt", "fresh.txt")) << "nor onto a free name";
    EXPECT_FALSE(fs.exist("fresh.txt"));

    EXPECT_FALSE(fs.copy("gone.txt", "c.txt"));
    EXPECT_FALSE(fs.exist("c.txt")) << "no tombstone propagated to the destination";

    EXPECT_FALSE(fs.rename("never.txt", "x.txt"));
    EXPECT_FALSE(fs.copy("never.txt", "x.txt"));
}

TEST(InMemoryFileSystem, RenameAndCopyRefuseADestinationThatCannotHoldAFile)
{
    InMemoryFileSystem fs;
    fs.seedFile("src.txt", "SRC");
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("blocker.txt", "B");

    EXPECT_FALSE(fs.rename("src.txt", "")) << "the root is not a file destination";
    EXPECT_FALSE(fs.rename("src.txt", "/"));
    EXPECT_FALSE(fs.rename("src.txt", "d")) << "an existing directory";
    EXPECT_FALSE(fs.rename("src.txt", "blocker.txt/under")) << "under a file";
    EXPECT_FALSE(fs.copy("src.txt", ""));
    EXPECT_FALSE(fs.copy("src.txt", "d"));
    EXPECT_EQ(fs.readFile("src.txt"), "SRC") << "every refusal left the source alone";
    EXPECT_EQ(listDir(fs, "/"),
              (std::vector<std::string>{ "src.txt", "d/", "blocker.txt" }));

    EXPECT_FALSE(fs.rename("src.txt", "blocker.txt")) << "an existing file";
    EXPECT_FALSE(fs.rename("src.txt", "/blocker.txt/")) << "the same, spelled differently";
    EXPECT_EQ(fs.readFile("blocker.txt"), "B");
    EXPECT_EQ(fs.readFile("src.txt"), "SRC");
}

TEST(InMemoryFileSystem, RenamingOrCopyingAFileOntoItselfKeepsIt)
{
    for (const char* destination : { "a.txt", "/a.txt", "a.txt/", "/a.txt/" }) {
        InMemoryFileSystem fs;
        fs.seedFile("a.txt", "PAYLOAD");

        EXPECT_TRUE(fs.rename("a.txt", destination)) << destination;
        EXPECT_TRUE(fs.exist("a.txt")) << destination << " must not remove the file";
        EXPECT_EQ(fs.readFile("a.txt"), "PAYLOAD") << destination;
        EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a.txt" })) << destination;
    }

    InMemoryFileSystem nested;
    nested.seedFile("d/a.txt", "PAYLOAD");
    EXPECT_TRUE(nested.rename("d/a.txt", "d//a.txt"));
    EXPECT_EQ(nested.readFile("d/a.txt"), "PAYLOAD");

    InMemoryFileSystem copied;
    copied.seedFile("a.txt", "PAYLOAD");
    EXPECT_TRUE(copied.copy("a.txt", "/a.txt"));
    EXPECT_EQ(copied.readFile("a.txt"), "PAYLOAD");
}

TEST(InMemoryDirectory, RepeatedSeparatorsCollapse)
{
    InMemoryFileSystem fs;
    fs.seedFile("a//b.txt", "x");

    EXPECT_TRUE(fs.exist("a//b.txt"));
    EXPECT_TRUE(fs.exist("a/b.txt")) << "one object, however it is spelled";
    EXPECT_EQ(fs.readFile("a/b.txt"), "x");
    EXPECT_EQ(listDir(fs, "a"), (std::vector<std::string>{ "b.txt" }));
    EXPECT_EQ(listDir(fs, "a//"), (std::vector<std::string>{ "b.txt" }));

    auto dir = fs.dir("a");
    ASSERT_TRUE(dir->open());
    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(dir->readNext(item));
    ASSERT_TRUE(dir->close());
    ASSERT_GT(std::strlen(item.name), 0u) << "a listing never yields an empty name";
    const std::string joined = std::string("a/") + item.name;
    EXPECT_TRUE(fs.exist(joined.c_str())) << joined;
}

TEST(InMemoryDirectory, RemovedFilesDisappearFromListings)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");
    fs.seedFile("d/b.txt", "y");

    ASSERT_TRUE(fs.remove("d/a.txt"));
    EXPECT_EQ(listDir(fs, "d"), (std::vector<std::string>{ "b.txt" }));
}

TEST(InMemoryDirectory, RemoveRefusesANonEmptyDirectory)
{
    InMemoryFileSystem fs;
    ASSERT_TRUE(fs.mkdir("d"));
    fs.seedFile("d/a.txt", "x");

    EXPECT_FALSE(fs.remove("d")) << "a populated directory";

    ASSERT_TRUE(fs.remove("d/a.txt"));
    ASSERT_TRUE(fs.mkdir("d/sub"));
    EXPECT_FALSE(fs.remove("d")) << "a directory holding only a subdirectory";

    EXPECT_FALSE(fs.dir("d")->remove()) << "through IDirectory too";

    ASSERT_TRUE(fs.remove("d/sub"));
    EXPECT_TRUE(fs.remove("d")) << "empty now";
    EXPECT_FALSE(fs.exist("d"));
}

TEST(InMemoryDirectory, AnExplicitSubdirectoryKeepsItsImpliedParent)
{
    InMemoryFileSystem fs;
    fs.seedFile("a/x.txt", "x");
    ASSERT_TRUE(fs.dir("a/b")->create());
    ASSERT_TRUE(fs.remove("a/x.txt"));

    EXPECT_TRUE(fs.exist("a"));
    EXPECT_EQ(listDir(fs, "/"), (std::vector<std::string>{ "a/" }));
    EXPECT_FALSE(fs.seedFile("a", "file")) << "a file over a directory";
}

TEST(InMemoryDirectory, AnImpliedDirectoryVanishesWithItsLastChild)
{
    InMemoryFileSystem fs;
    fs.seedFile("implied/a.txt", "x");
    ASSERT_TRUE(fs.exist("implied"));

    ASSERT_TRUE(fs.remove("implied/a.txt"));
    EXPECT_FALSE(fs.exist("implied"))
        << "implied directories are only as durable as their contents";

    InMemoryFileSystem explicitFs;
    ASSERT_TRUE(explicitFs.mkdir("kept"));
    explicitFs.seedFile("kept/a.txt", "x");
    ASSERT_TRUE(explicitFs.remove("kept/a.txt"));
    EXPECT_TRUE(explicitFs.exist("kept")) << "an mkdir'd directory survives";
}

TEST(InMemoryDirectory, ObjectInfoReportsTheLeafNameAndWhetherItIsADirectory)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "hello");

    SDK::Interface::IFileSystem::ObjectInfo item{};
    ASSERT_TRUE(fs.objectInfo("d/a.txt", item));
    EXPECT_FALSE(item.isDir);
    EXPECT_EQ(item.size, 5u);
    EXPECT_STREQ(item.name, "a.txt") << "the leaf name, not the path";

    ASSERT_TRUE(fs.objectInfo("d", item));
    EXPECT_TRUE(item.isDir);
    EXPECT_STREQ(item.name, "d");

    EXPECT_FALSE(fs.objectInfo("d/missing.txt", item));
}

TEST(InMemoryDirectory, DirectoryRenameFailsRatherThanLying)
{
    InMemoryFileSystem fs;
    fs.seedFile("d/a.txt", "x");

    auto dir = fs.dir("d");
    EXPECT_FALSE(dir->rename("e")) << "not modelled; must not silently orphan contents";
    EXPECT_TRUE(fs.exist("d/a.txt"));
}
