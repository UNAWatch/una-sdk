/**
 ******************************************************************************
 * @file    AppConfig.cpp
 * @brief   App-side reader and writer for developer-declared config fields.
 ******************************************************************************
 *
 ******************************************************************************
 */

#define LOG_MODULE_PRX "AppConfig"

#include "SDK/AppConfig/AppConfig.hpp"

#include "SDK/UnaLogger/Logger.h"

extern "C" {
#include "core_json.h"
}

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

// newlib does not put strtof in namespace std, so include the C header too and
// call it unqualified: that resolves on both the host and the ARM toolchain.
#include <stdlib.h>

namespace SDK {

namespace {

/// Longest query this class builds: an id plus its terminator.
constexpr size_t kMaxQueryLen = 40;

/// Longest text a number value may occupy in the file before we refuse it.
/// Matches the limit stated in Docs/app-config-fields.md section 8, and leaves
/// room for the longest plain decimal a binary32 can need (about 55 characters
/// for a denormal) rather than forcing a writer into exponent form.
constexpr size_t kMaxNumberText = 64;

bool isJsonWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/// Length in bytes of the UTF-8 sequence a lead byte starts, 0 if not a lead.
size_t utf8SequenceLength(unsigned char lead)
{
    if (lead < 0x80) {
        return 1;
    }
    if ((lead & 0xE0) == 0xC0) {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0) {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0) {
        return 4;
    }
    return 0;   // a continuation byte or an invalid lead
}

/// Encode a code point as UTF-8. Returns the byte count, 0 if unencodable.
size_t utf8Encode(uint32_t cp, char out[4])
{
    if (cp <= 0x7F) {
        out[0] = static_cast<char>(cp);
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = static_cast<char>(0xC0 | (cp >> 6));
        out[1] = static_cast<char>(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = static_cast<char>(0xE0 | (cp >> 12));
        out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[2] = static_cast<char>(0x80 | (cp & 0x3F));
        return 3;
    }
    if (cp <= 0x10FFFF) {
        out[0] = static_cast<char>(0xF0 | (cp >> 18));
        out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out[3] = static_cast<char>(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0;
}

bool hexDigit(char c, uint32_t &out)
{
    if (c >= '0' && c <= '9') {
        out = static_cast<uint32_t>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
        out = static_cast<uint32_t>(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
        out = static_cast<uint32_t>(c - 'A' + 10);
    } else {
        return false;
    }
    return true;
}

/// Read the 4 hex digits of a \uXXXX escape starting at @p pos.
bool readHex4(const char *data, size_t length, size_t pos, uint32_t &out)
{
    if (pos + 4 > length) {
        return false;
    }
    uint32_t value = 0;
    for (size_t i = 0; i < 4; ++i) {
        uint32_t digit = 0;
        if (!hexDigit(data[pos + i], digit)) {
            return false;
        }
        value = (value << 4) | digit;
    }
    out = value;
    return true;
}

/**
 * @brief   Copy a JSON string body out, decoding escapes.
 *
 * Stops before any character that would not fit whole, so the result is always
 * valid UTF-8 and never a partial multi-byte character. (The same rule as
 * SDK::detail::copyUtf8, applied to a length-delimited, still-escaped slice.)
 *
 * @return  Bytes written, excluding the terminator.
 */
size_t decodeJsonString(const char *data, size_t length, char *out, size_t budget)
{
    size_t written = 0;
    size_t i = 0;

    auto append = [&](const char *bytes, size_t n) -> bool {
        if (written + n > budget) {
            return false;
        }
        std::memcpy(out + written, bytes, n);
        written += n;
        return true;
    };

    while (i < length) {
        if (data[i] == '\\' && i + 1 < length) {
            char esc = data[i + 1];
            char decoded = 0;
            switch (esc) {
            case '"':  decoded = '"';  break;
            case '\\': decoded = '\\'; break;
            case '/':  decoded = '/';  break;
            case 'b':  decoded = '\b'; break;
            case 'f':  decoded = '\f'; break;
            case 'n':  decoded = '\n'; break;
            case 'r':  decoded = '\r'; break;
            case 't':  decoded = '\t'; break;
            case 'u': {
                uint32_t cp = 0;
                if (!readHex4(data, length, i + 2, cp)) {
                    i += 2;
                    continue;
                }
                i += 6;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    // High surrogate: pair it with the low surrogate that must
                    // follow. A lone surrogate is not representable, so drop it.
                    uint32_t low = 0;
                    if (i + 1 < length && data[i] == '\\' && data[i + 1] == 'u' &&
                            readHex4(data, length, i + 2, low) &&
                            low >= 0xDC00 && low <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                        i += 6;
                    } else {
                        continue;
                    }
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    continue;   // stray low surrogate
                }
                char buf[4];
                size_t n = utf8Encode(cp, buf);
                if (n == 0 || !append(buf, n)) {
                    break;
                }
                continue;
            }
            default:
                // Not a JSON escape; take the character literally.
                decoded = esc;
                break;
            }
            i += 2;
            if (!append(&decoded, 1)) {
                break;
            }
            continue;
        }

        size_t seq = utf8SequenceLength(static_cast<unsigned char>(data[i]));
        if (seq == 0 || i + seq > length) {
            ++i;    // malformed: skip the byte rather than emit half a character
            continue;
        }
        if (!append(&data[i], seq)) {
            break;
        }
        i += seq;
    }

    out[written] = '\0';
    return written;
}

/// Parse a JSON integer strictly: no fraction, no exponent, must fit int32.
bool parseInt32(const char *data, size_t length, int32_t &out)
{
    if (length == 0 || length > kMaxNumberText) {
        return false;
    }
    size_t i = 0;
    bool negative = false;
    if (data[i] == '-') {
        negative = true;
        ++i;
    }
    if (i >= length) {
        return false;
    }
    int64_t value = 0;
    for (; i < length; ++i) {
        char c = data[i];
        if (c < '0' || c > '9') {
            return false;   // '.', 'e' and anything else: not an integer
        }
        value = value * 10 + (c - '0');
        if (value > 2147483648LL) {
            return false;
        }
    }
    if (negative) {
        value = -value;
    }
    if (value < -2147483648LL || value > 2147483647LL) {
        return false;
    }
    out = static_cast<int32_t>(value);
    return true;
}

/// Parse a JSON number as a float. Exponent notation is accepted.
bool parseFloat(const char *data, size_t length, float &out)
{
    if (length == 0 || length > kMaxNumberText) {
        return false;
    }
    char text[kMaxNumberText + 1];
    std::memcpy(text, data, length);
    text[length] = '\0';

    char *end = nullptr;
    float value = strtof(text, &end);
    if (end != text + length) {
        return false;
    }
    if (!std::isfinite(value)) {
        return false;
    }
    out = value;
    return true;
}

/// Write a float as JSON. 9 significant digits round-trips a binary32 exactly.
size_t formatFloat(float value, char *out, size_t outSize)
{
    int written = std::snprintf(out, outSize, "%.9g", static_cast<double>(value));
    if (written < 0) {
        out[0] = '0';
        out[1] = '\0';
        return 1;
    }
    return static_cast<size_t>(written) < outSize ? static_cast<size_t>(written)
                                                 : outSize - 1;
}

/**
 * @brief   Buffered JSON output onto an IFile.
 *
 * The document is assembled directly rather than through JsonStreamWriter
 * because unrecognised keys must be copied through byte for byte, and a typed
 * writer cannot re-emit a value whose type it does not know.
 */
class DocumentWriter {
public:
    explicit DocumentWriter(Interface::IFile &file) : mFile(file) {}

    /// True when the document outgrew what the reader will accept.
    bool tooBig() const { return mTooBig; }

    bool put(const char *data, size_t length)
    {
        if (!mOk) {
            return false;
        }
        // A document over the reader's limit would be refused wholesale on the
        // next launch -- taking every preserved unknown key with it -- so refuse
        // to produce one at all and leave the previous file in place.
        if (mTotal + length > AppConfig::skMaxFileBytes) {
            if (!mTooBig) {
                // Logged here rather than only at the end: overflowing part-way
                // through a field would otherwise leave just the generic
                // "could not write" from save(), losing the actual reason.
                LOG_WARNING("the new configuration would exceed the %u-byte "
                            "limit; not saving\n",
                            static_cast<unsigned>(AppConfig::skMaxFileBytes));
            }
            mTooBig = true;
            mOk = false;
            return false;
        }
        mTotal += length;
        while (length > 0) {
            size_t room = sizeof(mBuf) - mUsed;
            size_t take = length < room ? length : room;
            std::memcpy(mBuf + mUsed, data, take);
            mUsed += take;
            data += take;
            length -= take;
            if (mUsed == sizeof(mBuf) && !drain()) {
                return false;
            }
        }
        return true;
    }

    bool put(const char *cstr) { return put(cstr, std::strlen(cstr)); }

    /// Write a JSON string, escaping only what JSON requires.
    bool putString(const char *data, size_t length)
    {
        if (!put("\"", 1)) {
            return false;
        }
        for (size_t i = 0; i < length; ++i) {
            unsigned char c = static_cast<unsigned char>(data[i]);
            switch (c) {
            case '"':  if (!put("\\\"", 2)) return false; break;
            case '\\': if (!put("\\\\", 2)) return false; break;
            case '\b': if (!put("\\b", 2))  return false; break;
            case '\f': if (!put("\\f", 2))  return false; break;
            case '\n': if (!put("\\n", 2))  return false; break;
            case '\r': if (!put("\\r", 2))  return false; break;
            case '\t': if (!put("\\t", 2))  return false; break;
            default:
                if (c < 0x20) {
                    char esc[7];
                    std::snprintf(esc, sizeof(esc), "\\u%04X", c);
                    if (!put(esc, 6)) {
                        return false;
                    }
                } else if (!put(reinterpret_cast<const char *>(&c), 1)) {
                    return false;
                }
                break;
            }
        }
        return put("\"", 1);
    }

    bool finish() { return mOk && drain() && mFile.flush(); }

private:
    bool drain()
    {
        if (mUsed == 0) {
            return mOk;
        }
        size_t written = 0;
        if (!mFile.write(mBuf, mUsed, written) || written != mUsed) {
            mOk = false;
            return false;
        }
        mUsed = 0;
        return true;
    }

    Interface::IFile &mFile;
    char   mBuf[128] {};
    size_t mUsed = 0;
    size_t mTotal = 0;
    bool   mOk = true;
    bool   mTooBig = false;
};

/**
 * @brief   Walk the flat "values" object, reporting each key and raw value.
 *
 * Used to copy through keys the app does not declare. Returns false if the
 * object is not the flat shape this class writes, in which case the caller
 * simply preserves nothing.
 */
template <typename Fn>
bool forEachMember(const char *data, size_t length, Fn &&callback)
{
    size_t i = 0;
    auto skipSpace = [&]() {
        while (i < length && isJsonWhitespace(data[i])) {
            ++i;
        }
    };

    skipSpace();
    if (i >= length || data[i] != '{') {
        return false;
    }
    ++i;
    skipSpace();
    if (i < length && data[i] == '}') {
        return true;
    }

    while (i < length) {
        skipSpace();
        if (i >= length || data[i] != '"') {
            return false;
        }
        size_t keyStart = ++i;
        while (i < length && data[i] != '"') {
            i += (data[i] == '\\') ? 2 : 1;
        }
        if (i >= length) {
            return false;
        }
        size_t keyLength = i - keyStart;
        ++i;

        skipSpace();
        if (i >= length || data[i] != ':') {
            return false;
        }
        ++i;
        skipSpace();

        size_t valueStart = i;
        if (i >= length) {
            return false;
        }
        if (data[i] == '"') {
            ++i;
            while (i < length && data[i] != '"') {
                i += (data[i] == '\\') ? 2 : 1;
            }
            if (i >= length) {
                return false;
            }
            ++i;
        } else if (data[i] == '{' || data[i] == '[') {
            int depth = 0;
            bool inString = false;
            while (i < length) {
                char c = data[i];
                if (inString) {
                    if (c == '\\') {
                        ++i;
                    } else if (c == '"') {
                        inString = false;
                    }
                } else if (c == '"') {
                    inString = true;
                } else if (c == '{' || c == '[') {
                    ++depth;
                } else if (c == '}' || c == ']') {
                    if (--depth == 0) {
                        ++i;
                        break;
                    }
                }
                ++i;
            }
            if (depth != 0) {
                return false;
            }
        } else {
            while (i < length && data[i] != ',' && data[i] != '}' &&
                    !isJsonWhitespace(data[i])) {
                ++i;
            }
        }
        size_t valueLength = i - valueStart;
        if (valueLength == 0) {
            return false;
        }

        callback(&data[keyStart], keyLength, &data[valueStart], valueLength);

        skipSpace();
        if (i < length && data[i] == ',') {
            ++i;
            continue;
        }
        if (i < length && data[i] == '}') {
            return true;
        }
        return false;
    }
    return false;
}

} // namespace

AppConfig::AppConfig(const Kernel &kernel, const char *fileName,
                     const Field *fields, size_t fieldCount)
    : mKernel(kernel)
    , mFields(fields)
    , mCount(fieldCount)
{
    if (mFields == nullptr || mCount == 0) {
        mCount = 0;
        return;
    }
    if (mCount > skMaxFields) {
        LOG_WARNING("%u fields declared, only the first %u are used\n",
                 static_cast<unsigned>(mCount), static_cast<unsigned>(skMaxFields));
        mCount = skMaxFields;
    }

    // A configFile is a bare filename in the sandbox root by contract; refusing
    // anything else here keeps a bad package from reaching outside the sandbox
    // even if the tooling was bypassed.
    if (fileName == nullptr) {
        LOG_WARNING("no config file name\n");
        return;
    }
    size_t nameLen = std::strlen(fileName);
    if (nameLen == 0 || nameLen >= skMaxFileNameLen) {
        LOG_WARNING("config file name is not usable\n");
        return;
    }
    if (std::strchr(fileName, '/') != nullptr ||
            std::strchr(fileName, '\\') != nullptr) {
        LOG_WARNING("config file name must not contain a path\n");
        return;
    }
    // A leading dot covers ".." and hidden names; the .json suffix is the rest
    // of the contract. Checked here as well as in the tooling, so a package that
    // bypassed CI still cannot aim this at a parent directory.
    if (fileName[0] == '.') {
        LOG_WARNING("config file name must not start with '.'\n");
        return;
    }
    if (nameLen < 5 || std::strcmp(&fileName[nameLen - 5], ".json") != 0) {
        LOG_WARNING("config file name must end in .json\n");
        return;
    }

    std::snprintf(mPath, sizeof(mPath), "/%s", fileName);
    std::snprintf(mTmpPath, sizeof(mTmpPath), "/%s.tmp", fileName);

    recoverInterruptedSave();

    if (!readFile()) {
        return;
    }
    indexPresentValues();
    mLoaded = true;
}

void AppConfig::recoverInterruptedSave()
{
    Interface::IFileSystem &fs = mKernel.fs;

    if (fs.exist(mPath)) {
        // A leftover temporary from a save that failed before the swap.
        if (fs.exist(mTmpPath)) {
            fs.remove(mTmpPath);
        }
        return;
    }
    if (!fs.exist(mTmpPath)) {
        return;
    }

    // The file is gone but the temporary is there: the reset landed between the
    // remove and the rename. Finish the job if the temporary is intact.
    bool usable = false;
    std::unique_ptr<Interface::IFile> file = fs.file(mTmpPath);
    if (file && file->open()) {
        size_t size = file->size();
        if (size > 0 && size <= skMaxFileBytes) {
            std::unique_ptr<char[]> buffer(new (std::nothrow) char[size]);
            size_t read = 0;
            usable = buffer && file->read(buffer.get(), size, read) &&
                    read == size &&
                    JSON_Validate(buffer.get(), size) == JSONSuccess;
        }
        file->close();
    }
    file.reset();

    if (usable) {
        LOG_INFO("recovering an interrupted config save\n");
        fs.rename(mTmpPath, mPath);
    } else {
        fs.remove(mTmpPath);
    }
}

bool AppConfig::readFile()
{
    if (!mKernel.fs.exist(mPath)) {
        return false;
    }

    std::unique_ptr<Interface::IFile> file = mKernel.fs.file(mPath);
    if (!file || !file->open()) {
        return false;
    }

    size_t size = file->size();
    if (size == 0 || size > skMaxFileBytes) {
        if (size > skMaxFileBytes) {
            LOG_WARNING("%s is %u bytes, over the %u-byte limit; using defaults\n",
                     mPath, static_cast<unsigned>(size),
                     static_cast<unsigned>(skMaxFileBytes));
        }
        file->close();
        return false;
    }

    std::unique_ptr<char[]> buffer(new (std::nothrow) char[size]);
    size_t read = 0;
    bool ok = buffer && file->read(buffer.get(), size, read) && read == size;
    file->close();
    file.reset();

    if (!ok) {
        return false;
    }
    if (JSON_Validate(buffer.get(), size) != JSONSuccess) {
        LOG_WARNING("%s is not valid JSON; using defaults\n", mPath);
        return false;
    }

    const char *value = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type = JSONInvalid;

    if (JSON_SearchConst(buffer.get(), size, "schema", 6, &value, &valueLen,
                         &type) != JSONSuccess || type != JSONNumber) {
        LOG_WARNING("%s has no schema; using defaults\n", mPath);
        return false;
    }
    int32_t schema = 0;
    if (!parseInt32(value, valueLen, schema) ||
            static_cast<uint32_t>(schema) != skSchemaSupported) {
        LOG_WARNING("%s has schema %.*s, expected %u; using defaults\n", mPath,
                 static_cast<int>(valueLen), value,
                 static_cast<unsigned>(skSchemaSupported));
        return false;
    }

    if (JSON_SearchConst(buffer.get(), size, "values", 6, &value, &valueLen,
                         &type) != JSONSuccess || type != JSONObject) {
        LOG_WARNING("%s has no values object; using defaults\n", mPath);
        return false;
    }

    mRaw = std::move(buffer);
    mRawLen = size;
    mValues.data = value;
    mValues.length = valueLen;
    return true;
}

void AppConfig::indexPresentValues()
{
    char scratch[skMaxStringBytes + 1];

    for (size_t i = 0; i < mCount; ++i) {
        const Field &field = mFields[i];
        bool usable = false;

        switch (field.type) {
        case Type::Bool: {
            bool value = false;
            usable = fileBool(field, value);
            break;
        }
        case Type::Int: {
            int32_t value = 0;
            usable = fileInt(field, value);
            break;
        }
        case Type::Float: {
            float value = 0.0f;
            usable = fileFloat(field, value);
            break;
        }
        case Type::String: {
            size_t written = 0;
            usable = fileString(field, scratch, sizeof(scratch), written);
            break;
        }
        }

        if (usable) {
            mPresent |= (1u << i);
        }
    }
}

const AppConfig::Field *AppConfig::find(const char *id, size_t &indexOut) const
{
    if (id == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < mCount; ++i) {
        if (mFields[i].id != nullptr && std::strcmp(mFields[i].id, id) == 0) {
            indexOut = i;
            return &mFields[i];
        }
    }
    LOG_WARNING("no field declared with id '%s'\n", id);
    return nullptr;
}

bool AppConfig::rawSlice(const Field &field, Slice &out) const
{
    if (mValues.data == nullptr || field.id == nullptr) {
        return false;
    }
    size_t idLen = std::strlen(field.id);
    if (idLen == 0 || idLen >= kMaxQueryLen) {
        return false;
    }

    const char *value = nullptr;
    size_t valueLen = 0;
    JSONTypes_t type = JSONInvalid;
    if (JSON_SearchConst(mValues.data, mValues.length, field.id, idLen, &value,
                         &valueLen, &type) != JSONSuccess) {
        return false;
    }

    // A value of the wrong JSON type (or null) counts as not set, so the
    // field falls back to its default rather than to a coerced nonsense value.
    switch (field.type) {
    case Type::Bool:
        if (type != JSONTrue && type != JSONFalse) {
            return false;
        }
        break;
    case Type::Int:
    case Type::Float:
        if (type != JSONNumber) {
            return false;
        }
        break;
    case Type::String:
        if (type != JSONString) {
            return false;
        }
        break;
    }

    out.data = value;
    out.length = valueLen;
    return true;
}

bool AppConfig::fileBool(const Field &field, bool &out) const
{
    Slice slice {};
    if (!rawSlice(field, slice)) {
        return false;
    }
    // coreJSON reports the type, so the text is "true" or "false".
    out = (slice.length > 0 && (slice.data[0] == 't' || slice.data[0] == 'T'));
    return true;
}

bool AppConfig::fileInt(const Field &field, int32_t &out) const
{
    Slice slice {};
    return rawSlice(field, slice) && parseInt32(slice.data, slice.length, out);
}

bool AppConfig::fileFloat(const Field &field, float &out) const
{
    Slice slice {};
    return rawSlice(field, slice) && parseFloat(slice.data, slice.length, out);
}

bool AppConfig::fileString(const Field &field, char *out, size_t outSize,
                           size_t &written) const
{
    Slice slice {};
    if (!rawSlice(field, slice) || outSize == 0) {
        return false;
    }
    size_t budget = outSize - 1;
    if (field.maxLength > 0 && field.maxLength < budget) {
        budget = field.maxLength;
    }
    written = decodeJsonString(slice.data, slice.length, out, budget);

    // A value below the declared minLength is not usable, so the field falls back
    // to its default -- symmetric with clamping a numeric value into range. The
    // companion app cannot produce one; a hand-edited file can. Note the test is
    // against the decoded length, and only when the value was not truncated to
    // fit (truncation already means it was longer than maxLength >= minLength).
    if (field.minLength > 0 && written < field.minLength) {
        written = 0;
        out[0] = '\0';
        return false;
    }
    return true;
}

bool AppConfig::has(const char *id) const
{
    size_t index = 0;
    if (find(id, index) == nullptr) {
        return false;
    }
    uint32_t bit = 1u << index;
    if (mRemoved & bit) {
        return false;
    }
    return (mPresent & bit) || (mOverride & bit);
}

bool AppConfig::getBool(const char *id) const
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr) {
        return false;
    }
    if (field->type != Type::Bool) {
        LOG_WARNING("'%s' is not a bool field\n", id);
        return false;
    }

    uint32_t bit = 1u << index;
    if ((mOverride & bit) && mSlots) {
        return mSlots[index].boolValue;
    }
    bool value = false;
    if (!(mRemoved & bit) && fileBool(*field, value)) {
        return value;
    }
    return field->boolDefault;
}

int32_t AppConfig::getInt(const char *id) const
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr) {
        return 0;
    }
    if (field->type != Type::Int) {
        LOG_WARNING("'%s' is not an int field\n", id);
        return 0;
    }

