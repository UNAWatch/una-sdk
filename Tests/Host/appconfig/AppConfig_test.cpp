/**
 * Tests for SDK::AppConfig -- the app-side reader/writer for the configuration
 * values the companion app writes next to a .uapp.
 *
 * The contract under test is Docs/app-config-fields.md: a bad, missing or
 * future-schema file must degrade to the declared defaults rather than break a
 * launch; a single bad key must cost only its own field; numeric values are
 * clamped to the declared bounds; strings are truncated on character
 * boundaries; a save must be crash-safe and must not disturb values it did not
 * change.
 *
 * Multi-byte UTF-8 is spelled out as explicit bytes so the cases do not depend
 * on this file's encoding. "\xD0\x94" is the Cyrillic letter De (2 bytes) and
 * "\xE2\x82\xAC" is the euro sign (3 bytes).
 */
#include <gtest/gtest.h>

#include <string>

#include "KernelTestDoubles.hpp"
#include "SDK/AppConfig/AppConfig.hpp"

namespace {

using SDK::AppConfig;

constexpr const char *kFileName = "app_config.json";
constexpr const char *kPath = "/app_config.json";
constexpr const char *kTmpPath = "/app_config.json.tmp";

// One field of each type, mirroring the Waypoint tutorial's declaration.
constexpr AppConfig::Field kFields[] = {
    AppConfig::stringField("waypointName", "Waypoint", 1, 16),
    AppConfig::floatField("targetLatitude", 51.5072f, -90.0f, 90.0f),
    AppConfig::intField("arrivalRadiusM", 25, 5, 500),
    AppConfig::boolField("vibrateOnArrival", true),
};

class AppConfigTest : public ::testing::Test {
protected:
    SDK::TestSupport::KernelFixture fx;

    void seed(const std::string &contents)
    {
        fx.fileSystem.seedFile(kPath, contents);
    }

    AppConfig open() { return AppConfig(fx.kernel, kFileName, kFields); }

    std::string contents(const char *path = kPath) const
    {
        return fx.fileSystem.readFile(path);
    }

    std::string name(const AppConfig &cfg) const
    {
        char buf[64] {};
        cfg.getString("waypointName", buf, sizeof(buf));
        return std::string(buf);
    }
};

// --- Reading ---------------------------------------------------------------

TEST_F(AppConfigTest, MissingFileYieldsDeclaredDefaults)
{
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_FALSE(cfg.has("waypointName"));
    EXPECT_EQ(name(cfg), "Waypoint");
    EXPECT_FLOAT_EQ(cfg.getFloat("targetLatitude"), 51.5072f);
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_TRUE(cfg.getBool("vibrateOnArrival"));
}

TEST_F(AppConfigTest, ReadsEveryDeclaredType)
{
    seed(R"({"schema":1,"values":{"waypointName":"Trailhead",)"
         R"("targetLatitude":48.8584,"arrivalRadiusM":40,)"
         R"("vibrateOnArrival":false}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.isLoaded());
    EXPECT_EQ(name(cfg), "Trailhead");
    EXPECT_FLOAT_EQ(cfg.getFloat("targetLatitude"), 48.8584f);
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 40);
    EXPECT_FALSE(cfg.getBool("vibrateOnArrival"));
    EXPECT_TRUE(cfg.has("waypointName"));
}

TEST_F(AppConfigTest, AbsentKeyFallsBackToItsOwnDefault)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 40);
    EXPECT_TRUE(cfg.has("arrivalRadiusM"));

    EXPECT_EQ(name(cfg), "Waypoint");
    EXPECT_FALSE(cfg.has("waypointName"));
}

TEST_F(AppConfigTest, EmptyValuesObjectIsValidAndMeansAllDefaults)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    EXPECT_TRUE(cfg.isLoaded());
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, WrongJsonTypeCostsOnlyThatField)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":"forty","waypointName":"Ridge"}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);      // default, not atoi("forty")
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
    EXPECT_EQ(name(cfg), "Ridge");                    // unaffected
}

TEST_F(AppConfigTest, NullMeansNotSet)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":null}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
}

