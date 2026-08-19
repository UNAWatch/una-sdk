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

#include <cstdio>
#include <limits>
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

// Two fields the main table cannot express: one whose minimum and maximum length
// are equal, so truncation and the minimum collide, and one whose minimum is
// longer than a small caller buffer.
constexpr AppConfig::Field kStrictFields[] = {
    AppConfig::stringField("exactSixteen", "0123456789abcdef", 16, 16),
    AppConfig::stringField("atLeastEight", "12345678", 8, 32),
};

class AppConfigTest : public ::testing::Test {
protected:
    SDK::TestSupport::KernelFixture fx;

    void seed(const std::string &contents)
    {
        fx.fileSystem.seedFile(kPath, contents);
    }

    AppConfig open() { return AppConfig(fx.kernel, kFileName, kFields); }

    AppConfig openStrict()
    {
        return AppConfig(fx.kernel, kFileName, kStrictFields);
    }

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

// The escape tests below must seed the six ASCII characters \, u, 0, 4, 1, 4 --
// NOT the pre-encoded UTF-8 bytes of the character. Writing "\\uXXXX" in a plain
// literal and concatenating it with the raw JSON around it makes that explicit:
// seeding the character itself would exercise plain byte passthrough and leave
// readHex4/utf8Encode and the surrogate-pair branch untested.
TEST_F(AppConfigTest, DecodesUnicodeEscapes)
{
    seed(R"({"schema":1,"values":{"waypointName":"De )" "\\u0414"
         R"("}})");
    AppConfig cfg = open();

    // U+0414 decodes to the two UTF-8 bytes D0 94.
    EXPECT_EQ(name(cfg), std::string("De ") + "\xD0\x94");
}

TEST_F(AppConfigTest, DecodesSurrogatePairs)
{
    // U+1F600 as a surrogate pair, the only way JSON can express it.
    seed(R"({"schema":1,"values":{"waypointName":")" "\\uD83D\\uDE00"
         R"("}})");
    AppConfig cfg = open();

    EXPECT_EQ(name(cfg), "\xF0\x9F\x98\x80");
}

TEST_F(AppConfigTest, LoneSurrogateInvalidatesTheWholeFile)
{
    // A high surrogate with no low surrogate is not valid JSON, and coreJSON
    // rejects the document rather than the one value -- so this is the exception
    // to "one bad key costs only its own field": every field falls back.
    seed(R"({"schema":1,"values":{"waypointName":"A)" "\\uD83D"
         R"(B","arrivalRadiusM":70}})");
    AppConfig cfg = open();

    EXPECT_FALSE(cfg.isLoaded());
    EXPECT_EQ(name(cfg), "Waypoint");
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 25);
}

TEST_F(AppConfigTest, EscapedValueIsTruncatedByDecodedLength)
{
    // Nine escapes are 54 bytes of JSON but 18 bytes decoded, so maxLength (16)
    // truncates: the limit applies to the decoded value, not to its encoding.
    seed(R"({"schema":1,"values":{"waypointName":")"
         "\\u0414\\u0414\\u0414\\u0414\\u0414\\u0414\\u0414\\u0414\\u0414"
         R"("}})");
    AppConfig cfg = open();

    // Eight two-byte characters fit exactly; the ninth is dropped whole.
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

    EXPECT_FALSE(cfg.setFloat("targetLatitude",
                              std::numeric_limits<float>::quiet_NaN()));
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

TEST_F(AppConfigTest, SetStringRefusesAValueTruncationWouldMakeTooShort)
{
    // 15 ASCII bytes then a 2-byte character is 17 bytes in, but maxLength is 16
    // and the boundary rule drops the pair rather than half of it -- so only 15
    // bytes would be stored, below the declared minimum of 16. Judging the input
    // instead of the stored result would accept it and write a file the reader
    // then refuses.
    seed(R"({"schema":1,"values":{}})");
    AppConfig cfg = openStrict();

    EXPECT_FALSE(cfg.setString("exactSixteen", "123456789012345" "\xD0\x94"));
    EXPECT_FALSE(cfg.isDirty());

    // 16 ASCII bytes is both the minimum and the maximum, so it must pass.
    EXPECT_TRUE(cfg.setString("exactSixteen", "1234567890123456"));
    EXPECT_TRUE(cfg.isDirty());
    ASSERT_TRUE(cfg.save());
    EXPECT_NE(contents().find(R"("exactSixteen":"1234567890123456")"),
              std::string::npos) << contents();
}

TEST_F(AppConfigTest, AStoredValueBelowMinLengthFallsBackToTheDefault)
{
    seed(R"({"schema":1,"values":{"atLeastEight":"short"}})");
    AppConfig cfg = openStrict();

    ASSERT_TRUE(cfg.isLoaded());
    char buf[64] {};
    EXPECT_EQ(cfg.getString("atLeastEight", buf, sizeof(buf)), 8u);
    EXPECT_STREQ(buf, "12345678");          // the declared default
    EXPECT_FALSE(cfg.has("atLeastEight"));  // unusable counts as absent
}

TEST_F(AppConfigTest, ASmallCallerBufferIsNotATooShortValue)
{
    // The buffer cannot hold minLength bytes, so a short result is the caller's
    // own doing -- they asked for a value truncated to fit, not a verdict on what
    // the file holds. Applying the minimum here would hand back the default and
    // report the stored value as absent.
    seed(R"({"schema":1,"values":{"atLeastEight":"twelve bytes"}})");
    AppConfig cfg = openStrict();

    char buf[5] {};
    EXPECT_EQ(cfg.getString("atLeastEight", buf, sizeof(buf)), 4u);
    EXPECT_STREQ(buf, "twel");
    EXPECT_TRUE(cfg.has("atLeastEight"));
}

