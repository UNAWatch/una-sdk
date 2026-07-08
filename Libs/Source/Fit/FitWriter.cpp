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

namespace {
    // Read exactly `n` bytes from a read-mode handle into `dst`. Returns false
    // on any short read / IO error.
    bool readExact(SDK::Interface::IFile& file, void* dst, size_t n)
    {
        auto*  p         = static_cast<char*>(dst);
        size_t remaining = n;
        while (remaining > 0) {
            size_t got = 0;
            if (!file.read(p, remaining, got) || got == 0) {
                return false;
            }
            p         += got;
            remaining -= got;
        }
        return true;
    }
}  // namespace

bool FitWriter::finalize(SDK::Interface::IFile& file, uint32_t dataEnd)
{
    // --- Read the on-disk header (read mode) --------------------------------
    // Protocol/profile MUST come from the file, not writer member state, so a
    // reboot-time recover() reconstructs the correct header. A write-mode
    // handle cannot read, so ensure we are in read mode first.
    file.close();  // start from a known-closed handle (no-op if already closed)
    if (!file.open(/*wMode=*/false)) {
        return false;
    }
    uint8_t hdr[kHeaderSize];
    if (!readExact(file, hdr, sizeof(hdr))) {
        file.close();
        return false;
    }
    const size_t fileSize = file.size();

    // Validate: header size byte + ".FIT" signature.
    if (hdr[0] != kHeaderSize || hdr[8] != '.' || hdr[9] != 'F'
        || hdr[10] != 'I' || hdr[11] != 'T') {
        file.close();
        return false;
    }
    // Guard: the claimed data range must lie within the file.
    if (dataEnd < kHeaderSize || dataEnd > fileSize) {
        file.close();
        return false;
    }
    const uint8_t  protocolVersion = hdr[1];
    const uint16_t profileVersion =
        static_cast<uint16_t>(hdr[2] | (static_cast<uint16_t>(hdr[3]) << 8));

    // --- Back-patch the header (write mode, non-truncating) -----------------
    // override=false must map to a non-truncating open (e.g. FatFs
    // FA_OPEN_ALWAYS — NOT FA_CREATE_ALWAYS); a truncating backend would zero
    // the activity here.
    if (!file.close() || !file.open(/*wMode=*/true, /*override=*/false)) {
        return false;
    }
    uint8_t patched[kHeaderSize];
    buildHeader(patched, protocolVersion, profileVersion,
                dataEnd - kHeaderSize, /*withCrc=*/true);
    size_t bw = 0;
    if (!file.seek(0)
        || !file.write(reinterpret_cast<const char*>(patched), sizeof(patched), bw)
        || bw != sizeof(patched)) {
        file.close();
        return false;
    }
    file.flush();

    // --- Compute the file CRC over [0, dataEnd) (read mode) -----------------
    if (!file.close() || !file.open(/*wMode=*/false)) {
        return false;
    }
    uint16_t crc       = 0;
    size_t   remaining = dataEnd;
    char     buf[256];
    while (remaining > 0) {
        const size_t want = remaining < sizeof(buf) ? remaining : sizeof(buf);
        size_t       got  = 0;
        if (!file.read(buf, want, got) || got == 0) {
            file.close();
            return false;
        }
        crc = fitCrcUpdate(crc, buf, got);
        remaining -= got;
    }

    // --- Append the CRC + trim any torn tail (write mode) -------------------
    if (!file.close() || !file.open(/*wMode=*/true, /*override=*/false)) {
        return false;
    }
    uint8_t crcLE[2] = {static_cast<uint8_t>(crc & 0xFFu),
                        static_cast<uint8_t>((crc >> 8) & 0xFFu)};
    bw = 0;
    if (!file.seek(dataEnd)
        || !file.write(reinterpret_cast<const char*>(crcLE), sizeof(crcLE), bw)
        || bw != sizeof(crcLE)) {
        file.close();
        return false;
    }
    // Drop any partially-written record beyond the recovered data + CRC.
    if (!file.truncate(dataEnd + 2)) {
        file.close();
        return false;
    }
    file.flush();
    return true;
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
    // getPosition() after the final record is the current data-end offset.
    if (!finalize(mFile, static_cast<uint32_t>(end))) {
        mOk = false;
        return false;
    }
    return mOk;
}

bool FitWriter::recover(SDK::Interface::IFile& file, uint32_t dataEnd)
{
    // Inspect the file READ-ONLY first. A non-truncating WRITE open (wMode=true,
    // override=false) maps to FatFs FA_OPEN_ALWAYS, which CREATES a missing path
    // -- leaving a stray 0-byte .fit and violating the public "does not modify
    // the file on bad input" contract. A read open never creates or writes, so a
    // missing/too-short/non-FIT path is rejected untouched. We only open for
    // write inside finalize().
    if (!file.open(/*wMode=*/false)) {
        return false;
    }
    const size_t fileSize = file.size();

    // Read + validate the on-disk header. A missing/too-short/non-FIT path is
    // rejected here, untouched.
    uint8_t hdr[kHeaderSize];
    if (fileSize < kHeaderSize || !readExact(file, hdr, sizeof(hdr))
        || hdr[0] != kHeaderSize || hdr[8] != '.' || hdr[9] != 'F'
        || hdr[10] != 'I' || hdr[11] != 'T') {
        file.close();
        return false;
    }
    const uint32_t hdrDataSize =
        static_cast<uint32_t>(hdr[4]) | (static_cast<uint32_t>(hdr[5]) << 8)
        | (static_cast<uint32_t>(hdr[6]) << 16)
        | (static_cast<uint32_t>(hdr[7]) << 24);
    file.close();

    // Derive the effective data-end and ALWAYS re-run the (idempotent)
    // finalize(). We must NOT early-return "already finalized" on dataSize != 0:
    // finish()/finalize() flush the header (with dataSize) BEFORE writing and
    // flushing the trailing CRC and truncating the tail. A crash in that window
    // leaves dataSize != 0 but a missing/torn CRC -- skipping repair would leave
    // an invalid FIT on disk forever. Re-finalizing is harmless because
    // finalize() recomputes the same header/CRC and truncates to the same size.
    //
    //  - hdrDataSize != 0 (and it fits within the file): trust the header's own
    //    dataSize -- finish()/a prior finalize got far enough to write it, so
    //    this is the DURABLE data end, not the possibly-stale marker offset.
    //    Finalizing here writes the correct CRC (crash-mid-finalize) and trims
    //    any extra tail (isolated truncate() failure) WITHOUT discarding the
    //    session/activity messages finish() appended past the marker.
    //  - otherwise: use the caller/marker offset -- the genuine crash-mid-
    //    recording case where dataSize is still 0, or a fallback if the header
    //    over-claims vs the actual file size.
    uint32_t effectiveEnd;
    if (hdrDataSize != 0
        && static_cast<uint64_t>(kHeaderSize) + hdrDataSize <= fileSize) {
        effectiveEnd = kHeaderSize + hdrDataSize;
    } else {
        effectiveEnd = dataEnd;
    }

    // Guard: the effective data range must lie within the file.
    if (effectiveEnd < kHeaderSize || effectiveEnd > fileSize) {
        return false;
    }

    return finalize(file, effectiveEnd);
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