TEST_F(AppConfigTest, NumberIsNotAcceptedForABoolField)
{
    seed(R"({"schema":1,"values":{"vibrateOnArrival":0}})");
    AppConfig cfg = open();

    // 0 would coerce to false, which is a value the user never chose.
    EXPECT_TRUE(cfg.getBool("vibrateOnArrival"));
    EXPECT_FALSE(cfg.has("vibrateOnArrival"));
}

TEST_F(AppConfigTest, FractionalNumberIsNotAnInt)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":25.5}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
}

TEST_F(AppConfigTest, IntOutOfRangeIsClampedNotDiscarded)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":9000}})");
    EXPECT_EQ(open().getInt("arrivalRadiusM"), 500);

    fx.fileSystem.files.clear();
    seed(R"({"schema":1,"values":{"arrivalRadiusM":-3}})");
    EXPECT_EQ(open().getInt("arrivalRadiusM"), 5);
}

TEST_F(AppConfigTest, IntOverflowIsRejected)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":99999999999999}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
}

TEST_F(AppConfigTest, FloatOutOfRangeIsClamped)
{
    seed(R"({"schema":1,"values":{"targetLatitude":120.0}})");
    EXPECT_FLOAT_EQ(open().getFloat("targetLatitude"), 90.0f);
}

TEST_F(AppConfigTest, FloatAcceptsExponentNotation)
{
    seed(R"({"schema":1,"values":{"targetLatitude":5.15072e1}})");
    EXPECT_FLOAT_EQ(open().getFloat("targetLatitude"), 51.5072f);
}

TEST_F(AppConfigTest, NonFiniteFloatIsRejected)
{
    seed(R"({"schema":1,"values":{"targetLatitude":1e999}})");
    AppConfig cfg = open();

    EXPECT_FLOAT_EQ(cfg.getFloat("targetLatitude"), 51.5072f);
    EXPECT_FALSE(cfg.has("targetLatitude"));
}

TEST_F(AppConfigTest, FloatFieldAcceptsAnIntegerLiteral)
{
    seed(R"({"schema":1,"values":{"targetLatitude":51}})");
    EXPECT_FLOAT_EQ(open().getFloat("targetLatitude"), 51.0f);
}

TEST_F(AppConfigTest, LongStringIsTruncatedToMaxLength)
{
    seed(R"({"schema":1,"values":{"waypointName":"An extremely long waypoint name"}})");
    AppConfig cfg = open();

    // maxLength is 16 bytes.
    EXPECT_EQ(name(cfg), "An extremely lon");
}

TEST_F(AppConfigTest, TruncationDoesNotSplitAMultiByteCharacter)
{
    // 15 ASCII bytes then a 2-byte character: byte 16 would be half of it.
    seed(std::string(R"({"schema":1,"values":{"waypointName":")") +
         "123456789012345" "\xD0\x94" R"("}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), "123456789012345");
}

TEST_F(AppConfigTest, TruncationKeepsAMultiByteCharacterThatFits)
{
    // 13 ASCII bytes + a 3-byte character == 16 bytes exactly.
    seed(std::string(R"({"schema":1,"values":{"waypointName":")") +
         "1234567890123" "\xE2\x82\xAC" R"("}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), std::string("1234567890123") + "\xE2\x82\xAC");
}

TEST_F(AppConfigTest, DecodesJsonEscapes)
{
    seed(R"({"schema":1,"values":{"waypointName":"A\"B\\C\tD"}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), "A\"B\\C\tD");
}

TEST_F(AppConfigTest, DecodesUnicodeEscapes)
{
    // A raw string literal keeps the backslash, so the file really holds the
    // six characters Д rather than the pre-encoded UTF-8 bytes.
    seed(R"({"schema":1,"values":{"waypointName":"De Д"}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), std::string("De ") + "\xD0\x94");
}

TEST_F(AppConfigTest, DecodesSurrogatePairs)
{
    // 😀 is U+1F600 written as a surrogate pair, which is the only way
    // JSON can express it: four UTF-8 bytes out.
    seed(R"({"schema":1,"values":{"waypointName":"😀"}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), "\xF0\x9F\x98\x80");
}

TEST_F(AppConfigTest, EscapedValueIsTruncatedByDecodedLength)
{
    // Eight Д escapes are 48 bytes of JSON but 16 bytes decoded, which is
    // exactly maxLength: the limit applies to the value, not to its encoding.
    seed(R"({"schema":1,"values":{"waypointName":)"
         R"("ДДДДДДДДД"}})");
    AppConfig cfg = open();

    // The ninth would exceed 16 bytes, so it is dropped whole.
    EXPECT_EQ(name(cfg).size(), 16u);
}

TEST_F(AppConfigTest, CallerBufferSmallerThanTheValueStillTruncatesSafely)
{
    seed(std::string(R"({"schema":1,"values":{"waypointName":")") +
         "AB" "\xD0\x94" R"("}})");
    AppConfig cfg = open();

    char small[4] {};   // room for 3 bytes: "AB" then half of the 2-byte char
    size_t written = cfg.getString("waypointName", small, sizeof(small));

    EXPECT_EQ(written, 2u);
    EXPECT_STREQ(small, "AB");
}

