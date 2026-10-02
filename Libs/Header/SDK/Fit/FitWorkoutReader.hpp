/**
 ******************************************************************************
 * @file    FitWorkoutReader.hpp
 * @brief   Decodes a FIT workout file into a SDK::Workout::Program.
 *
 * A streaming decoder for one FIT file type, workout (file_id.type 5). It
 * reads the file once, front to back, through a small fixed buffer, and
 * fills a caller-owned Program. It does not allocate.
 *
 * What it reads:
 *   - file_id: type.
 *   - workout: wkt_name, sport, sub_sport, num_valid_steps, wkt_description.
 *   - workout_step: message_index, wkt_step_name, duration type and value,
 *     target type and value, custom target low and high, intensity, notes.
 *   - memo_glob: long text split into parts. A memo for a field replaces the
 *     field's own, possibly truncated, value. Each text's parts must arrive
 *     in order; parts of different texts may interleave. A step's memo must
 *     name its step (parent_index).
 * Every other message and field, and all developer data, is skipped by size.
 * Numeric fields are read only from unsigned base types (enum, uint8/16/32
 * and their z forms); a value in any other type counts as absent. Only the
 * first FIT file in the data is read: anything after its CRC is ignored.
 * Protocol versions above 2.x are rejected as NotFit.
 *
 * What it checks. A file fails, and the Program must not be used, when:
 *   - it is not a FIT file, or its header or file CRC is wrong;
 *   - it is not a workout file, or has no steps;
 *   - a step has no message_index, or one that repeats or is outside
 *     [0, num_valid_steps - 1];
 *   - it has more steps than SDK::Workout::kMaxSteps;
 *   - a repeat does not point to an earlier step, two repeat blocks overlap
 *     without one containing the other, or blocks nest deeper than
 *     SDK::Workout::kMaxRepeatDepth.
 *
 * What it simplifies. A step the watch cannot run as written is marked
 * degraded and still read:
 *   - an unsupported target (cadence, power, a speed zone, ...) becomes Open;
 *   - an unsupported end (calories, heart rate, power, ...) becomes Open;
 *   - an unsupported repeat (until time, distance, ...) runs its block once;
 *   - a repeat count of 0 runs its block once;
 *   - a repeat count above SDK::Workout::kMaxRepeatCount is clamped to it;
 *   - a heart-rate range outside 1-100 % or 1-255 bpm becomes Open.
 *
 * Units follow the FIT profile: durations in ms, distances in cm, speeds
 * in mm/s. A heart-rate custom value of 100 or less is a percentage of
 * maximum heart rate; above 100 it is beats per minute plus 100.
 ******************************************************************************
 */

#ifndef SDK_FIT_WORKOUT_READER_HPP
#define SDK_FIT_WORKOUT_READER_HPP

#include "SDK/Interfaces/IFileSystem.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include <cstddef>
#include <cstdint>

namespace SDK::Fit {

/// Where the reader gets its bytes from.
class IByteSource {
public:
    /// Copy up to @p n bytes into @p dst. Returns the number copied; 0 at the
    /// end of the data or on an error.
    virtual size_t read(uint8_t* dst, size_t n) = 0;

    /// True once a read has failed, as opposed to reaching the end.
    virtual bool failed() const { return false; }

protected:
    ~IByteSource() = default;
};

/// Reads from a buffer in memory.
class MemoryByteSource : public IByteSource {
public:
    MemoryByteSource(const void* data, size_t size)
        : mData(static_cast<const uint8_t*>(data)), mSize(size) {}
    size_t read(uint8_t* dst, size_t n) override;

private:
    const uint8_t* mData;
    size_t         mSize;
    size_t         mPos = 0;
};

/// Reads from a file that is already open for reading.
class FileByteSource : public IByteSource {
public:
    explicit FileByteSource(SDK::Interface::IFile& file) : mFile(file) {}
    size_t read(uint8_t* dst, size_t n) override;
    bool   failed() const override { return mFailed; }

private:
    SDK::Interface::IFile& mFile;
    bool                   mFailed = false;
};

/// Holds about 3 KB of working state (record definitions and buffers), so
/// keep it with the app's other long-lived objects rather than on a task
/// stack. One reader can decode any number of files, one at a time.
class FitWorkoutReader {
public:
    enum class Result : uint8_t {
        Ok,
        Truncated,       ///< The data ended before the file said it would.
        NotFit,          ///< No valid FIT header.
        BadHeaderCrc,
        BadFileCrc,
        Malformed,       ///< A record could not be decoded.
        NotWorkout,      ///< file_id.type is not workout.
        NoSteps,
        TooManySteps,    ///< More than SDK::Workout::kMaxSteps.
        BadStepIndex,    ///< A message_index repeats, is missing or is out of range.
        BadRepeat,       ///< A repeat points forwards or at itself, or blocks overlap.
        NestingTooDeep,  ///< More than SDK::Workout::kMaxRepeatDepth nested blocks.
        ReadError,       ///< The file could not be opened, or a read failed.
    };

