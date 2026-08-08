/**
 ******************************************************************************
 * @file    VariantConfig_test.cpp
 * @brief   Tests for the app-side variant reader's sandbox-root scan.
 ******************************************************************************
 *
 * Config's constructor picks which .uapp in the sandbox root defines this
 * app's identity, following the mixed-directory determinism rules: any real
 * (non-alias) .uapp means the app runs as its classic self; otherwise the
 * first alias-flagged .uapp is the variant; and an unreadable candidate
 * counts as real. Those rules decide what a user sees their activity called,
 * so they are worth pinning.
 *
 * Every case here goes through a real directory scan, which InMemoryFileSystem
 * only supports now that it enumerates seeded files. Because most of these
 * expect the app to run classic -- the same answer a scan that saw nothing
 * would give -- they assert the scan is live before trusting its verdict.
 *
 ******************************************************************************
 */

#include "SDK/Variant/VariantConfig.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "KernelTestDoubles.hpp"

using SDK::TestSupport::KernelFixture;

namespace {

// On-disk .uapp layout, mirroring the private constants in VariantConfig.cpp.
// Duplicated deliberately: if the reader's offsets drift from the format,
// these tests should fail rather than drift along with it.
constexpr uint32_t kFlagVariantAlias = 0x40;
constexpr size_t   kFlagsOffset      = 20;
constexpr size_t   kPayloadOffset    = 48 + 3600 + 900;
constexpr size_t   kConfigSizeOffset = kPayloadOffset + 17;
constexpr size_t   kConfigOffset     = kPayloadOffset + 32;
constexpr uint32_t kPayloadVersion   = 1;

void putU32(std::string& blob, size_t offset, uint32_t value)
{
    if (blob.size() < offset + sizeof(value)) {
        blob.resize(offset + sizeof(value), '\0');
    }
    for (size_t i = 0; i < sizeof(value); ++i) {
        blob[offset + i] = static_cast<char>((value >> (8 * i)) & 0xFFu);
    }
}

/// A real (non-alias) app binary: flags without the alias bit, no payload.
std::string realUapp()
{
    std::string blob(kPayloadOffset, '\0');
    putU32(blob, kFlagsOffset, 0);
    return blob;
}

/// An alias .uapp carrying @p json as its embedded variant config.
std::string aliasUapp(const std::string& json, uint32_t payloadVersion = kPayloadVersion)
{
    std::string blob(kConfigOffset + json.size(), '\0');
    putU32(blob, kFlagsOffset, kFlagVariantAlias);
    putU32(blob, kPayloadOffset, payloadVersion);
    putU32(blob, kConfigSizeOffset, static_cast<uint32_t>(json.size()));
    std::memcpy(&blob[kConfigOffset], json.data(), json.size());
    return blob;
}

const char* kWalkJson =
    R"({"schema":1,"name":"Walk","fit":{"sport":11,"subSport":0},)"
    R"("features":{"showCadence":true,"autoLapMetres":1000}})";

/// Assert that the sandbox root really enumerates @p expected.
///
/// Every "runs classic" expectation below is also exactly what a scan that
/// saw *nothing* produces, so on its own such a test cannot tell a correct
/// decision from a dead enumerator or a path the fake failed to resolve.
/// Calling this first makes each one prove its fixture is live.
void expectSandboxLists(SDK::TestSupport::InMemoryFileSystem& fs,
                        const std::vector<std::string>& expected)
{
    std::vector<std::string> seen;
    auto dir = fs.dir("/");
    ASSERT_TRUE(dir && dir->open()) << "the sandbox root must be scannable";
    SDK::Interface::IFileSystem::ObjectInfo item {};
    while (dir->readNext(item)) {
        seen.push_back(item.name);
    }
    dir->close();
    EXPECT_EQ(seen, expected) << "the scan under test did not see what was seeded";
}

} // namespace

// --------------------------------------------------------------------------
// Identity selection
// --------------------------------------------------------------------------