TEST_F(AppConfigTest, UnsupportedSchemaMeansAllDefaults)
{
    seed(R"({"schema":2,"values":{"arrivalRadiusM":40}})");
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, MissingSchemaMeansAllDefaults)
{
    seed(R"({"values":{"arrivalRadiusM":40}})");
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, MalformedJsonMeansAllDefaults)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40)");
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, MissingValuesObjectMeansAllDefaults)
{
    seed(R"({"schema":1})");
    EXPECT_FALSE(open().isLoaded());
}

TEST_F(AppConfigTest, ValuesMustBeAnObject)
{
    seed(R"({"schema":1,"values":[1,2,3]})");
    EXPECT_FALSE(open().isLoaded());
}

TEST_F(AppConfigTest, OversizedFileIsRefused)
{
    std::string big = R"({"schema":1,"values":{"waypointName":"Ridge","pad":")";
    big.append(AppConfig::skMaxFileBytes, 'x');
    big += R"("}})";
    seed(big);

    AppConfig cfg = open();
    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(name(cfg), "Waypoint");
}

TEST_F(AppConfigTest, UndeclaredIdIsSafeToAskFor)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("noSuchField"), 0);
    EXPECT_FALSE(cfg.getBool("noSuchField"));
    EXPECT_FALSE(cfg.has("noSuchField"));

    char buf[8] {};
    EXPECT_EQ(cfg.getString("noSuchField", buf, sizeof(buf)), 0u);
    EXPECT_STREQ(buf, "");
}

TEST_F(AppConfigTest, WrongGetterForTheDeclaredTypeReturnsZero)
{
    seed(R"({"schema":1,"values":{"waypointName":"Ridge"}})");
    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("waypointName"), 0);
    EXPECT_FLOAT_EQ(cfg.getFloat("waypointName"), 0.0f);
}

TEST_F(AppConfigTest, SaveRefusesToWriteAFileTheReaderWouldReject)
{
    // A document that grows past the 8 KB limit could never be read again, and
    // the unknown keys it carries would be lost with it. Padding is an unknown
    // key, exactly as a newer app version's fields would be to an older binary.
    // Sized so the file loads (<= 8192) but re-serialising it with one more
    // key would not.
    const std::string prefix = R"({"schema":1,"values":{"arrivalRadiusM":40,"futureBlob":")";
    const std::string suffix = R"("}})";
    std::string padding(AppConfig::skMaxFileBytes - prefix.size() - suffix.size() - 2, 'x');
    seed(prefix + padding + suffix);
    const std::string before = contents();
    ASSERT_LE(before.size(), AppConfig::skMaxFileBytes);
    AppConfig cfg = open();
    ASSERT_TRUE(cfg.isLoaded());

    ASSERT_TRUE(cfg.setString("waypointName", "0123456789ABCDEF"));
    EXPECT_FALSE(cfg.save());

    // The original survives untouched, and nothing is left half-written.
    EXPECT_EQ(contents(), before);
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));

    // Re-opening still sees the user's values rather than defaults.
    AppConfig again = open();
    EXPECT_TRUE(again.isLoaded());
    EXPECT_EQ(again.getInt("arrivalRadiusM"), 40);
}

TEST_F(AppConfigTest, LongestAllowedFileNameStillLoads)
{
    // The declaration pattern allows up to 63 characters; the reader must not
    // reject the longest name the tooling accepts, or the app silently runs on
    // defaults forever.
    const std::string longest = std::string(58, 'a') + ".json";
    ASSERT_EQ(longest.size(), 63u);
    fx.fileSystem.seedFile("/" + longest,
                           R"({"schema":1,"values":{"arrivalRadiusM":70}})");

    AppConfig cfg(fx.kernel, longest.c_str(), kFields);
    EXPECT_TRUE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 70);
}

