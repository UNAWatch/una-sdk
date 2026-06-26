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

bool FitWriter::writeBytes(const void* p, size_t n)
{
    if (!mOk) {
        return false;
    }
    size_t bw = 0;
    if (!mFile.write(static_cast<const char*>(p), n, bw) || bw != n) {
        mOk = false;
    }
    return mOk;
}

namespace {
    void buildHeader(uint8_t hdr[14], uint8_t protocolVersion, uint16_t profileVersion,
                     uint32_t dataSize, bool withCrc)
    {
        hdr[0] = 14;  // header size
        hdr[1] = protocolVersion;
        hdr[2] = static_cast<uint8_t>(profileVersion & 0xFFu);
        hdr[3] = static_cast<uint8_t>((profileVersion >> 8) & 0xFFu);
        hdr[4] = static_cast<uint8_t>(dataSize & 0xFFu);
        hdr[5] = static_cast<uint8_t>((dataSize >> 8) & 0xFFu);
        hdr[6] = static_cast<uint8_t>((dataSize >> 16) & 0xFFu);
        hdr[7] = static_cast<uint8_t>((dataSize >> 24) & 0xFFu);
        hdr[8]  = '.';
        hdr[9]  = 'F';
        hdr[10] = 'I';
        hdr[11] = 'T';
        const uint16_t crc = withCrc ? SDK::Fit::fitCrcUpdate(0, hdr, 12) : 0;
        hdr[12] = static_cast<uint8_t>(crc & 0xFFu);
        hdr[13] = static_cast<uint8_t>((crc >> 8) & 0xFFu);
    }
}  // namespace

bool FitWriter::begin(uint16_t profileVersion, uint8_t protocolVersion)
{
    mProtocolVersion = protocolVersion;
    mProfileVersion  = profileVersion;
    uint8_t hdr[14];
    buildHeader(hdr, protocolVersion, profileVersion, /*dataSize=*/0, /*withCrc=*/false);
    mBegun = writeBytes(hdr, sizeof(hdr));
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

    std::vector<uint8_t> rec;
    const uint8_t hdr = static_cast<uint8_t>(
        kDefinitionFlag | (devFields.size() ? kDeveloperFlag : 0) | (localType & 0x0Fu));
    rec.push_back(hdr);
    rec.push_back(0x00);                 // reserved
    rec.push_back(kArchLittleEndian);    // architecture
    appendLE(rec, globalMesgNum, 2);
    rec.push_back(static_cast<uint8_t>(fields.size()));

    size_t payload = 0;
    for (const Field& f : fields) {
        // A field's size is one byte on the wire, so an array field cannot
        // exceed 255 bytes; reject rather than silently truncate (which would
        // desync the data-message payload and corrupt the file).
        const size_t fieldBytes = static_cast<size_t>(baseTypeSize(f.baseType)) * f.count;
        if (fieldBytes > 255) {
            mOk = false;
            return false;
        }
        rec.push_back(f.fieldDefNum);
        rec.push_back(static_cast<uint8_t>(fieldBytes));
        rec.push_back(baseTypeId(f.baseType));
        payload += fieldBytes;
    }

    if (devFields.size()) {
        rec.push_back(static_cast<uint8_t>(devFields.size()));
        for (const DevField& d : devFields) {
            rec.push_back(d.fieldNum);
            rec.push_back(d.sizeBytes);
            rec.push_back(d.devDataIndex);
            payload += d.sizeBytes;
        }
    }

    mExpected[localType] = payload;
    mDefined[localType]  = true;
    return writeBytes(rec.data(), rec.size());
}

bool FitWriter::emitData(uint8_t localType, const std::vector<uint8_t>& payload)
{
    if (!mBegun || localType > 15 || !mDefined[localType]
        || payload.size() != mExpected[localType]) {
        mOk = false;
        return false;
    }
    std::vector<uint8_t> rec;
    rec.push_back(static_cast<uint8_t>(localType & 0x0Fu));   // normal data header
    rec.insert(rec.end(), payload.begin(), payload.end());
    return writeBytes(rec.data(), rec.size());
}

bool FitWriter::finish()
{
    if (!mOk || !mBegun) {
        return false;
    }
    mFile.flush();

    const size_t end = mFile.getPosition();
    if (end < kHeaderSize) {
        mOk = false;
        return false;
    }
    const uint32_t dataSize = static_cast<uint32_t>(end - kHeaderSize);

    // Back-patch the header in place (data size + header CRC over bytes 0..11).
    uint8_t hdr[14];
    buildHeader(hdr, mProtocolVersion, mProfileVersion, dataSize, /*withCrc=*/true);
    if (!mFile.seek(0) || !writeBytes(hdr, sizeof(hdr))) {
        mOk = false;
        return false;
    }
    mFile.flush();

    // Compute the file CRC over header + data by reading the file back (a
    // write-mode handle cannot read, so reopen read-only), then append it.
    if (!mFile.close() || !mFile.open(/*wMode=*/false)) {
        mOk = false;
        return false;
    }
    uint16_t crc = 0;
    size_t   remaining = end;
    char     buf[256];
    while (remaining > 0) {
        const size_t want = remaining < sizeof(buf) ? remaining : sizeof(buf);
        size_t got = 0;
        if (!mFile.read(buf, want, got) || got == 0) {
            mOk = false;
            return false;
        }
        crc = fitCrcUpdate(crc, buf, got);
        remaining -= got;
    }
    // Reopen for write WITHOUT truncating (override=false must map to a
    // non-truncating open, e.g. FatFs FA_OPEN_ALWAYS — NOT FA_CREATE_ALWAYS),
    // then append the CRC at the end. A truncating backend would zero the
    // activity here.
    if (!mFile.close() || !mFile.open(/*wMode=*/true, /*override=*/false)) {
        mOk = false;
        return false;
    }
    uint8_t crcLE[2] = {static_cast<uint8_t>(crc & 0xFFu),
                        static_cast<uint8_t>((crc >> 8) & 0xFFu)};
    if (!mFile.seek(end) || !writeBytes(crcLE, sizeof(crcLE))) {
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
