/**
 ******************************************************************************
 * @file    FitWorkoutWriter.cpp
 * @brief   A workout program as FIT workout, workout_step and memo_glob
 *          messages.
 ******************************************************************************
 */

#include "SDK/Fit/FitWorkoutWriter.hpp"

#include "SDK/Fit/FitProfile.hpp"

#include <cstring>

namespace SDK::Fit {

namespace {

using SDK::Workout::kRepeatForever;
using SDK::Workout::Program;
using SDK::Workout::Step;
using SDK::Workout::StepEnd;

bool written(const Step& s)
{
    return !(s.end == StepEnd::Repeat && s.repeatCount == kRepeatForever);
}

// The first @p cap - 1 bytes of @p src, cut back to a character boundary,
// NUL-terminated in @p dst.
void truncateUtf8(char* dst, size_t cap, const char* src)
{
    size_t n = std::strlen(src);
    if (n >= cap) {
        n = cap - 1;
        while (n > 0 && (static_cast<uint8_t>(src[n]) & 0xC0) == 0x80) {
            --n;
        }
    }
    std::memcpy(dst, src, n);
    dst[n] = '\0';
}

bool writeMemoParts(FitWriter& fit, uint8_t local, const char* text, size_t len)
{
    bool ok = fit.defineMessage(local, mesgNum(MesgNum::MemoGlob),
        {field::MemoGlob::PartIndex, field::MemoGlob::MesgNum, field::MemoGlob::ParentIndex,
         field::MemoGlob::FieldNum,
         {field::MemoGlob::kDataNum, BaseType::UInt8z, kMemoPartBytes}});
    uint32_t part = 0;
    for (size_t at = 0; at < len; at += kMemoPartBytes, ++part) {
        uint8_t data[kMemoPartBytes] = {};  // a short last part is padded with 0, "no value"
        const size_t n = len - at < kMemoPartBytes ? len - at : kMemoPartBytes;
        std::memcpy(data, text + at, n);
        ok = fit.data(local)
                 .u32(part)
                 .u16(mesgNum(MesgNum::Workout))
                 .u16(0)
                 .u8(field::Workout::kWktDescriptionNum)
                 .bytes(data, sizeof(data))
                 .write() && ok;
    }
    return ok;
}

}  // namespace

bool writeWorkout(FitWriter& fit, const Program& program, const WorkoutLocalTypes& local)
{
    uint16_t count = 0;
    for (uint16_t i = 0; i < program.stepCount; ++i) {
        count += written(program.steps[i]) ? 1 : 0;
    }

    char description[kWktDescriptionBytes];
    truncateUtf8(description, sizeof(description), program.description);

    bool ok = fit.defineMessage(local.workout, mesgNum(MesgNum::Workout),
        {field::Workout::MessageIndex,
         {field::Workout::kWktNameNum, BaseType::String,
          static_cast<uint8_t>(SDK::Workout::kNameBytes)},
         field::Workout::NumValidSteps, field::Workout::Sport, field::Workout::SubSport,
         {field::Workout::kWktDescriptionNum, BaseType::String, kWktDescriptionBytes}});
    ok = fit.data(local.workout)
             .u16(0)
             .str(program.name, static_cast<uint8_t>(SDK::Workout::kNameBytes))
             .u16(count)
             .u8(program.sport)
             .u8(program.subSport)
             .str(description, kWktDescriptionBytes)
             .write() && ok;

    const size_t descriptionLen = std::strlen(program.description);
    if (descriptionLen >= kWktDescriptionBytes) {
        ok = writeMemoParts(fit, local.memo, program.description, descriptionLen) && ok;
    }

    ok = fit.defineMessage(local.step, mesgNum(MesgNum::WorkoutStep),
        {field::WorkoutStep::MessageIndex, field::WorkoutStep::DurationType,
         field::WorkoutStep::DurationValue, field::WorkoutStep::TargetType,
         field::WorkoutStep::TargetValue, field::WorkoutStep::CustomTargetValueLow,
         field::WorkoutStep::CustomTargetValueHigh, field::WorkoutStep::Intensity,
         {field::WorkoutStep::kNotesNum, BaseType::String,
          static_cast<uint8_t>(SDK::Workout::kStepNotesBytes)},
         {field::WorkoutStep::kWktStepNameNum, BaseType::String,
          static_cast<uint8_t>(SDK::Workout::kStepNameBytes)}}) && ok;

    for (uint16_t i = 0; i < program.stepCount; ++i) {
        const Step& s = program.steps[i];
        uint16_t index = 0;
        if (!written(s) || !SDK::Workout::fitStepIndex(program, i, index)) {
            continue;
        }
        uint32_t durationValue = s.raw.durationValue;
        if (s.end == StepEnd::Repeat) {
            // A repeat names the first step of its block, which may have
            // moved down if a step before it was left out.
            uint16_t from = 0;
            if (SDK::Workout::fitStepIndex(program, s.repeatFrom, from)) {
                durationValue = from;
            }
        }
        ok = fit.data(local.step)
                 .u16(index)
                 .u8(s.raw.durationType)
                 .u32(durationValue)
                 .u8(s.raw.targetType)
                 .u32(s.raw.targetValue)
                 .u32(s.raw.customLow)
                 .u32(s.raw.customHigh)
                 .u8(s.raw.intensity)
                 .str(s.notes, static_cast<uint8_t>(SDK::Workout::kStepNotesBytes))
                 .str(s.name, static_cast<uint8_t>(SDK::Workout::kStepNameBytes))
                 .write() && ok;
    }
    return ok;
}

}  // namespace SDK::Fit