TEST_F(AppConfigTest, ADuplicatedKeyCollapsesToOneEntryOnSave)
{
    // JSON does not forbid a repeated key and leaves the winner undefined. A
    // hand-edited file or a buggy writer can produce one, and the two sides of
    // this feature disagree about which copy wins: this reader takes the first,
    // a phone-side JSON.parse takes the last. So a save must resolve it, for a
    // declared field and for a preserved undeclared one alike -- the second is
    // the case that matters, since an undeclared key belongs to the phone.
    seed(R"({"schema":1,"values":{"arrivalRadiusM":40,"arrivalRadiusM":41,)"
         R"("futureField":"a","futureField":"b"}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.isLoaded());
    EXPECT_EQ(cfg.getInt("arrivalRadiusM"), 40);

    ASSERT_TRUE(cfg.setBool("vibrateOnArrival", false));
    ASSERT_TRUE(cfg.save());

    const std::string out = contents();
    EXPECT_EQ(out.find("arrivalRadiusM"), out.rfind("arrivalRadiusM")) << out;
    EXPECT_NE(out.find(R"("arrivalRadiusM":40)"), std::string::npos) << out;

    // The undeclared key keeps its first value and appears once.
    EXPECT_EQ(out.find("futureField"), out.rfind("futureField")) << out;
    EXPECT_NE(out.find(R"("futureField":"a")"), std::string::npos) << out;
    EXPECT_EQ(out.find(R"("futureField":"b")"), std::string::npos) << out;

    AppConfig again = open();
    ASSERT_TRUE(again.isLoaded());
    EXPECT_EQ(again.getInt("arrivalRadiusM"), 40);
}

TEST_F(AppConfigTest, DeduplicationDoesNotDisturbDistinctUnknownKeys)
{
    // The dedupe compares whole key text, so keys that merely share a prefix,
    // or whose text appears inside a neighbouring value, must all survive.
    seed(R"({"schema":1,"values":{"future":1,"futureField":2,"futures":3,)"
         R"("note":"future futureField"}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 30));
    ASSERT_TRUE(cfg.save());

    const std::string out = contents();
    EXPECT_NE(out.find(R"("future":1)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("futureField":2)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("futures":3)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("note":"future futureField")"), std::string::npos) << out;
}

TEST_F(AppConfigTest, ASecondSaveBuildsOnTheFirst)
{
    seed(R"({"schema":1,"values":{"targetLatitude":51.5072,"futureField":"x"}})");
    AppConfig cfg = open();

    ASSERT_TRUE(cfg.setInt("arrivalRadiusM", 60));
    ASSERT_TRUE(cfg.save());
    EXPECT_FALSE(cfg.isDirty());

    ASSERT_TRUE(cfg.setString("waypointName", "Summit"));
    ASSERT_TRUE(cfg.save());
    EXPECT_FALSE(cfg.isDirty());
    EXPECT_FALSE(fx.fileSystem.exist(kTmpPath));

    // Both edits, the untouched phone value and the unknown key all survive the
    // second pass: the in-memory document is the authority, not whatever the
    // first save happened to leave on disk.
    const std::string out = contents();
    EXPECT_NE(out.find(R"("arrivalRadiusM":60)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("waypointName":"Summit")"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("targetLatitude":51.5072)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("futureField":"x")"), std::string::npos) << out;

    AppConfig again = open();
    ASSERT_TRUE(again.isLoaded());
    EXPECT_EQ(again.getInt("arrivalRadiusM"), 60);
    EXPECT_EQ(name(again), "Summit");
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

TEST_F(AppConfigTest, FieldsBeyondTheSupportedCountAreDropped)
{
    // The present/override/removed sets are 32 bits wide, which is why the
    // declaration is capped at 32 fields. The template constructor rejects a
    // longer table at compile time; this is the runtime path, reached only by
    // an app that passes a count of its own.
    constexpr size_t kTooMany = AppConfig::skMaxFields + 2;
    char ids[kTooMany][8] {};
    AppConfig::Field many[kTooMany] {};
    for (size_t i = 0; i < kTooMany; ++i) {
        std::snprintf(ids[i], sizeof(ids[i]), "f%02u", static_cast<unsigned>(i));
        many[i] = AppConfig::intField(ids[i], 0, 0, 100);
    }

    seed(R"({"schema":1,"values":{"f00":1,"f31":31,"f32":32,"f33":33}})");
    AppConfig cfg(fx.kernel, kFileName, many, kTooMany);

    ASSERT_TRUE(cfg.isLoaded());
    EXPECT_TRUE(cfg.has("f00"));
    EXPECT_EQ(cfg.getInt("f31"), 31);
    EXPECT_TRUE(cfg.has("f31"));

    // Past the cap the field does not exist at all, so a setter reports failure
    // rather than shifting a bit off the end of the sets above.
    EXPECT_FALSE(cfg.has("f32"));
    EXPECT_EQ(cfg.getInt("f32"), 0);
    EXPECT_FALSE(cfg.setInt("f32", 5));
    EXPECT_FALSE(cfg.isDirty());

    // The dropped keys are still unknown members as far as the writer is
    // concerned, so a build that does declare them finds the user's answers.
    ASSERT_TRUE(cfg.setInt("f00", 7));
    ASSERT_TRUE(cfg.save());
    const std::string out = contents();
    EXPECT_NE(out.find(R"("f32":32)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("f33":33)"), std::string::npos) << out;
    EXPECT_NE(out.find(R"("f00":7)"), std::string::npos) << out;
}

} // namespace