TEST(VariantConfig, EmptySandboxIsTheClassicApp)
{
    KernelFixture fixture;
    expectSandboxLists(fixture.fileSystem, {});

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, ASingleRealAppIsTheClassicApp)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Running.uapp", realUapp());
    expectSandboxLists(fixture.fileSystem, { "Running.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, ASingleAliasBecomesTheVariant)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson));

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Walk");
    EXPECT_EQ(config.fitSport(1), 11);
    EXPECT_EQ(config.fitSubSport(7), 0);
    EXPECT_TRUE(config.featureBool("showCadence", false));
    EXPECT_EQ(config.featureU32("autoLapMetres", 0u), 1000u);
}

// The mixed-directory rule: a real binary alongside an alias means the app is
// its classic self, whichever order the scan happens to encounter them in.
// Both orderings are exercised because the scan short-circuits on the first
// real app it sees, so only one of them takes the early-break path.
TEST(VariantConfig, ARealAppAlongsideAnAliasWinsWhenItSortsFirst)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/AAA-real.uapp", realUapp());
    fixture.fileSystem.seedFile("/ZZZ-alias.uapp", aliasUapp(kWalkJson));
    expectSandboxLists(fixture.fileSystem, { "AAA-real.uapp", "ZZZ-alias.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, ARealAppAlongsideAnAliasWinsWhenItSortsLast)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/AAA-alias.uapp", aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile("/ZZZ-real.uapp", realUapp());
    expectSandboxLists(fixture.fileSystem, { "AAA-alias.uapp", "ZZZ-real.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant())
        << "a real app discovered after an alias must still win";
    EXPECT_STREQ(config.name("Run"), "Run");
}

// Degenerate case -- a sandbox root is meant to hold exactly one .uapp -- but
// the reader still has to be deterministic about it rather than picking at
// random. Note this pins "the first alias *encountered*": the fake enumerates
// alphabetically so the test is reproducible, while the device enumerates in
// directory-entry order. Do not read this as an alphabetical guarantee.
TEST(VariantConfig, TheFirstAliasWinsWhenThereAreSeveral)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile(
        "/AAA.uapp", aliasUapp(R"({"schema":1,"name":"First","fit":{"sport":11}})"));
    fixture.fileSystem.seedFile(
        "/ZZZ.uapp", aliasUapp(R"({"schema":1,"name":"Second","fit":{"sport":12}})"));

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "First");
}

TEST(VariantConfig, AnUnreadableCandidateCountsAsARealApp)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/AAA.uapp", aliasUapp(kWalkJson));
    // Shorter than kFlagsOffset + sizeof(u32), so the flags read fails
    // outright rather than returning whatever bytes happen to be there.
    fixture.fileSystem.seedFile("/truncated.uapp", "short");
    expectSandboxLists(fixture.fileSystem, { "AAA.uapp", "truncated.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant())
        << "a .uapp whose flags cannot be read must not be assumed to be an alias";
}

// A name too long to form a candidate path must be declined, not silently
// truncated -- because a truncated path can name a *different, existing*
// file, and adopting that file's identity is the failure this guards.
//
// The two seeds below are sized so that clipping the long one's path yields
// exactly the short one's path:
//   long  "/xxx...x.uapp"  (256 chars -- one over the buffer)
//   clip  "/xxx...x.uap"   (255 chars -- what snprintf would leave behind)
// The decoy ends ".uap", so it is never a candidate in its own right; the
// only way to reach it is through the truncation being fixed here.
TEST(VariantConfig, ANameTooLongToFormAPathIsNotSilentlyTruncatedIntoAnotherApp)
{
    KernelFixture fixture;

    // ObjectInfo::name carries 255 chars plus a terminator, so a name of
    // exactly 255 survives the listing with its extension intact and
    // overflows only once the scan prepends its '/'.
    const std::string longName = std::string(250, 'x') + ".uapp";
    ASSERT_EQ(longName.size(), 255u);
    const std::string longPath = "/" + longName;
    const std::string clippedPath = longPath.substr(0, 255);
    ASSERT_EQ(clippedPath, "/" + std::string(250, 'x') + ".uap");

    const std::string wrongAlias =
        aliasUapp(R"({"schema":1,"name":"WRONG","fit":{"sport":99}})");

    // Positive control. "Runs classic" is also what a scan that saw nothing
    // produces, so the decline below only means something once the same
    // payload is known to be adoptable when its name is one character
    // shorter -- isolating length as the sole reason for the difference.
    // (The decoy itself ends ".uap" and so is never a candidate in its own
    // right; truncation is the only way anything reaches it.)
    {
        const std::string longestThatFits = "/" + std::string(249, 'x') + ".uapp";
        ASSERT_EQ(longestThatFits.size(), 255u);
        KernelFixture control;
        control.fileSystem.seedFile(longestThatFits, wrongAlias);
        SDK::Variant::Config adopted(control.kernel);
        ASSERT_TRUE(adopted.isVariant()) << "a name that fits must still be adopted";
        ASSERT_STREQ(adopted.name("Run"), "WRONG");
    }

    fixture.fileSystem.seedFile(longPath, aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile(clippedPath, wrongAlias);
    // Both are really there and really enumerated: the clipped one sorts
    // first, being a prefix of the other.
    expectSandboxLists(fixture.fileSystem,
                       { clippedPath.substr(1), longPath.substr(1) });

    SDK::Variant::Config config(fixture.kernel);

    EXPECT_FALSE(config.isVariant())
        << "an unformable path must be declined, not clipped into a neighbour";
    EXPECT_STRNE(config.name("Run"), "WRONG")
        << "the app must never adopt the identity of a file it reached by truncation";
    EXPECT_STREQ(config.name("Run"), "Run");
}

// A declined name must not inherit the path of the entry scanned before it.
// If a candidate buffer outlived the iteration that filled it, an entry whose
// own path cannot be formed would be read against whatever the last entry
// left behind, and the app would adopt that one -- the same wrong-identity
// failure a truncated path causes, by another route.
//
// "AAA.uapp" sorts first and is a perfectly good alias, so it is exactly what
// a stale buffer would be holding. The overlong entry that follows must still
// force the classic path.
TEST(VariantConfig, ADeclinedNameDoesNotInheritThePreviousCandidatesPath)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile(
        "/AAA.uapp", aliasUapp(R"({"schema":1,"name":"STALE","fit":{"sport":11}})"));

    // 255 chars: survives the listing intact, overflows only once '/' is
    // prepended -- so its own path is never formed.
    const std::string longName = std::string(250, 'z') + ".uapp";
    ASSERT_EQ(longName.size(), 255u);
    fixture.fileSystem.seedFile("/" + longName, aliasUapp(kWalkJson));
    expectSandboxLists(fixture.fileSystem, { "AAA.uapp", longName });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant())
        << "an unformable candidate counts as a real app, whatever preceded it";
    EXPECT_STRNE(config.name("Run"), "STALE")
        << "the scan must not re-read the previous entry's path";
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, NonUappFilesAreIgnored)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile("/notes.txt", "irrelevant");
    fixture.fileSystem.seedFile("/uapp", "no extension, just the word");
    fixture.fileSystem.seedFile("/.uapp", "extension but no name");

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Walk");
}

// The scan skips directory entries. Reaching this guard at all needs a fake
// that really enumerates, since the loop body is where it lives.
TEST(VariantConfig, SubdirectoriesAreSkipped)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson));
    // A directory that ends in ".uapp" would pass the extension test, so the
    // isDir guard is the only thing stopping it being opened as a candidate.
    fixture.fileSystem.seedFile("/Stale.uapp/leftover.bin", "junk");

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Walk")
        << "a directory named like an app must not be treated as one";
}