    uint32_t bit = 1u << index;
    if ((mOverride & bit) && mSlots) {
        return mSlots[index].intValue;
    }
    int32_t value = 0;
    if (!(mRemoved & bit) && fileInt(*field, value)) {
        if (value < field->intMin) {
            return field->intMin;
        }
        if (value > field->intMax) {
            return field->intMax;
        }
        return value;
    }
    return field->intDefault;
}

float AppConfig::getFloat(const char *id) const
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr) {
        return 0.0f;
    }
    if (field->type != Type::Float) {
        LOG_WARNING("'%s' is not a float field\n", id);
        return 0.0f;
    }

    uint32_t bit = 1u << index;
    if ((mOverride & bit) && mSlots) {
        return mSlots[index].floatValue;
    }
    float value = 0.0f;
    if (!(mRemoved & bit) && fileFloat(*field, value)) {
        if (value < field->floatMin) {
            return field->floatMin;
        }
        if (value > field->floatMax) {
            return field->floatMax;
        }
        return value;
    }
    return field->floatDefault;
}

size_t AppConfig::getString(const char *id, char *out, size_t outSize) const
{
    if (out == nullptr || outSize == 0) {
        return 0;
    }
    out[0] = '\0';

    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr) {
        return 0;
    }
    if (field->type != Type::String) {
        LOG_WARNING("'%s' is not a string field\n", id);
        return 0;
    }

    uint32_t bit = 1u << index;
    if ((mOverride & bit) && mSlots && mSlots[index].stringValue) {
        size_t budget = outSize - 1;
        if (field->maxLength > 0 && field->maxLength < budget) {
            budget = field->maxLength;
        }
        const char *src = mSlots[index].stringValue.get();
        // The overlay already holds a whole-character value; this only has to
        // respect the caller's smaller buffer.
        size_t n = 0;
        while (src[n] != '\0' && n < budget) {
            ++n;
        }
        if (src[n] != '\0') {
            while (n > 0 && (static_cast<unsigned char>(src[n]) & 0xC0) == 0x80) {
                --n;
            }
        }
        std::memcpy(out, src, n);
        out[n] = '\0';
        return n;
    }

    size_t written = 0;
    if (!(mRemoved & bit) && fileString(*field, out, outSize, written)) {
        return written;
    }

    if (field->stringDefault == nullptr) {
        return 0;
    }
    size_t budget = outSize - 1;
    if (field->maxLength > 0 && field->maxLength < budget) {
        budget = field->maxLength;
    }
    size_t n = 0;
    while (field->stringDefault[n] != '\0' && n < budget) {
        ++n;
    }
    if (field->stringDefault[n] != '\0') {
        while (n > 0 &&
                (static_cast<unsigned char>(field->stringDefault[n]) & 0xC0) == 0x80) {
            --n;
        }
    }
    std::memcpy(out, field->stringDefault, n);
    out[n] = '\0';
    return n;
}