TEST_F(AppConfigTest, ParentDirectoryNameIsRefused)
{
    AppConfig cfg(fx.kernel, "..", kFields);

    EXPECT_FALSE(cfg.isLoaded());
    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    EXPECT_FALSE(cfg.save());
    EXPECT_FALSE(fx.fileSystem.exist("/.."));
}

TEST_F(AppConfigTest, FileNameWithoutJsonSuffixIsRefused)
{
    fx.fileSystem.seedFile("/app_config.txt",
                           R"({"schema":1,"values":{"arrivalRadiusM":70}})");
    AppConfig cfg(fx.kernel, "app_config.txt", kFields);

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, FileNameWithAPathIsRefused)
{
    fx.fileSystem.seedFile("/sub/app_config.json",
                           R"({"schema":1,"values":{"arrivalRadiusM":40}})");
    AppConfig cfg(fx.kernel, "sub/app_config.json", kFields);

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

// --- Writing ---------------------------------------------------------------

TEST_F(AppConfigTest, SaveWritesASetValue)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    EXPECT_TRUE(cfg.isDirty());
    ASSERT_TRUE(cfg.save());
    EXPECT_FALSE(cfg.isDirty());

    EXPECT_NE(contents().find("\"arrivalRadiusM\":60"), std::string::npos)
        << contents();
    EXPECT_NE(contents().find("\"schema\":1"), std::string::npos);
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 60);
}

TEST_F(AppConfigTest, SaveLeavesNoTemporaryBehind)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setBool("vibrateOnArrival", false));
    ASSERT_TRUE(cfg.save());

    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));
    EXPECT_TRUE(fx.fileSystem.exist(kPath));
}

TEST_F(AppConfigTest, AnUntouchedValueIsCopiedVerbatim)
{
    // The phone wrote 51.5072; re-serialising a float would turn it into
    // 51.5071983. A value the app did not change must come through unaltered.
    seed(R"({"schema":1,"values":{"targetLatitude":51.5072,"arrivalRadiusM":40}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    ASSERT_TRUE(cfg.save());

    EXPECT_NE(contents().find("\"targetLatitude\":51.5072"), std::string::npos)
        << contents();
}

TEST_F(AppConfigTest, UnknownKeysArePreserved)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40,)"
         R"("futureField":"keep me","futureCount":7,)"
         R"("futureObject":{"a":[1,2]}}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    ASSERT_TRUE(cfg.save());

    const std::string out = contents();
    EXPECT_NE(out.find(R"("futureField":"keep me")"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("futureCount":7)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("futureObject":{"a":[1,2]})"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("arrivalRadiusM":60)"), std::string::npos) << out;
}

TEST_F(AppConfigTest, SavedDocumentIsStillValidJson)
{
    seed(R"({"schema":1,"values":{"futureField":"x"}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setString("waypointName", "Ridge \"top\""));
    ASSERT_TRUE(cfg.setFloat("targetLatitude", -12.25f));
    ASSERT_TRUE(cfg.save());

    // Re-open: a document this class cannot parse would fall back to defaults.
    AppConfig again = open();
    ASSERT_TRUE(again.isLoaded());
    EXPECT_EQ(name(again), "Ridge \"top\"");
    EXPECT_FLOAT_EQ(again.getFloat("targetLatitude"), -12.25f);
    EXPECT_NE(contents().find(R"("futureField":"x")"), std::string::npos);
}

TEST_F(AppConfigTest, DefaultedFieldsAreNotWrittenAsExplicitValues)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    ASSERT_TRUE(cfg.save());

    // Only the field the user actually set is in the file, so "chose it" stays
    // distinguishable from "left it alone".
    EXPECT_EQ(contents().find("waypointName"), std::string::npos) << contents();
    EXPECT_EQ(contents().find("vibrateOnArrival"), std::string::npos);
    EXPECT_FALSE(cfg.has("waypointName"));
}

