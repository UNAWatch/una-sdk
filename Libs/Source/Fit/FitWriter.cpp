/**
 ******************************************************************************
 * @file    FitWriter.cpp
 * @brief   Native FIT-format encoder engine implementation.
 ******************************************************************************
 */

#include "SDK/Fit/FitWriter.hpp"

#include "SDK/Fit/FitCrc.hpp"

#include <cstring>

namespace SDK::Fit {

namespace {
    constexpr uint8_t kHeaderSize = 14;
    constexpr uint8_t kArchLittleEndian = 0x00;
    constexpr uint8_t kDefinitionFlag = 0x40;   ///< record header bit 6
    constexpr uint8_t kDeveloperFlag  = 0x20;   ///< record header bit 5

    template <typename T>
    void appendLE(std::vector<uint8_t>& out, T value, uint8_t nbytes)
    {
        auto u = static_cast<uint64_t>(value);
        for (uint8_t i = 0; i < nbytes; ++i) {
            out.push_back(static_cast<uint8_t>(u & 0xFFu));
            u >>= 8;
        }
    }
}  // namespace

bool FitWriter::begin(uint16_t profileVersion, uint8_t protocolVersion)
{
    mBuf.clear();
    mBuf.resize(kHeaderSize, 0);
    mBuf[0] = kHeaderSize;
    mBuf[1] = protocolVersion;
    mBuf[2] = static_cast<uint8_t>(profileVersion & 0xFFu);
    mBuf[3] = static_cast<uint8_t>((profileVersion >> 8) & 0xFFu);
    // mBuf[4..7] data size: filled in by finish().
    mBuf[8]  = '.';
    mBuf[9]  = 'F';
    mBuf[10] = 'I';
    mBuf[11] = 'T';
    // mBuf[12..13] header CRC: left 0x0000 (permitted by the spec).
    mBegun = true;
    return mBegun;
}

bool FitWriter::defineMessage(uint8_t localType, uint16_t globalMesgNum,
                              std::initializer_list<Field>    fields,
                              std::initializer_list<DevField> devFields)
{
    if (!mBegun || localType > 15 || fields.size() > 255 || devFields.size() > 255) {
        mOk = false;
        return false;
    }

    const uint8_t hdr = static_cast<uint8_t>(
        kDefinitionFlag | (devFields.size() ? kDeveloperFlag : 0) | (localType & 0x0Fu));
    mBuf.push_back(hdr);
    mBuf.push_back(0x00);                 // reserved
    mBuf.push_back(kArchLittleEndian);    // architecture
    appendLE(mBuf, globalMesgNum, 2);
    mBuf.push_back(static_cast<uint8_t>(fields.size()));

    size_t payload = 0;
    for (const Field& f : fields) {
        const uint8_t fieldBytes = static_cast<uint8_t>(baseTypeSize(f.baseType) * f.count);
        mBuf.push_back(f.fieldDefNum);
        mBuf.push_back(fieldBytes);
        mBuf.push_back(baseTypeId(f.baseType));
        payload += fieldBytes;
    }

    if (devFields.size()) {
        mBuf.push_back(static_cast<uint8_t>(devFields.size()));
        for (const DevField& d : devFields) {
            mBuf.push_back(d.fieldNum);
            mBuf.push_back(d.sizeBytes);
            mBuf.push_back(d.devDataIndex);
            payload += d.sizeBytes;
        }
    }

    mExpected[localType] = payload;
    mDefined[localType]  = true;
    return true;
}

bool FitWriter::emitData(uint8_t localType, const std::vector<uint8_t>& payload)
{
    if (!mBegun || localType > 15 || !mDefined[localType]
        || payload.size() != mExpected[localType]) {
        mOk = false;
        return false;
    }
    mBuf.push_back(static_cast<uint8_t>(localType & 0x0Fu));   // normal data header
    mBuf.insert(mBuf.end(), payload.begin(), payload.end());
    return true;
}

bool FitWriter::finish()
{
    if (!mOk || !mBegun || mBuf.size() < kHeaderSize) {
        return false;
    }

    // Fill in the data-size field (bytes 4..7, little-endian).
    const uint32_t dataSize = static_cast<uint32_t>(mBuf.size() - kHeaderSize);
    mBuf[4] = static_cast<uint8_t>(dataSize & 0xFFu);
    mBuf[5] = static_cast<uint8_t>((dataSize >> 8) & 0xFFu);
    mBuf[6] = static_cast<uint8_t>((dataSize >> 16) & 0xFFu);
    mBuf[7] = static_cast<uint8_t>((dataSize >> 24) & 0xFFu);

    // Trailing file CRC over header + data, little-endian.
    const uint16_t crc = fitCrcUpdate(0, mBuf.data(), mBuf.size());
    appendLE(mBuf, crc, 2);

    size_t bw = 0;
    if (!mFile.write(reinterpret_cast<const char*>(mBuf.data()), mBuf.size(), bw)
        || bw != mBuf.size()) {
        mOk = false;
        return false;
    }
    mFile.flush();
    return mOk;
}

// --- Data builder -----------------------------------------------------------

FitWriter::Data& FitWriter::Data::u8(uint8_t v)  { mPayload.push_back(v); return *this; }
FitWriter::Data& FitWriter::Data::i8(int8_t v)   { mPayload.push_back(static_cast<uint8_t>(v)); return *this; }
FitWriter::Data& FitWriter::Data::u16(uint16_t v){ appendLE(mPayload, v, 2); return *this; }
FitWriter::Data& FitWriter::Data::i16(int16_t v) { appendLE(mPayload, static_cast<uint16_t>(v), 2); return *this; }
FitWriter::Data& FitWriter::Data::u32(uint32_t v){ appendLE(mPayload, v, 4); return *this; }
FitWriter::Data& FitWriter::Data::i32(int32_t v) { appendLE(mPayload, static_cast<uint32_t>(v), 4); return *this; }
FitWriter::Data& FitWriter::Data::u64(uint64_t v){ appendLE(mPayload, v, 8); return *this; }
FitWriter::Data& FitWriter::Data::i64(int64_t v) { appendLE(mPayload, static_cast<uint64_t>(v), 8); return *this; }

FitWriter::Data& FitWriter::Data::f32(float v)
{
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    appendLE(mPayload, bits, 4);
    return *this;
}

FitWriter::Data& FitWriter::Data::f64(double v)
{
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    appendLE(mPayload, bits, 8);
    return *this;
}

FitWriter::Data& FitWriter::Data::str(const char* s, uint8_t fieldBytes)
{
    const size_t len = s ? std::strlen(s) : 0;
    for (uint8_t i = 0; i < fieldBytes; ++i) {
        mPayload.push_back(i < len ? static_cast<uint8_t>(s[i]) : 0x00);
    }
    return *this;
}

FitWriter::Data& FitWriter::Data::bytes(const void* p, size_t n)
{
    const auto* b = static_cast<const uint8_t*>(p);
    mPayload.insert(mPayload.end(), b, b + n);
    return *this;
}

FitWriter::Data& FitWriter::Data::invalid(BaseType t, uint8_t count)
{
    const uint8_t  sz  = baseTypeSize(t);
    const uint64_t inv = baseTypeInvalid(t);
    for (uint8_t i = 0; i < count; ++i) {
        appendLE(mPayload, inv, sz);
    }
    return *this;
}

bool FitWriter::Data::write()
{
    return mWriter.emitData(mLocalType, mPayload);
}

}  // namespace SDK::Fit