AppConfig::Slot *AppConfig::slot(size_t index)
{
    if (index >= mCount) {
        return nullptr;
    }
    if (!mSlots) {
        // Sized to the app's field count, not the 32-field ceiling: four fields
        // should not cost 32 slots' worth of storage.
        mSlots.reset(new (std::nothrow) Slot[mCount]);
        if (!mSlots) {
            LOG_WARNING("could not allocate configuration write storage\n");
            return nullptr;
        }
    }
    return &mSlots[index];
}

bool AppConfig::setBool(const char *id, bool value)
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr || field->type != Type::Bool) {
        return false;
    }
    Slot *entry = slot(index);
    if (entry == nullptr) {
        return false;
    }
    entry->boolValue = value;
    mOverride |= (1u << index);
    mRemoved &= ~(1u << index);
    mDirty = true;
    return true;
}

bool AppConfig::setInt(const char *id, int32_t value)
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr || field->type != Type::Int) {
        return false;
    }
    Slot *entry = slot(index);
    if (entry == nullptr) {
        return false;
    }
    if (value < field->intMin) {
        value = field->intMin;
    } else if (value > field->intMax) {
        value = field->intMax;
    }
    entry->intValue = value;
    mOverride |= (1u << index);
    mRemoved &= ~(1u << index);
    mDirty = true;
    return true;
}

