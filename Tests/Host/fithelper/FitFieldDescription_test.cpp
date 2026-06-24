/**
 * Host unit tests for FitHelper developer-field descriptions: a written
 * FIT_FIELD_DESCRIPTION message must carry the field_name (and units) so FIT
 * consumers can label the developer fields it declares.
 *
 * Built with FIT_PRODUCT_RELEASE (as the device apps are): the release profile
 * strips FIELD_DESCRIPTION string fields to 1 byte, so the encoder must size
 * the name/units fields itself rather than trusting the profile definition.
 */

#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "FakeFileSystem.hpp"
#include "SDK/FitHelper/FitHelper.hpp"

extern "C" {
#include "fit_product.h"
}

using SDK::Component::FitHelper;
using SDK::Test::FakeFileSystem;

namespace {

// Write one developer field_description (definition + data) via FitHelper and
// return the raw bytes emitted to the FIT file.
std::string encodeFieldDescription(const char* name, const char* units)
{
    FakeFileSystem fs;
    auto file = fs.file("/dev.fit");
    file->open(/*wMode=*/true, /*override=*/true);

    // A developer field helper: local message id 0, field-definition number 7.
    FitHelper fh(/*msgID=*/0, /*fieldID=*/7, /*container=*/{});
    fh.writeFieldDescription(name, units, FIT_BASE_TYPE_UINT8, file.get());

    file->close();
    return fs.fileContents("/dev.fit");
}

bool contains(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}

} // namespace

TEST(FitFieldDescription, EncodesFieldNameAndUnits)
{
    const std::string out = encodeFieldDescription("hr_external", "bpm");
    EXPECT_TRUE(contains(out, "hr_external")) << "field_name missing from FIT output";
    EXPECT_TRUE(contains(out, "bpm"))         << "units missing from FIT output";
}

TEST(FitFieldDescription, EncodesFieldNameWhenNoUnits)
{
    // hr_source has no units; the name must still be present.
    const std::string out = encodeFieldDescription("hr_source", nullptr);
    EXPECT_TRUE(contains(out, "hr_source")) << "field_name missing from FIT output";
}