TEST_F(AppConfigTest, ClearRemovesTheKeyAndRestoresTheDefault)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40,"waypointName":"Ridge"}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.clear("arrivalRadiusM"));
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
    ASSERT_TRUE(cfg.save());

    EXPECT_EQ(contents().find("arrivalRadiusM"), std::string::npos) << contents();
    EXPECT_NE(contents().find("Ridge"), std::string::npos);
    EXPECT_EQ(open().getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, SaveWithNothingDirtyDoesNotTouchTheFile)
{
    const std::string original = R"({"schema":1,"values":{"arrivalRadiusM":40}})";
    seed(original);
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isDirty());
    EXPECT_TRUE(cfg.save());
    EXPECT_EQ(contents(), original);
}

TEST_F(AppConfigTest, SettersClampAndValidate)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    EXPECT_TRUE(cfg.setInt("arrivalRadiusM", 100000));
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 500);

    EXPECT_TRUE(cfg.setFloat("targetLatitude", -1000.0f));
    EXPECT_FLOAT_EQ(cfg.getFloat("targetLatitude"), -90.0f);

    EXPECT_FALSE(cfg.setFloat("targetLatitude", std::nanf("")));
    EXPECT_FALSE(cfg.setInt("waypointName", 1));       // wrong type
    EXPECT_FALSE(cfg.setInt("noSuchField", 1));        // unknown id
    EXPECT_FALSE(cfg.setString("waypointName", nullptr));
}

TEST_F(AppConfigTest, SetStringTruncatesOnACharacterBoundary)
{
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setString("waypointName",
                              "123456789012345" "\xD0\x94" "more"));
    EXPECT_EQ(name(cfg), "123456789012345");

    ASSERT_TRUE(cfg.save());
    EXPECT_EQ(open().getString("waypointName", nullptr, 0), 0u);  // guards null
    EXPECT_EQ(name(open()), "123456789012345");
}

TEST_F(AppConfigTest, WriteFailureLeavesTheOriginalIntact)
{
    const std::string original = R"({"schema":1,"values":{"arrivalRadiusM":40}})";
    seed(original);
    AppConfig cfg = open();

    // Opening the temporary for writing fails, standing in for a storage error.
    fx.fileSystem.failWriteOpenSuffix = ".tmp";

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    EXPECT_FALSE(cfg.save());

    EXPECT_EQ(contents(), original);
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));
}

// --- Crash recovery --------------------------------------------------------

TEST_F(AppConfigTest, AnInterruptedSaveIsCompletedOnNextLaunch)
{
    // The reset landed between removing the old file and renaming the new one.
    fx.fileSystem.seedFile(kTmpPath,
                           R"({"schema":1,"values":{"arrivalRadiusM":75}})");

    AppConfig cfg = open();

    EXPECT_TRUE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 75);
    EXPECT_TRUE(fx.fileSystem.exist(kPath));
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));
}

TEST_F(AppConfigTest, ACorruptTemporaryIsDiscarded)
{
    fx.fileSystem.seedFile(kTmpPath, R"({"schema":1,"values":{"arrival)");

    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));
    EXPECT_FALSE(fx.fileSystem.exist(kPath));
}

TEST_F(AppConfigTest, AStaleTemporaryAlongsideAGoodFileIsRemoved)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40}})");
    fx.fileSystem.seedFile(kTmpPath,
                           R"({"schema":1,"values":{"arrivalRadiusM":999}})");

    AppConfig cfg = open();

    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 40);   // the real file wins
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));
}

// --- Declaration handling --------------------------------------------------

TEST_F(AppConfigTest, AnEmptyFieldTableIsHarmless)
{
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40}})");
    AppConfig cfg(fx.kernel, kFileName, nullptr, 0);

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_FALSE(cfg.has("arrivalRadiusM"));
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 0);
}

TEST_F(AppConfigTest, HasDistinguishesUserValuesFromDefaults)
{
    seed(R"({"schema":1,"values":{"vibrateOnArrival":true}})");
    AppConfig cfg = open();

    // Same value as the default, but the user chose it.
    EXPECT_TRUE(cfg.getBool("vibrateOnArrival"));
    EXPECT_TRUE(cfg.has("vibrateOnArrival"));

    EXPECT_TRUE(cfg.getBool("vibrateOnArrival"));
    EXPECT_FALSE(cfg.has("waypointName"));
}

} // namespace