bool AppConfig::setFloat(const char *id, float value)
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr || field->type != Type::Float) {
        return false;
    }
    if (!std::isfinite(value)) {
        LOG_WARNING("refusing a non-finite value for '%s'\n", id);
        return false;
    }
    Slot *entry = slot(index);
    if (entry == nullptr) {
        return false;
    }
    if (value < field->floatMin) {
        value = field->floatMin;
    } else if (value > field->floatMax) {
        value = field->floatMax;
    }
    entry->floatValue = value;
    mOverride |= (1u << index);
    mRemoved &= ~(1u << index);
    mDirty = true;
    return true;
}

bool AppConfig::setString(const char *id, const char *value)
{
    size_t index = 0;
    const Field *field = find(id, index);
    if (field == nullptr || field->type != Type::String || value == nullptr) {
        return false;
    }
    Slot *entry = slot(index);
    if (entry == nullptr) {
        return false;
    }

    // Refuse a value the app's own declaration forbids, so an app cannot write a
    // file that violates the contract the companion app is held to.
    if (field->minLength > 0 && std::strlen(value) < field->minLength) {
        LOG_WARNING("'%s' needs at least %u bytes\n", id,
                    static_cast<unsigned>(field->minLength));
        return false;
    }

    size_t budget = field->maxLength > 0 ? field->maxLength : skMaxStringBytes;
    if (budget > skMaxStringBytes) {
        budget = skMaxStringBytes;
    }

    std::unique_ptr<char[]> copy(new (std::nothrow) char[budget + 1]);
    if (!copy) {
        return false;
    }
    size_t n = 0;
    while (value[n] != '\0' && n < budget) {
        ++n;
    }
    if (value[n] != '\0') {
        while (n > 0 && (static_cast<unsigned char>(value[n]) & 0xC0) == 0x80) {
            --n;
        }
    }
    std::memcpy(copy.get(), value, n);
    copy[n] = '\0';

    entry->stringValue = std::move(copy);
    mOverride |= (1u << index);
    mRemoved &= ~(1u << index);
    mDirty = true;
    return true;
}