    /// Decode the whole of @p src into @p out. @p out is overwritten; on any
    /// result other than Ok its contents are unspecified.
    Result read(IByteSource& src, SDK::Workout::Program& out);

    /// Open @p file for reading, decode it into @p out, and close it.
    Result read(SDK::Interface::IFile& file, SDK::Workout::Program& out);

    /// A short, fixed English name for @p r, for logs.
    static const char* resultName(Result r);

private:
    static constexpr size_t kMaxFieldsRead = 12;  ///< Fields read from one message type.
    static constexpr size_t kLocalTypes    = 16;

    /// One field the reader decodes, and how many bytes to skip before it.
    struct Item {
        uint32_t skipBefore;
        uint8_t  fieldNum;
        uint8_t  size;
        uint8_t  baseType;
    };

    /// A definition record, reduced to the fields the reader decodes.
    struct Definition {
        bool     valid     = false;
        bool     bigEndian = false;
        uint16_t global    = 0;
        uint8_t  itemCount = 0;
        uint32_t tail      = 0;  ///< Bytes after the last decoded field, developer data included.
        Item     items[kMaxFieldsRead];
    };

    /// Values of the message being decoded. Which members are used depends
    /// on the message type.
    struct Pending {
        uint32_t value[9];  ///< By slot; see the .cpp.
        uint16_t present;   ///< Bit per slot.
        char     notes[SDK::Workout::kStepNotesBytes];
        char     name[SDK::Workout::kStepNameBytes];
        uint8_t  memo[256];
        uint16_t memoLen;
        bool     memoFromData;  ///< memo holds the data field, not the older memo field.
    };

    // Buffered input with a running CRC.
    bool fill();
    bool getBytes(uint8_t* dst, size_t n);
    bool skipBytes(size_t n);

    Result decode(SDK::Workout::Program& out);
    bool   readDefinition(uint8_t header);
    bool   readData(uint8_t localType, SDK::Workout::Program& out);
    void   finishMessage(uint16_t global, SDK::Workout::Program& out);
    void   applyMemo(SDK::Workout::Program& out);
    Result validate(SDK::Workout::Program& out);

    IByteSource* mSrc = nullptr;
    uint8_t      mBuf[128];
    size_t       mBufLen = 0, mBufPos = 0;
    uint32_t     mConsumed = 0;  ///< Bytes taken from the source so far.
    uint16_t     mCrc = 0;
    bool         mEof = false;

    Definition mDefs[kLocalTypes];
    Pending    mMsg;
    uint8_t    mScratch[256];

    uint8_t  mFileType = 0xFF;
    bool     mNumValidStepsSet = false;
    uint16_t mNumValidSteps = 0;
    uint16_t mArrived = 0;        ///< workout_step messages seen.
    bool     mTooMany = false, mDuplicate = false, mMissingIndex = false;
    uint8_t  mSeen[(SDK::Workout::kMaxSteps + 7) / 8];

    // memo_glob progress per text: the part expected next, 0 while the text
    // still comes from its own field. Each text joins independently, so
    // parts of different texts may interleave.
    uint32_t mWktNamePart = 0, mWktDescPart = 0;
    uint8_t  mStepNotesPart[SDK::Workout::kMaxSteps];
    uint8_t  mStepNamePart[SDK::Workout::kMaxSteps];
};

}  // namespace SDK::Fit

#endif  // SDK_FIT_WORKOUT_READER_HPP
