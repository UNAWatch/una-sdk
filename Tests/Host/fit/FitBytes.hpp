/**
 ******************************************************************************
 * @file    FitBytes.hpp
 * @brief   Builds FIT files byte by byte for decoder tests.
 *
 * Independent of SDK::Fit::FitWriter, so a mistake shared by the encoder
 * and a decoder cannot hide. It can produce what FitWriter never does:
 * big-endian records, compressed-timestamp headers, developer data, a
 * 12-byte header, and deliberately damaged files.
 *
 * Test-only; not part of the SDK.
 ******************************************************************************
 */

#ifndef TEST_FIT_BYTES_HPP
#define TEST_FIT_BYTES_HPP

#include "SDK/Fit/FitCrc.hpp"

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

namespace testfit {

/// One field in a definition: number, size in bytes, base-type id.
struct FDef {
    uint8_t num;
    uint8_t size;
    uint8_t base;
};

/// A field value for a data record, encoded to its definition's size.
struct FVal {
    enum class Kind { Uint, Text, Bytes } kind;
    uint64_t             u = 0;
    std::string          text;
    std::vector<uint8_t> bytes;

    static FVal U(uint64_t v) { FVal f{Kind::Uint}; f.u = v; return f; }
    static FVal T(std::string s) { FVal f{Kind::Text}; f.text = std::move(s); return f; }
    static FVal B(std::vector<uint8_t> b) { FVal f{Kind::Bytes}; f.bytes = std::move(b); return f; }
};

// Base-type ids.
constexpr uint8_t kEnum = 0x00, kU8 = 0x02, kU16 = 0x84, kU32 = 0x86, kStr = 0x07,
                  kU8z = 0x0A, kU32z = 0x8C;

class FitBytes {
public:
    explicit FitBytes(uint8_t headerSize = 14, uint8_t protocol = 0x10, uint16_t profile = 21215)
        : mHeaderSize(headerSize), mProtocol(protocol), mProfile(profile) {}

    /// Definition record. Remembers the layout so data() can encode values.
    FitBytes& def(uint8_t local, uint16_t global, std::initializer_list<FDef> fields,
                  bool bigEndian = true, std::vector<FDef> devFields = {})
    {
        mBody.push_back(static_cast<uint8_t>(0x40 | (devFields.empty() ? 0 : 0x20) | local));
        mBody.push_back(0);
        mBody.push_back(bigEndian ? 1 : 0);
        put16(global, bigEndian);
        mBody.push_back(static_cast<uint8_t>(fields.size()));
        for (const FDef& f : fields) {
            mBody.insert(mBody.end(), {f.num, f.size, f.base});
        }
        if (!devFields.empty()) {
            mBody.push_back(static_cast<uint8_t>(devFields.size()));
            for (const FDef& f : devFields) {
                mBody.insert(mBody.end(), {f.num, f.size, f.base});
            }
        }
        mLayout[local] = Layout{std::vector<FDef>(fields), std::move(devFields), bigEndian};
        return *this;
    }

    /// Data record for @p local, one value per defined field, in order.
    /// Developer fields are filled with 0xAA.
    FitBytes& data(uint8_t local, std::initializer_list<FVal> values)
    {
        mBody.push_back(local);
        encode(local, values);
        return *this;
    }

    /// Data record with a compressed-timestamp header (local type 0-3).
    FitBytes& compressed(uint8_t local, uint8_t timeOffset, std::initializer_list<FVal> values)
    {
        mBody.push_back(static_cast<uint8_t>(0x80 | ((local & 0x03) << 5) | (timeOffset & 0x1F)));
        encode(local, values);
        return *this;
    }

    /// Raw bytes into the record stream.
    FitBytes& raw(std::initializer_list<uint8_t> b)
    {
        mBody.insert(mBody.end(), b);
        return *this;
    }

    /// The finished file: header, records, file CRC.
    std::vector<uint8_t> build(bool headerCrc = true) const
    {
        std::vector<uint8_t> out;
        out.push_back(mHeaderSize);
        out.push_back(mProtocol);
        out.push_back(static_cast<uint8_t>(mProfile & 0xFF));
        out.push_back(static_cast<uint8_t>(mProfile >> 8));
        const uint32_t n = static_cast<uint32_t>(mBody.size());
        for (int i = 0; i < 4; ++i) {
            out.push_back(static_cast<uint8_t>(n >> (8 * i)));
        }
        out.insert(out.end(), {'.', 'F', 'I', 'T'});
        if (mHeaderSize >= 14) {
            const uint16_t c = headerCrc ? SDK::Fit::fitCrcUpdate(0, out.data(), 12) : 0;
            out.push_back(static_cast<uint8_t>(c & 0xFF));
            out.push_back(static_cast<uint8_t>(c >> 8));
            out.resize(mHeaderSize, 0);
        }
        out.insert(out.end(), mBody.begin(), mBody.end());
        const uint16_t crc = SDK::Fit::fitCrcUpdate(0, out.data(), out.size());
        out.push_back(static_cast<uint8_t>(crc & 0xFF));
        out.push_back(static_cast<uint8_t>(crc >> 8));
        return out;
    }

private:
    struct Layout {
        std::vector<FDef> fields, dev;
        bool              bigEndian = true;
    };

    void put16(uint16_t v, bool be)
    {
        if (be) {
            mBody.push_back(static_cast<uint8_t>(v >> 8));
            mBody.push_back(static_cast<uint8_t>(v & 0xFF));
        } else {
            mBody.push_back(static_cast<uint8_t>(v & 0xFF));
            mBody.push_back(static_cast<uint8_t>(v >> 8));
        }
    }

    void encode(uint8_t local, std::initializer_list<FVal> values)
    {
        const Layout& l = mLayout[local];
        size_t i = 0;
        for (const FVal& v : values) {
            const FDef& f = l.fields.at(i++);
            if (v.kind == FVal::Kind::Uint) {
                for (uint8_t k = 0; k < f.size; ++k) {
                    const uint8_t shift = static_cast<uint8_t>(8 * (l.bigEndian ? f.size - 1 - k : k));
                    mBody.push_back(static_cast<uint8_t>(v.u >> shift));
                }
            } else {
                const std::vector<uint8_t> src =
                    v.kind == FVal::Kind::Text ? std::vector<uint8_t>(v.text.begin(), v.text.end())
                                               : v.bytes;
                for (uint8_t k = 0; k < f.size; ++k) {
                    mBody.push_back(k < src.size() ? src[k] : 0);
                }
            }
        }
        for (const FDef& f : l.dev) {
            mBody.insert(mBody.end(), f.size, 0xAA);
        }
    }

    uint8_t              mHeaderSize;
    uint8_t              mProtocol;
    uint16_t             mProfile;
    std::vector<uint8_t> mBody;
    Layout               mLayout[16];
};

}  // namespace testfit

#endif  // TEST_FIT_BYTES_HPP
