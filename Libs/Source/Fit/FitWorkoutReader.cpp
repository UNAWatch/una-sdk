/**
 ******************************************************************************
 * @file    FitWorkoutReader.cpp
 * @brief   Streaming decoder for FIT workout files.
 ******************************************************************************
 */

#include "SDK/Fit/FitWorkoutReader.hpp"

#include "SDK/Fit/FitCrc.hpp"
#include "SDK/Fit/FitProfile.hpp"

#include <cstring>

namespace SDK::Fit {

using SDK::Workout::Program;
using SDK::Workout::Step;

size_t MemoryByteSource::read(uint8_t* dst, size_t n)
{
    const size_t left = mSize - mPos;
    const size_t k    = n < left ? n : left;
    std::memcpy(dst, mData + mPos, k);
    mPos += k;
    return k;
}

size_t FileByteSource::read(uint8_t* dst, size_t n)
{
    size_t br = 0;
    if (!mFile.read(reinterpret_cast<char*>(dst), n, br)) {
        mFailed = true;
        return 0;
    }
    return br;
}

namespace {

// Slots in Pending::value, per message type.
namespace FileIdSlot { constexpr uint8_t Type = 0; }
namespace WorkoutSlot { constexpr uint8_t Sport = 0, NumValidSteps = 1, SubSport = 2; }
namespace StepSlot {
    constexpr uint8_t Index = 0, DurationType = 1, DurationValue = 2, TargetType = 3,
                      TargetValue = 4, CustomLow = 5, CustomHigh = 6, Intensity = 7;
}
namespace MemoSlot { constexpr uint8_t Part = 0, Mesg = 1, Parent = 2, Field = 3; }

// String fields are not slots; they are copied out as they are read.
constexpr int8_t kString = -1;
constexpr int8_t kNotRead = -2;

// Which slot a field fills, kString for a text field, or kNotRead.
int8_t slotFor(uint16_t global, uint8_t field)
{
    switch (global) {
    case mesgNum(MesgNum::FileId):
        return field == field::FileId::Type.fieldDefNum ? FileIdSlot::Type : kNotRead;
    case mesgNum(MesgNum::Workout):
        if (field == field::Workout::Sport.fieldDefNum)         return WorkoutSlot::Sport;
        if (field == field::Workout::NumValidSteps.fieldDefNum) return WorkoutSlot::NumValidSteps;
        if (field == field::Workout::SubSport.fieldDefNum)      return WorkoutSlot::SubSport;
        if (field == field::Workout::kWktNameNum ||
            field == field::Workout::kWktDescriptionNum)        return kString;
        return kNotRead;
    case mesgNum(MesgNum::WorkoutStep):
        if (field == field::WorkoutStep::MessageIndex.fieldDefNum)          return StepSlot::Index;
        if (field == field::WorkoutStep::DurationType.fieldDefNum)          return StepSlot::DurationType;
        if (field == field::WorkoutStep::DurationValue.fieldDefNum)         return StepSlot::DurationValue;
        if (field == field::WorkoutStep::TargetType.fieldDefNum)            return StepSlot::TargetType;
        if (field == field::WorkoutStep::TargetValue.fieldDefNum)           return StepSlot::TargetValue;
        if (field == field::WorkoutStep::CustomTargetValueLow.fieldDefNum)  return StepSlot::CustomLow;
        if (field == field::WorkoutStep::CustomTargetValueHigh.fieldDefNum) return StepSlot::CustomHigh;
        if (field == field::WorkoutStep::Intensity.fieldDefNum)             return StepSlot::Intensity;
        if (field == field::WorkoutStep::kWktStepNameNum ||
            field == field::WorkoutStep::kNotesNum)                         return kString;
        return kNotRead;
    case mesgNum(MesgNum::MemoGlob):
        if (field == field::MemoGlob::PartIndex.fieldDefNum)   return MemoSlot::Part;
        if (field == field::MemoGlob::MesgNum.fieldDefNum)     return MemoSlot::Mesg;
        if (field == field::MemoGlob::ParentIndex.fieldDefNum) return MemoSlot::Parent;
        if (field == field::MemoGlob::FieldNum.fieldDefNum)    return MemoSlot::Field;
        if (field == field::MemoGlob::kDataNum ||
            field == field::MemoGlob::kMemoNum)                return kString;
        return kNotRead;
    default:
        return kNotRead;
    }
}

// Element size of a base type, from its identifier byte; 0 if unknown.
uint8_t baseSize(uint8_t id)
{
    switch (id & 0x1F) {
    case 0: case 1: case 2: case 7: case 10: case 13: return 1;
    case 3: case 4: case 11:                          return 2;
    case 5: case 6: case 8: case 12:                  return 4;
    case 9: case 14: case 15: case 16:                return 8;
    default:                                          return 0;
    }
}

// True for the base types a numeric field may use: enum, uint8/16/32 and
// their z forms.
bool isUnsigned(uint8_t baseType)
{
    switch (baseType & 0x1F) {
    case 0: case 2: case 4: case 6: case 10: case 11: case 12: return true;
    default:                                                   return false;
    }
}

// Decode a 1-, 2- or 4-byte unsigned value. False if the field holds its
// type's invalid value, is not an unsigned type, or is not a single value.
bool decodeUnsigned(const uint8_t* p, uint8_t size, uint8_t baseType, bool bigEndian,
                    uint32_t& out)
{
    if (!isUnsigned(baseType) || baseSize(baseType) != size) {
        return false;
    }
    uint32_t v = 0;
    for (uint8_t i = 0; i < size; ++i) {
        const uint8_t b = bigEndian ? p[i] : p[size - 1 - i];
        v = (v << 8) | b;
    }
    const uint8_t num   = baseType & 0x1F;
    const bool    zType = (num == 10 || num == 11 || num == 12);
    const uint32_t allOnes = size == 4 ? 0xFFFFFFFFu : ((1u << (8 * size)) - 1u);
    if (zType ? v == 0 : v == allOnes) {
        return false;
    }
    out = v;
    return true;
}

// Length of the text in a fixed string field: up to the first NUL.
size_t textLen(const uint8_t* p, size_t n)
{
    size_t k = 0;
    while (k < n && p[k] != 0) {
        ++k;
    }
    return k;
}

// Length of @p s[0, len) without a trailing UTF-8 character that is not
// complete. The cut may fall in text appended earlier, when a character was
// split across two memo parts.
size_t dropIncompleteTail(const char* s, size_t len)
{
    size_t lead = len;
    while (lead > 0 && (static_cast<uint8_t>(s[lead - 1]) & 0xC0) == 0x80) {
        --lead;
    }
    if (lead == 0) {
        return len;  // only continuation bytes: not UTF-8 we can repair
    }
    const uint8_t b    = static_cast<uint8_t>(s[lead - 1]);
    const size_t  need = b >= 0xF0 ? 4 : b >= 0xE0 ? 3 : b >= 0xC0 ? 2 : 1;
    return (len - (lead - 1) < need) ? lead - 1 : len;
}

// Append @p n bytes to the NUL-terminated text in @p dst (capacity @p cap).
// When the text does not fit it is cut, and a character the cut would split
// is dropped whole. Returns the new length.
size_t appendText(char* dst, size_t cap, size_t len, const uint8_t* src, size_t n)
{
    if (cap == 0) {
        return 0;
    }
    const size_t room = len + 1 < cap ? cap - 1 - len : 0;
    const size_t k    = n < room ? n : room;
    std::memcpy(dst + len, src, k);
    len += k;
    if (k < n) {
        len = dropIncompleteTail(dst, len);
    }
    dst[len] = '\0';
    return len;
}

void setText(char* dst, size_t cap, const uint8_t* src, size_t n)
{
    dst[0] = '\0';
    appendText(dst, cap, 0, src, n);
}

bool bitGet(const uint8_t* bits, size_t i) { return (bits[i / 8] >> (i % 8)) & 1u; }
void bitSet(uint8_t* bits, size_t i)       { bits[i / 8] |= static_cast<uint8_t>(1u << (i % 8)); }

constexpr uint32_t kInvalid32 = 0xFFFFFFFFu;
constexpr uint8_t  kInvalid8  = 0xFF;

}  // namespace

// --- Input -----------------------------------------------------------------

bool FitWorkoutReader::fill()
{
    if (mEof) {
        return false;
    }
    mBufLen = mSrc->read(mBuf, sizeof(mBuf));
    mBufPos = 0;
    if (mBufLen == 0) {
        mEof = true;
        return false;
    }
    return true;
}

bool FitWorkoutReader::getBytes(uint8_t* dst, size_t n)
{
    while (n > 0) {
        if (mBufPos == mBufLen && !fill()) {
            return false;
        }
        size_t k = mBufLen - mBufPos;
        if (k > n) {
            k = n;
        }
        mCrc = fitCrcUpdate(mCrc, mBuf + mBufPos, k);
        if (dst != nullptr) {
            std::memcpy(dst, mBuf + mBufPos, k);
            dst += k;
        }
        mBufPos += k;
        mConsumed += static_cast<uint32_t>(k);
        n -= k;
    }
    return true;
}

bool FitWorkoutReader::skipBytes(size_t n)
{
    return getBytes(nullptr, n);
}

// --- Decoding --------------------------------------------------------------

FitWorkoutReader::Result FitWorkoutReader::read(SDK::Interface::IFile& file, Program& out)
{
    if (!file.open(false, false)) {
        return Result::ReadError;
    }
    FileByteSource src(file);
    const Result r = read(src, out);
    file.close();
    return r;
}

FitWorkoutReader::Result FitWorkoutReader::read(IByteSource& src, Program& out)
{
    mSrc = &src;
    mBufLen = mBufPos = 0;
    mConsumed = 0;
    mCrc = 0;
    mEof = false;
    for (Definition& d : mDefs) {
        d.valid = false;
    }
    mFileType = kInvalid8;
    mNumValidStepsSet = false;
    mNumValidSteps = 0;
    mArrived = 0;
    mTooMany = mDuplicate = mMissingIndex = false;
    std::memset(mSeen, 0, sizeof(mSeen));
    mWktNamePart = mWktDescPart = 0;
    std::memset(mStepNotesPart, 0, sizeof(mStepNotesPart));
    std::memset(mStepNamePart, 0, sizeof(mStepNamePart));
    out = Program{};

    const Result r = decode(out);
    const bool readFailed = mSrc->failed();
    mSrc = nullptr;
    if (r != Result::Ok) {
        return readFailed ? Result::ReadError : r;
    }
    return validate(out);
}

FitWorkoutReader::Result FitWorkoutReader::decode(Program& out)
{
    // Header: size, protocol, profile (2), data size (4, little-endian),
    // ".FIT", then an optional header CRC.
    uint8_t hdr[14] = {};
    if (!getBytes(hdr, 1)) {
        return Result::NotFit;
    }
    const uint8_t headerSize = hdr[0];
    if (headerSize < 12) {
        return Result::NotFit;
    }
    if (!getBytes(hdr + 1, (headerSize < 14 ? headerSize : 14) - 1)) {
        return Result::NotFit;
    }
    // A protocol major version above 2 may change the record layout, so it
    // is not decoded.
    if (std::memcmp(hdr + 8, ".FIT", 4) != 0 || (hdr[1] >> 4) > 2) {
        return Result::NotFit;
    }
    if (headerSize >= 14) {
        const uint16_t want = static_cast<uint16_t>(hdr[12] | (hdr[13] << 8));
        if (want != 0 && want != fitCrcUpdate(0, hdr, 12)) {
            return Result::BadHeaderCrc;
        }
        if (headerSize > 14 && !skipBytes(headerSize - 14u)) {
            return Result::NotFit;
        }
    }
    const uint32_t dataSize = static_cast<uint32_t>(hdr[4]) | (static_cast<uint32_t>(hdr[5]) << 8) |
                              (static_cast<uint32_t>(hdr[6]) << 16) |
                              (static_cast<uint32_t>(hdr[7]) << 24);
    if (dataSize > UINT32_MAX - headerSize) {
        return Result::NotFit;
    }
    // A data size of 0 is taken literally (no records), never as "unknown".
    const uint32_t dataEnd = headerSize + dataSize;

    while (mConsumed < dataEnd) {
        uint8_t h = 0;
        if (!getBytes(&h, 1)) {
            return Result::Truncated;
        }
        bool ok;
        if (h & 0x80) {
            // Compressed timestamp header: a data message for local type 0-3.
            ok = readData(static_cast<uint8_t>((h >> 5) & 0x03), out);
        } else if (h & 0x40) {
            ok = readDefinition(h);
        } else {
            ok = readData(h & 0x0F, out);
        }
        if (!ok) {
            return mEof ? Result::Truncated : Result::Malformed;
        }
        if (mConsumed > dataEnd) {
            return Result::Malformed;
        }
    }

    const uint16_t crc = mCrc;
    uint8_t trailer[2];
    if (!getBytes(trailer, 2)) {
        return Result::Truncated;
    }
    if (static_cast<uint16_t>(trailer[0] | (trailer[1] << 8)) != crc) {
        return Result::BadFileCrc;
    }
    return Result::Ok;
}

bool FitWorkoutReader::readDefinition(uint8_t header)
{
    uint8_t b[5];
    if (!getBytes(b, 5)) {
        return false;
    }
    Definition& d = mDefs[header & 0x0F];
    d.valid     = false;
    d.bigEndian = b[1] != 0;
    d.global    = d.bigEndian ? static_cast<uint16_t>((b[2] << 8) | b[3])
                              : static_cast<uint16_t>(b[2] | (b[3] << 8));
    d.itemCount = 0;

    uint32_t skip = 0;
    for (uint8_t i = 0; i < b[4]; ++i) {
        uint8_t f[3];
        if (!getBytes(f, 3)) {
            return false;
        }
        if (slotFor(d.global, f[0]) != kNotRead && d.itemCount < kMaxFieldsRead && f[1] > 0) {
            d.items[d.itemCount++] = Item{skip, f[0], f[1], f[2]};
            skip = 0;
        } else {
            skip += f[1];
        }
    }
    if (header & 0x20) {
        uint8_t n = 0;
        if (!getBytes(&n, 1)) {
            return false;
        }
        for (uint8_t i = 0; i < n; ++i) {
            uint8_t f[3];
            if (!getBytes(f, 3)) {
                return false;
            }
            skip += f[1];
        }
    }
    d.tail  = skip;
    d.valid = true;
    return true;
}

bool FitWorkoutReader::readData(uint8_t localType, Program& out)
{
    const Definition& d = mDefs[localType];
    if (!d.valid) {
        return false;
    }
    std::memset(&mMsg, 0, sizeof(mMsg));

    for (uint8_t i = 0; i < d.itemCount; ++i) {
        const Item& it = d.items[i];
        if (!skipBytes(it.skipBefore) || !getBytes(mScratch, it.size)) {
            return false;
        }
        const int8_t slot = slotFor(d.global, it.fieldNum);
        if (slot >= 0) {
            uint32_t v = 0;
            if (decodeUnsigned(mScratch, it.size, it.baseType, d.bigEndian, v)) {
                mMsg.value[slot] = v;
                mMsg.present |= static_cast<uint16_t>(1u << slot);
            }
            continue;
        }
        // Text fields.
        const size_t n = textLen(mScratch, it.size);
        switch (d.global) {
        case mesgNum(MesgNum::Workout):
            if (it.fieldNum == field::Workout::kWktNameNum && mWktNamePart == 0) {
                setText(out.name, sizeof(out.name), mScratch, n);
            } else if (it.fieldNum == field::Workout::kWktDescriptionNum && mWktDescPart == 0) {
                setText(out.description, sizeof(out.description), mScratch, n);
            }
            break;
        case mesgNum(MesgNum::WorkoutStep):
            if (it.fieldNum == field::WorkoutStep::kNotesNum) {
                setText(mMsg.notes, sizeof(mMsg.notes), mScratch, n);
            } else {
                setText(mMsg.name, sizeof(mMsg.name), mScratch, n);
            }
            break;
        case mesgNum(MesgNum::MemoGlob):
            // The data field wins over the older memo field, in either order.
            if (it.fieldNum == field::MemoGlob::kDataNum || !mMsg.memoFromData) {
                std::memcpy(mMsg.memo, mScratch, n);
                mMsg.memoLen      = static_cast<uint16_t>(n);
                mMsg.memoFromData = it.fieldNum == field::MemoGlob::kDataNum;
            }
            break;
        default:
            break;
        }
    }
    if (!skipBytes(d.tail)) {
        return false;
    }
    finishMessage(d.global, out);
    return true;
}

void FitWorkoutReader::finishMessage(uint16_t global, Program& out)
{
    const auto has = [this](uint8_t slot) { return (mMsg.present >> slot) & 1u; };

    switch (global) {
    case mesgNum(MesgNum::FileId):
        if (has(FileIdSlot::Type)) {
            mFileType = static_cast<uint8_t>(mMsg.value[FileIdSlot::Type]);
        }
        break;

    case mesgNum(MesgNum::Workout):
        if (has(WorkoutSlot::Sport)) {
            out.sport = static_cast<uint8_t>(mMsg.value[WorkoutSlot::Sport]);
        }
        if (has(WorkoutSlot::SubSport)) {
            out.subSport = static_cast<uint8_t>(mMsg.value[WorkoutSlot::SubSport]);
        }
        if (has(WorkoutSlot::NumValidSteps)) {
            mNumValidStepsSet = true;
            mNumValidSteps = static_cast<uint16_t>(mMsg.value[WorkoutSlot::NumValidSteps]);
        }
        break;

    case mesgNum(MesgNum::WorkoutStep): {
        ++mArrived;
        // Repeats name their target step by message_index, so a step
        // without one cannot be placed.
        if (!has(StepSlot::Index)) {
            mMissingIndex = true;
            break;
        }
        const uint32_t index = mMsg.value[StepSlot::Index];
        if (index >= SDK::Workout::kMaxSteps) {
            mTooMany = true;
            break;
        }
        if (bitGet(mSeen, index)) {
            mDuplicate = true;
            break;
        }
        bitSet(mSeen, index);
        Step::Raw& raw = out.steps[index].raw;
        if (has(StepSlot::DurationType))  raw.durationType  = static_cast<uint8_t>(mMsg.value[StepSlot::DurationType]);
        if (has(StepSlot::DurationValue)) raw.durationValue = mMsg.value[StepSlot::DurationValue];
        if (has(StepSlot::TargetType))    raw.targetType    = static_cast<uint8_t>(mMsg.value[StepSlot::TargetType]);
        if (has(StepSlot::TargetValue))   raw.targetValue   = mMsg.value[StepSlot::TargetValue];
        if (has(StepSlot::CustomLow))     raw.customLow     = mMsg.value[StepSlot::CustomLow];
        if (has(StepSlot::CustomHigh))    raw.customHigh    = mMsg.value[StepSlot::CustomHigh];
        if (has(StepSlot::Intensity))     raw.intensity     = static_cast<uint8_t>(mMsg.value[StepSlot::Intensity]);
        if (mStepNotesPart[index] == 0) {
            std::memcpy(out.steps[index].notes, mMsg.notes, sizeof(mMsg.notes));
        }
        if (mStepNamePart[index] == 0) {
            std::memcpy(out.steps[index].name, mMsg.name, sizeof(mMsg.name));
        }
        break;
    }

    case mesgNum(MesgNum::MemoGlob):
        if (has(MemoSlot::Part) && has(MemoSlot::Mesg) && has(MemoSlot::Field)) {
            applyMemo(out);
        }
        break;

    default:
        break;
    }
}

void FitWorkoutReader::applyMemo(Program& out)
{
    const uint32_t part      = mMsg.value[MemoSlot::Part];
    const uint16_t mesg      = static_cast<uint16_t>(mMsg.value[MemoSlot::Mesg]);
    const bool     hasParent = (mMsg.present >> MemoSlot::Parent) & 1u;
    const uint16_t parent    = hasParent ? static_cast<uint16_t>(mMsg.value[MemoSlot::Parent]) : 0;
    const uint8_t  fld       = static_cast<uint8_t>(mMsg.value[MemoSlot::Field]);

    // Where the text goes, and its part counter. A workout file has one
    // workout, so its memos need no parent; a step's memo must name its step.
    char*     dst         = nullptr;
    size_t    cap         = 0;
    uint32_t* wktCounter  = nullptr;
    uint8_t*  stepCounter = nullptr;
    if (mesg == mesgNum(MesgNum::Workout) && fld == field::Workout::kWktNameNum) {
        dst = out.name; cap = sizeof(out.name); wktCounter = &mWktNamePart;
    } else if (mesg == mesgNum(MesgNum::Workout) && fld == field::Workout::kWktDescriptionNum) {
        dst = out.description; cap = sizeof(out.description); wktCounter = &mWktDescPart;
    } else if (mesg == mesgNum(MesgNum::WorkoutStep) && hasParent &&
               parent < SDK::Workout::kMaxSteps) {
        if (fld == field::WorkoutStep::kNotesNum) {
            dst = out.steps[parent].notes; cap = sizeof(out.steps[parent].notes);
            stepCounter = &mStepNotesPart[parent];
        } else if (fld == field::WorkoutStep::kWktStepNameNum) {
            dst = out.steps[parent].name; cap = sizeof(out.steps[parent].name);
            stepCounter = &mStepNamePart[parent];
        }
    }
    if (dst == nullptr) {
        return;
    }
    const uint32_t next = wktCounter ? *wktCounter : *stepCounter;

    if (part == 0) {
        // The text starts again from its first part, replacing the field's
        // own value.
        dst[0] = '\0';
    } else if (next == 0 || part != next) {
        // Out of order, or without its first part: ignore it.
        return;
    }
    const uint32_t following = part + 1;
    if (wktCounter) {
        *wktCounter = following;
    } else {
        *stepCounter = static_cast<uint8_t>(following < 0xFF ? following : 0xFF);
    }
    appendText(dst, cap, std::strlen(dst), mMsg.memo, mMsg.memoLen);
}

// --- Checking and interpreting ---------------------------------------------

FitWorkoutReader::Result FitWorkoutReader::validate(Program& out)
{
    using namespace SDK::Workout;

    if (mFileType != static_cast<uint8_t>(File::Workout)) {
        return Result::NotWorkout;
    }
    if (mNumValidStepsSet && mNumValidSteps > kMaxSteps) {
        return Result::TooManySteps;
    }
    if (mTooMany) {
        // An index past kMaxSteps: too many steps if the workout says it has
        // that many (or doesn't say), otherwise just a bad index.
        return mNumValidStepsSet ? Result::BadStepIndex : Result::TooManySteps;
    }
    if (mArrived == 0) {
        return Result::NoSteps;
    }
    if (mDuplicate || mMissingIndex) {
        return Result::BadStepIndex;
    }
    // Every index 0..count-1 present exactly once, and count matching the
    // workout's num_valid_steps when it gives one.
    const uint16_t count = mArrived;
    if (mNumValidStepsSet && mNumValidSteps != count) {
        return Result::BadStepIndex;
    }
    for (uint16_t i = 0; i < count; ++i) {
        if (!bitGet(mSeen, i)) {
            return Result::BadStepIndex;
        }
    }
    out.stepCount = count;
    // A memo can name a step the file never sends; leave nothing beyond the
    // last step.
    for (uint16_t i = count; i < kMaxSteps; ++i) {
        out.steps[i] = Step{};
    }

    // Interpret each step.
    for (uint16_t i = 0; i < count; ++i) {
        Step& s = out.steps[i];
        const Step::Raw& r = s.raw;

        // Both namespaces have an Intensity: the file's (SDK::Fit, which also
        // has Recovery, Interval and Other) and the program's.
        using WI = SDK::Workout::Intensity;
        switch (r.intensity) {
        case static_cast<uint8_t>(Fit::Intensity::Rest):
        case static_cast<uint8_t>(Fit::Intensity::Recovery): s.intensity = WI::Rest;     break;
        case static_cast<uint8_t>(Fit::Intensity::Warmup):   s.intensity = WI::Warmup;   break;
        case static_cast<uint8_t>(Fit::Intensity::Cooldown): s.intensity = WI::Cooldown; break;
        default:                                             s.intensity = WI::Active;   break;
        }

        const uint8_t dt = r.durationType;
        if (dt == static_cast<uint8_t>(WktStepDuration::RepeatUntilStepsComplete) ||
            (dt >= static_cast<uint8_t>(WktStepDuration::RepeatUntilTime) &&
             dt <= static_cast<uint8_t>(WktStepDuration::RepeatUntilPowerGreaterThan))) {
            if (r.durationValue == kInvalid32 || r.durationValue >= i) {
                return Result::BadRepeat;
            }
            s.end        = StepEnd::Repeat;
            s.repeatFrom = static_cast<uint16_t>(r.durationValue);
            if (dt == static_cast<uint8_t>(WktStepDuration::RepeatUntilStepsComplete) &&
                r.targetValue != kInvalid32 && r.targetValue > 0) {
                s.repeatCount = r.targetValue <= kMaxRepeatCount ? r.targetValue : kMaxRepeatCount;
                s.degraded    = r.targetValue > kMaxRepeatCount;
            } else {
                s.repeatCount = 1;
                s.degraded    = true;
            }
            continue;  // A repeat has no target.
        }

        if (dt == static_cast<uint8_t>(WktStepDuration::Time) && r.durationValue != kInvalid32) {
            s.end        = StepEnd::Time;
            s.durationMs = r.durationValue;
        } else if (dt == static_cast<uint8_t>(WktStepDuration::Distance) &&
                   r.durationValue != kInvalid32) {
            s.end        = StepEnd::Distance;
            s.distanceCm = r.durationValue;
        } else {
            s.end = StepEnd::Open;
            if (dt != static_cast<uint8_t>(WktStepDuration::Open)) {
                s.degraded = true;
            }
        }

        const uint8_t tt = r.targetType;
        const bool haveLow  = r.customLow != kInvalid32;
        const bool haveHigh = r.customHigh != kInvalid32;
        const bool custom   = r.targetValue == 0 || r.targetValue == kInvalid32;
        if (tt == static_cast<uint8_t>(WktStepTarget::Open) || tt == kInvalid8) {
            s.target = Target::Open;
        } else if (tt == static_cast<uint8_t>(WktStepTarget::Speed) && custom && haveLow &&
                   haveHigh && (r.customLow > 0 || r.customHigh > 0)) {
            s.target        = Target::Speed;
            s.speedLowMmps  = r.customLow < r.customHigh ? r.customLow : r.customHigh;
            s.speedHighMmps = r.customLow < r.customHigh ? r.customHigh : r.customLow;
        } else if (tt == static_cast<uint8_t>(WktStepTarget::HeartRate) && !custom &&
                   r.targetValue >= 1 && r.targetValue <= 5) {
            s.target = Target::HeartRate;
            s.hrKind = HeartRateTarget::Zone;
            s.hrZone = static_cast<uint8_t>(r.targetValue);
        } else if (tt == static_cast<uint8_t>(WktStepTarget::HeartRate) && custom && haveLow &&
                   haveHigh && r.customLow > 0 && r.customHigh > 0 &&
                   // 1-100 is a percentage; 101-355 is 1-255 bpm. Both bounds
                   // must be the same kind.
                   ((r.customLow <= 100 && r.customHigh <= 100) ||
                    (r.customLow > 100 && r.customHigh > 100 &&
                     r.customLow <= 355 && r.customHigh <= 355))) {
            const bool     pct = r.customLow <= 100;
            const uint32_t lo  = pct ? r.customLow : r.customLow - 100;
            const uint32_t hi  = pct ? r.customHigh : r.customHigh - 100;
            s.target = Target::HeartRate;
            s.hrKind = pct ? HeartRateTarget::PercentMax : HeartRateTarget::Bpm;
            s.hrLow  = static_cast<uint16_t>(lo < hi ? lo : hi);
            s.hrHigh = static_cast<uint16_t>(lo < hi ? hi : lo);
        } else {
            s.target   = Target::Open;
            s.degraded = true;
        }
    }

    // Repeat blocks must nest: two blocks are either disjoint or one holds
    // the other (they may share a first step). A block is [repeatFrom, i].
    for (uint16_t a = 0; a < count; ++a) {
        if (out.steps[a].end != StepEnd::Repeat) {
            continue;
        }
        const uint16_t s1 = out.steps[a].repeatFrom;
        for (uint16_t b = a + 1; b < count; ++b) {
            if (out.steps[b].end != StepEnd::Repeat) {
                continue;
            }
            const uint16_t s2 = out.steps[b].repeatFrom;
            // a ends before b. Overlap without nesting: b starts inside a,
            // after a's start.
            if (s1 < s2 && s2 <= a) {
                return Result::BadRepeat;
            }
        }
    }
    for (uint16_t p = 0; p < count; ++p) {
        uint8_t depth = 0;
        for (uint16_t r = p; r < count; ++r) {
            if (out.steps[r].end == StepEnd::Repeat && out.steps[r].repeatFrom <= p) {
                ++depth;
            }
        }
        if (depth > kMaxRepeatDepth) {
            return Result::NestingTooDeep;
        }
    }

    for (uint16_t i = 0; i < count; ++i) {
        out.degraded = out.degraded || out.steps[i].degraded;
    }
    return Result::Ok;
}

const char* FitWorkoutReader::resultName(Result r)
{
    switch (r) {
    case Result::Ok:             return "ok";
    case Result::Truncated:      return "truncated";
    case Result::NotFit:         return "not a FIT file";
    case Result::BadHeaderCrc:   return "bad header CRC";
    case Result::BadFileCrc:     return "bad file CRC";
    case Result::Malformed:      return "malformed record";
    case Result::NotWorkout:     return "not a workout file";
    case Result::NoSteps:        return "no steps";
    case Result::TooManySteps:   return "too many steps";
    case Result::BadStepIndex:   return "bad step index";
    case Result::BadRepeat:      return "bad repeat";
    case Result::NestingTooDeep: return "repeats nested too deep";
    case Result::ReadError:      return "read error";
    }
    return "?";
}

}  // namespace SDK::Fit
