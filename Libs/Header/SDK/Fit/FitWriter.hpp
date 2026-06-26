/**
 ******************************************************************************
 * @file    FitWriter.hpp
 * @brief   Native FIT-format encoder engine.
 *
 * Encodes a FIT file into an in-memory buffer -- a 14-byte header, a sequence
 * of definition and data records keyed by local message type, and a trailing
 * little-endian file CRC -- and writes the whole buffer to an IFile on
 * finish(). Buffering keeps the encoder dependent only on IFile::write (no
 * seek/read-back, which write-mode file handles may not support) at the cost
 * of holding the file in RAM; activity files are modest but long recordings
 * are not free, so this trade-off is worth revisiting if RAM becomes tight.
 *
 * The encoder is profile-agnostic -- callers supply global message numbers,
 * field-definition numbers and base types, so the same engine serves every
 * FIT message. Higher layers map message/field names to those numbers.
 *
 * Layout follows the public FIT Protocol specification: definitions carry a
 * reserved byte, a little-endian architecture byte, the global message number,
 * a field count and per-field (number, size, base type) triples, optionally
 * followed by developer-field triples. Data records carry the field values in
 * definition order. Multi-byte values are written little-endian.
 ******************************************************************************
 */

#ifndef __SDK_FIT_WRITER_HPP
#define __SDK_FIT_WRITER_HPP

#include "SDK/Fit/FitBaseType.hpp"
#include "SDK/Interfaces/IFileSystem.hpp"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace SDK::Fit {

/// FIT protocol version 2.0 encoded as a single byte (major<<4 | minor).
constexpr uint8_t kProtocolVersion20 = 0x20;

class FitWriter {
public:
    /// A field of a (native) FIT message: its definition number, base type,
    /// and element count (>1 for arrays / multi-element fields).
    struct Field {
        uint8_t  fieldDefNum;
        BaseType baseType;
        uint8_t  count = 1;
    };

    /// A developer field reference within a definition: developer field number,
    /// its size in bytes, and the developer-data index it belongs to.
    struct DevField {
        uint8_t fieldNum;
        uint8_t sizeBytes;
        uint8_t devDataIndex;
    };

    /// Builds and writes a single data message for a defined local type.
    /// Values are appended in definition order; multi-byte values are stored
    /// little-endian. Returns from write() reflect IO + size-match success.
    class Data {
    public:
        Data& u8(uint8_t v);
        Data& i8(int8_t v);
        Data& u16(uint16_t v);
        Data& i16(int16_t v);
        Data& u32(uint32_t v);
        Data& i32(int32_t v);
        Data& u64(uint64_t v);
        Data& i64(int64_t v);
        Data& f32(float v);
        Data& f64(double v);
        /// Append a null-padded string occupying exactly `fieldBytes` bytes.
        Data& str(const char* s, uint8_t fieldBytes);
        /// Append raw bytes (e.g. a byte array field).
        Data& bytes(const void* p, size_t n);
        /// Append the canonical invalid sentinel for a field that is declared
        /// but carries no value (`count` elements).
        Data& invalid(BaseType t, uint8_t count = 1);

        /// Emit the data record (header + payload). Fails if the payload size
        /// does not match the active definition for this local type.
        bool write();

    private:
        friend class FitWriter;
        Data(FitWriter& w, uint8_t localType) : mWriter(w), mLocalType(localType) {}
        FitWriter&           mWriter;
        uint8_t              mLocalType;
        std::vector<uint8_t> mPayload;
    };

    explicit FitWriter(SDK::Interface::IFile& file) : mFile(file) {}

    /// Write the file header placeholder. The header CRC is set to 0x0000
    /// (permitted by the spec) and the data size is back-patched by finish().
    bool begin(uint16_t profileVersion, uint8_t protocolVersion = kProtocolVersion20);

    /// Emit a definition record associating `localType` (0-15) with a global
    /// message number and an ordered field list, optionally with developer
    /// fields (which set the developer-data flag in the record header).
    bool defineMessage(uint8_t localType, uint16_t globalMesgNum,
                       std::initializer_list<Field>    fields,
                       std::initializer_list<DevField> devFields = {});

    /// Start a data message for a previously defined local type.
    Data data(uint8_t localType) { return Data(*this, localType); }

    /// Back-patch the header data size and append the trailing file CRC.
    bool finish();

    bool ok() const { return mOk; }

private:
    bool emitData(uint8_t localType, const std::vector<uint8_t>& payload);

    SDK::Interface::IFile& mFile;
    std::vector<uint8_t>   mBuf;                  ///< full file (header + records)
    bool                   mOk            = true;
    bool                   mBegun         = false;
    size_t                 mExpected[16]  = {};   ///< payload size per local type
    bool                   mDefined[16]   = {};
};

}  // namespace SDK::Fit

#endif  // __SDK_FIT_WRITER_HPP
