/**
 ******************************************************************************
 * @file    FitWriter.hpp
 * @brief   Native FIT-format encoder engine.
 *
 * Streams a FIT file to an IFile -- a 14-byte header, a sequence of definition
 * and data records keyed by local message type, and a trailing little-endian
 * file CRC. Records are written as they are produced (constant RAM, and the
 * activity survives on disk if recording is interrupted). begin() writes a
 * header placeholder; finish() back-patches the data size + header CRC and
 * appends the file CRC, computed by reopening the file read-only -- the same
 * approach the platform's file API supports for write-mode handles.
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

    /// Recover a FIT file that was streamed but never finish()ed (e.g. power
    /// loss mid-activity), turning it into a complete, CRC-valid file.
    ///
    /// Opens @p file read/write WITHOUT truncation, so a torn file is never
    /// destroyed. If the on-disk header already describes a finalized file
    /// (non-zero data-size field and a total size of 14 + dataSize + 2), the
    /// file is already good and is left untouched (idempotent no-op -> true).
    /// Otherwise it is finalized: the header data size + header CRC are
    /// back-patched, the file CRC is recomputed by reading the bytes back, the
    /// CRC is appended, and any torn/garbage tail past @p dataEnd is trimmed.
    ///
    /// @p dataEnd is the byte offset one-past the last complete, flushed data
    /// record. It is supplied by the caller (recover() does not parse FIT
    /// records -- it trusts @p dataEnd as a record boundary) and must satisfy
    /// 14 <= dataEnd <= file.size(). Returns false WITHOUT modifying the file on
    /// a bad/non-FIT header or an out-of-range @p dataEnd.
    ///
    /// On success @p file is left OPEN in write mode (mirroring finish(), whose
    /// caller owns and closes the writer's file); the caller must close() it --
    /// destroying the handle does not. On failure the file is closed.
    static bool recover(SDK::Interface::IFile& file, uint32_t dataEnd);

private:
    /// Shared finalization tail used by both finish() and recover(): read the
    /// on-disk 14-byte header (so protocol/profile survive a reboot), validate
    /// it, back-patch the data size + header CRC, compute the file CRC over
    /// [0, dataEnd) by reading the bytes back, append it little-endian at
    /// dataEnd, and truncate to dataEnd + 2 (dropping any torn tail). Manages
    /// the file's open mode internally (write handles cannot read). Returns
    /// false WITHOUT corrupting the file on a bad header or out-of-range
    /// dataEnd. Requires 14 <= dataEnd <= file.size().
    static bool finalize(SDK::Interface::IFile& file, uint32_t dataEnd);

    bool emitData(uint8_t localType, const std::vector<uint8_t>& payload);
    bool writeBytes(const void* p, size_t n);

    SDK::Interface::IFile& mFile;
    uint8_t                mProtocolVersion = 0;
    uint16_t               mProfileVersion  = 0;
    bool                   mOk            = true;
    bool                   mBegun         = false;
    size_t                 mExpected[16]  = {};   ///< payload size per local type
    bool                   mDefined[16]   = {};
};

}  // namespace SDK::Fit

#endif  // __SDK_FIT_WRITER_HPP
