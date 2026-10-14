#include "SDK/Variant/VariantConfig.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "KernelTestDoubles.hpp"

using SDK::TestSupport::KernelFixture;

namespace {

// Restated rather than shared with VariantConfig.cpp, so a drifted offset there fails here.
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

std::string realUapp()
{
    std::string blob(kPayloadOffset, '\0');
    putU32(blob, kFlagsOffset, 0);
    return blob;
}

std::string aliasUapp(const std::string& json, uint32_t payloadVersion = kPayloadVersion)
{
    std::string blob(kConfigOffset + json.size(), '\0');
    putU32(blob, kFlagsOffset, kFlagVariantAlias);
    putU32(blob, kPayloadOffset, payloadVersion);
    putU32(blob, kConfigSizeOffset, static_cast<uint32_t>(json.size()));
    std::memcpy(&blob[kConfigOffset], json.data(), json.size());
    return blob;
}

constexpr size_t kLongestListedName = sizeof(SDK::Interface::IFileSystem::ObjectInfo::name) - 1;
constexpr size_t kLongestPath = SDK::Interface::IFileSystem::skMaxPathLen - 1;

std::string uappNameOfLength(size_t length, char fill)
{
    const std::string extension = ".uapp";
    return std::string(length - extension.size(), fill) + extension;
}

const char* kWalkJson =
    R"({"schema":1,"name":"Walk","fit":{"sport":11,"subSport":0},)"
    R"("features":{"showCadence":true,"autoLapMetres":1000}})";

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

TEST(VariantConfig, ARealAppAlongsideAnAliasWinsWhenListedFirst)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/AAA-real.uapp", realUapp());
    fixture.fileSystem.seedFile("/ZZZ-alias.uapp", aliasUapp(kWalkJson));
    expectSandboxLists(fixture.fileSystem, { "AAA-real.uapp", "ZZZ-alias.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, ARealAppAlongsideAnAliasWinsWhenListedLast)
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

TEST(VariantConfig, TheFirstAliasEnumeratedWinsWhenThereAreSeveral)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile(
        "/ZZZ.uapp", aliasUapp(R"({"schema":1,"name":"First","fit":{"sport":11}})"));
    fixture.fileSystem.seedFile(
        "/AAA.uapp", aliasUapp(R"({"schema":1,"name":"Second","fit":{"sport":12}})"));

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "First");
}

TEST(VariantConfig, AnUnreadableCandidateCountsAsARealApp)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/AAA.uapp", aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile("/truncated.uapp", "short");
    expectSandboxLists(fixture.fileSystem, { "AAA.uapp", "truncated.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant())
        << "a .uapp whose flags cannot be read must not be assumed to be an alias";
}

TEST(VariantConfig, ANameTooLongToFormAPathIsNotSilentlyTruncatedIntoAnotherApp)
{
    KernelFixture fixture;

    const std::string longName = uappNameOfLength(kLongestListedName, 'x');
    ASSERT_EQ(longName.size(), 255u);
    const std::string longPath = "/" + longName;
    const std::string clippedPath = longPath.substr(0, kLongestPath);
    ASSERT_EQ(clippedPath, "/" + std::string(250, 'x') + ".uap");

    const std::string wrongAlias =
        aliasUapp(R"({"schema":1,"name":"WRONG","fit":{"sport":99}})");

    {
        const std::string longestThatFits = "/" + uappNameOfLength(kLongestPath - 1, 'x');
        ASSERT_EQ(longestThatFits.size(), 255u);
        KernelFixture control;
        control.fileSystem.seedFile(longestThatFits, wrongAlias);
        SDK::Variant::Config adopted(control.kernel);
        ASSERT_TRUE(adopted.isVariant()) << "a name that fits must still be adopted";
        ASSERT_STREQ(adopted.name("Run"), "WRONG");
    }

    fixture.fileSystem.seedFile(longPath, aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile(clippedPath, wrongAlias);
    expectSandboxLists(fixture.fileSystem,
                       { longPath.substr(1), clippedPath.substr(1) });

    SDK::Variant::Config config(fixture.kernel);

    EXPECT_FALSE(config.isVariant())
        << "an unformable path must be skipped, not clipped into a neighbour";
    EXPECT_STRNE(config.name("Run"), "WRONG")
        << "the app must never adopt the identity of a file it reached by truncation";
    EXPECT_STREQ(config.name("Run"), "Run");
}

TEST(VariantConfig, ANameTooLongToFormAPathIsSkippedAndTheScanCarriesOn)
{
    const std::string longName = uappNameOfLength(kLongestListedName, 'z');
    ASSERT_EQ(longName.size(), 255u);

    {
        KernelFixture fixture;
        fixture.fileSystem.seedFile("/AAA.uapp", aliasUapp(kWalkJson));
        fixture.fileSystem.seedFile("/" + longName, realUapp());
        expectSandboxLists(fixture.fileSystem, { "AAA.uapp", longName });

        SDK::Variant::Config config(fixture.kernel);
        EXPECT_TRUE(config.isVariant())
            << "a skipped name is not a real app, whatever it holds";
        EXPECT_STREQ(config.name("Run"), "Walk");
    }

    {
        KernelFixture fixture;
        fixture.fileSystem.seedFile("/AAA.uapp", aliasUapp(kWalkJson));
        fixture.fileSystem.seedFile("/" + longName, aliasUapp(kWalkJson));
        fixture.fileSystem.seedFile("/~real.uapp", realUapp());
        expectSandboxLists(fixture.fileSystem, { "AAA.uapp", longName, "~real.uapp" });

        SDK::Variant::Config config(fixture.kernel);
        EXPECT_FALSE(config.isVariant()) << "the scan must not stop at a skipped name";
        EXPECT_STREQ(config.name("Run"), "Run");
    }
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

TEST(VariantConfig, ADirectoryNamedLikeAUappIsSkipped)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson));
    fixture.fileSystem.seedFile("/Stale.uapp/leftover.bin", "junk");

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Walk")
        << "a directory named like an app must not be treated as one";
}

TEST(VariantConfig, AnUnknownPayloadVersionFallsBackToClassic)
{
    KernelFixture fixture;
    fixture.fileSystem.seedFile("/Walk.uapp", aliasUapp(kWalkJson, kPayloadVersion + 1));
    expectSandboxLists(fixture.fileSystem, { "Walk.uapp" });

    SDK::Variant::Config config(fixture.kernel);
    EXPECT_FALSE(config.isVariant()) << "a newer payload layout is not guessed at";
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
    fixture.fileSystem.seedFile("/Hike.uapp", aliasUapp(R"({"schema":1,"name":"Hike"})"));

    SDK::Variant::Config config(fixture.kernel);
    ASSERT_TRUE(config.isVariant());
    EXPECT_STREQ(config.name("Run"), "Hike");
    EXPECT_EQ(config.fitSport(1), 1) << "unspecified sport keeps the family default";
    EXPECT_EQ(config.fitSubSport(7), 7);
    EXPECT_TRUE(config.featureBool("missing", true));
    EXPECT_EQ(config.featureU32("missing", 42u), 42u);
}