// --------------------------------------------------------------------------
// Never brick a launch: a malformed alias falls back to classic defaults.
// --------------------------------------------------------------------------

TEST(VariantConfig, AnUnknownPayloadVersionFallsBackToClassic)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson, kPayloadVersion + 1));
    expectSandboxLists(fixture.fileSystem, { "Walk.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant()) << "a shipped binary must not guess at a newer layout";
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, AnUnknownSchemaFallsBackToClassic)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(R"({"schema":99,"name":"Future"})"));
    expectSandboxLists(fixture.fileSystem, { "Walk.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, MalformedJsonFallsBackToClassic)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(R"({"schema":1,"name":)"));
    expectSandboxLists(fixture.fileSystem, { "Walk.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, AZeroLengthConfigFallsBackToClassic)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(""));
    expectSandboxLists(fixture.fileSystem, { "Walk.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
}

TEST(VariantConfig, MissingOptionalKeysLeaveCallerDefaultsInPlace)
{
    KernelFixture fixture;
    // A valid variant that names itself but says nothing about FIT identity
    // or features.
    fixture.fileSystem.seedFile("/Hike.uapp", aliasUapp(R"({"schema":1,"name":"Hike"})"));

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Hike");
    EXPECT_EQ(config.fitSport(1), 1) << "unspecified sport keeps the family default";
    EXPECT_EQ(config.fitSubSport(7), 7);
    EXPECT_TRUE(config.featureBool("missing", true));
    EXPECT_EQ(config.featureU32("missing", 42u), 42u);
}