bool AppConfig::clear(const char *id)
{
    size_t index = 0;
    if (find(id, index) == nullptr) {
        return false;
    }
    uint32_t bit = 1u << index;
    mRemoved |= bit;
    mOverride &= ~bit;
    if (mSlots) {
        mSlots[index].stringValue.reset();
    }
    mDirty = true;
    return true;
}

bool AppConfig::writeDocument(Interface::IFile &file) const
{
    DocumentWriter out(file);
    char number[kMaxNumberText + 1];

    if (!out.put("{\"schema\":1,\"values\":{")) {
        return false;
    }

    bool first = true;
    auto separator = [&]() -> bool {
        if (first) {
            first = false;
            return true;
        }
        return out.put(",", 1);
    };

    for (size_t i = 0; i < mCount; ++i) {
        const Field &field = mFields[i];
        uint32_t bit = 1u << i;

        if (mRemoved & bit) {
            continue;
        }

        if (mOverride & bit) {
            if (!mSlots) {
                continue;
            }
            if (!separator() || !out.putString(field.id, std::strlen(field.id)) ||
                    !out.put(":", 1)) {
                return false;
            }
            switch (field.type) {
            case Type::Bool:
                if (!out.put(mSlots[i].boolValue ? "true" : "false")) {
                    return false;
                }
                break;
            case Type::Int: {
                int written = std::snprintf(number, sizeof(number), "%ld",
                                            static_cast<long>(mSlots[i].intValue));
                if (written < 0 || !out.put(number, static_cast<size_t>(written))) {
                    return false;
                }
                break;
            }
            case Type::Float: {
                size_t written = formatFloat(mSlots[i].floatValue, number,
                                            sizeof(number));
                if (!out.put(number, written)) {
                    return false;
                }
                break;
            }
            case Type::String: {
                const char *text = mSlots[i].stringValue
                        ? mSlots[i].stringValue.get() : "";
                if (!out.putString(text, std::strlen(text))) {
                    return false;
                }
                break;
            }
            }
            continue;
        }

        // Not touched by the app: copy the value exactly as it arrived, so a
        // value the user typed on the phone never drifts through a reformat.
        if (mPresent & bit) {
            Slice slice {};
            if (!rawSlice(field, slice)) {
                continue;
            }
            if (!separator() || !out.putString(field.id, std::strlen(field.id)) ||
                    !out.put(":", 1)) {
                return false;
            }
            if (field.type == Type::String) {
                if (!out.put("\"", 1) || !out.put(slice.data, slice.length) ||
                        !out.put("\"", 1)) {
                    return false;
                }
            } else if (!out.put(slice.data, slice.length)) {
                return false;
            }
        }
    }

    // Anything this build does not declare is preserved, so an older binary
    // cannot silently discard a newer version's values.
    if (mValues.data != nullptr) {
        bool failed = false;
        forEachMember(mValues.data, mValues.length,
                [&](const char *key, size_t keyLen, const char *value,
                        size_t valueLen) {
            if (failed) {
                return;
            }
            for (size_t i = 0; i < mCount; ++i) {
                const char *id = mFields[i].id;
                if (id != nullptr && std::strlen(id) == keyLen &&
                        std::memcmp(id, key, keyLen) == 0) {
                    return;     // declared: already emitted above
                }
            }
            if (!separator() || !out.put("\"", 1) || !out.put(key, keyLen) ||
                    !out.put("\":", 2) || !out.put(value, valueLen)) {
                failed = true;
            }
        });
        if (failed) {
            return false;
        }
    }

    // DocumentWriter reports an overflow where it happens, so a failure here
    // needs no further diagnosis.
    return out.put("}}") && out.finish();
}

bool AppConfig::save()
{
    if (!mDirty) {
        return true;
    }
    if (mPath[0] == '\0') {
        return false;
    }

    Interface::IFileSystem &fs = mKernel.fs;

    // Build the replacement alongside the original: a reset or a flat battery
    // part-way through must not destroy the values already on the watch.
    {
        std::unique_ptr<Interface::IFile> tmp = fs.file(mTmpPath);
        if (!tmp || !tmp->open(true, true)) {
            LOG_WARNING("could not open %s for writing\n", mTmpPath);
            return false;
        }
        bool ok = writeDocument(*tmp);
        tmp->close();
        tmp.reset();
        if (!ok) {
            LOG_WARNING("could not write %s\n", mTmpPath);
            fs.remove(mTmpPath);
            return false;
        }
    }

    if (fs.exist(mPath) && !fs.remove(mPath)) {
        LOG_WARNING("could not replace %s\n", mPath);
        fs.remove(mTmpPath);
        return false;
    }
    if (!fs.rename(mTmpPath, mPath)) {
        // The window this leaves is exactly what recoverInterruptedSave()
        // repairs on the next launch.
        LOG_WARNING("could not rename %s to %s\n", mTmpPath, mPath);
        return false;
    }

    // Re-read so later getters see the saved document, and so the raw slices
    // the write path copies from stay valid.
    mPresent = 0;
    mOverride = 0;
    mRemoved = 0;
    mSlots.reset();
    mValues = Slice {};
    mRaw.reset();
    mRawLen = 0;
    mLoaded = readFile();
    if (mLoaded) {
        indexPresentValues();
    }
    mDirty = false;

    if (!mLoaded) {
        // The document reached the disk but this build cannot read it back, so
        // the getters have just reverted to defaults. Report the failure rather
        // than letting the caller believe its values are safe.
        LOG_WARNING("%s was written but could not be read back\n", mPath);
        return false;
    }
    return true;
}

} // namespace SDK
