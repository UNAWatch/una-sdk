/**
 ******************************************************************************
 * @file    FitProfile.hpp
 * @brief   FIT profile constants for the messages the UNA apps produce.
 *
 * Global message numbers, per-message field definitions (number + base type)
 * and the enum value sets used by the activity-recording apps. Values are the
 * public FIT Profile interoperability constants (data dictionary), required so
 * that files produced by the native encoder interoperate with FIT consumers.
 * Only the messages and fields the apps actually write are included.
 *
 * Field scale/offset/units (where non-trivial) are noted in comments; callers
 * supply already-scaled integer values, matching the FIT convention.
 ******************************************************************************
 */

#ifndef __SDK_FIT_PROFILE_HPP
#define __SDK_FIT_PROFILE_HPP

#include "SDK/Fit/FitBaseType.hpp"
#include "SDK/Fit/FitWriter.hpp"

#include <cstdint>

namespace SDK::Fit {

/// Global FIT message numbers (mesg_num).
enum class MesgNum : uint16_t {
    FileId           = 0,
    Session          = 18,
    Lap              = 19,
    Record           = 20,
    Event            = 21,
    Workout          = 26,
    WorkoutStep      = 27,
    Activity         = 34,
    FieldDescription = 206,
    DeveloperDataId  = 207,
};

constexpr uint16_t mesgNum(MesgNum m) { return static_cast<uint16_t>(m); }

// --- Enum value sets the apps set on enum fields ----------------------------

enum class File : uint8_t { Activity = 4 };
enum class Sport : uint8_t { Generic = 0, Running = 1, Cycling = 2, Hiking = 17 };
enum class SubSport : uint8_t { Generic = 0, Treadmill = 1 };
enum class Event : uint8_t { Timer = 0 };
enum class EventType : uint8_t { Start = 0, Stop = 1 };
enum class ActivityType : uint8_t { Manual = 0, AutoMultiSport = 1 };
enum class Intensity : uint8_t { Active = 0, Rest = 1, Warmup = 2, Cooldown = 3, Invalid = 0xFF };
enum class Manufacturer : uint16_t { Development = 255 };
enum class WktStepDuration : uint8_t {
    Time = 0, Distance = 1, Open = 5, RepeatUntilStepsComplete = 6,
};
enum class WktStepTarget : uint8_t { Open = 2 };

/// message_index "invalid" sentinel (uint16).
constexpr uint16_t kMessageIndexInvalid = 0xFFFFu;

// --- Per-message field definitions ------------------------------------------
// Each scalar field is a (field-definition-number, base type) pair ready to
// pass to FitWriter::defineMessage. Variable-length string/array fields expose
// their field number so callers can size them at encode time.

namespace field {

namespace FileId {
    constexpr FitWriter::Field Type{0, BaseType::Enum};       // file
    constexpr FitWriter::Field Manufacturer{1, BaseType::UInt16};
    constexpr FitWriter::Field Product{2, BaseType::UInt16};
    constexpr FitWriter::Field SerialNumber{3, BaseType::UInt32z};
    constexpr FitWriter::Field TimeCreated{4, BaseType::UInt32};  // date_time
}

namespace Event {
    constexpr FitWriter::Field Timestamp{253, BaseType::UInt32};  // date_time, s
    constexpr FitWriter::Field EventField{0, BaseType::Enum};     // event
    constexpr FitWriter::Field EventType{1, BaseType::Enum};      // event_type
}

namespace Record {
    constexpr FitWriter::Field Timestamp{253, BaseType::UInt32};       // s
    constexpr FitWriter::Field PositionLat{0, BaseType::SInt32};       // semicircles
    constexpr FitWriter::Field PositionLong{1, BaseType::SInt32};      // semicircles
    constexpr FitWriter::Field HeartRate{3, BaseType::UInt8};          // bpm
    constexpr FitWriter::Field Cadence{4, BaseType::UInt8};            // rpm
    constexpr FitWriter::Field Distance{5, BaseType::UInt32};          // scale 100, m
    constexpr FitWriter::Field FractionalCadence{53, BaseType::UInt8}; // scale 128, rpm
    constexpr FitWriter::Field EnhancedSpeed{73, BaseType::UInt32};    // scale 1000, m/s
    constexpr FitWriter::Field EnhancedAltitude{78, BaseType::UInt32}; // scale 5, off 500, m
    constexpr FitWriter::Field StepLength{85, BaseType::UInt16};       // scale 10, mm
}

namespace Lap {
    constexpr FitWriter::Field MessageIndex{254, BaseType::UInt16};
    constexpr FitWriter::Field Timestamp{253, BaseType::UInt32};
    constexpr FitWriter::Field StartTime{2, BaseType::UInt32};
    constexpr FitWriter::Field TotalElapsedTime{7, BaseType::UInt32};  // scale 1000, s
    constexpr FitWriter::Field TotalTimerTime{8, BaseType::UInt32};    // scale 1000, s
    constexpr FitWriter::Field TotalDistance{9, BaseType::UInt32};     // scale 100, m
    constexpr FitWriter::Field TotalCalories{11, BaseType::UInt16};    // kcal
    constexpr FitWriter::Field AvgSpeed{13, BaseType::UInt16};         // scale 1000, m/s
    constexpr FitWriter::Field MaxSpeed{14, BaseType::UInt16};         // scale 1000, m/s
    constexpr FitWriter::Field AvgHeartRate{15, BaseType::UInt8};      // bpm
    constexpr FitWriter::Field MaxHeartRate{16, BaseType::UInt8};      // bpm
    constexpr FitWriter::Field TotalAscent{21, BaseType::UInt16};      // m
    constexpr FitWriter::Field TotalDescent{22, BaseType::UInt16};     // m
    constexpr FitWriter::Field WktStepIndex{71, BaseType::UInt16};     // message_index
}

namespace Session {
    constexpr FitWriter::Field MessageIndex{254, BaseType::UInt16};
    constexpr FitWriter::Field Timestamp{253, BaseType::UInt32};
    constexpr FitWriter::Field StartTime{2, BaseType::UInt32};
    constexpr FitWriter::Field Sport{5, BaseType::Enum};
    constexpr FitWriter::Field SubSport{6, BaseType::Enum};
    constexpr FitWriter::Field TotalElapsedTime{7, BaseType::UInt32};  // scale 1000, s
    constexpr FitWriter::Field TotalTimerTime{8, BaseType::UInt32};    // scale 1000, s
    constexpr FitWriter::Field TotalDistance{9, BaseType::UInt32};     // scale 100, m
    constexpr FitWriter::Field TotalCalories{11, BaseType::UInt16};    // kcal
    constexpr FitWriter::Field AvgSpeed{14, BaseType::UInt16};         // scale 1000, m/s
    constexpr FitWriter::Field MaxSpeed{15, BaseType::UInt16};         // scale 1000, m/s
    constexpr FitWriter::Field AvgHeartRate{16, BaseType::UInt8};      // bpm
    constexpr FitWriter::Field MaxHeartRate{17, BaseType::UInt8};      // bpm
    constexpr FitWriter::Field TotalAscent{22, BaseType::UInt16};      // m
    constexpr FitWriter::Field TotalDescent{23, BaseType::UInt16};     // m
    constexpr FitWriter::Field NumLaps{26, BaseType::UInt16};
    constexpr FitWriter::Field MetabolicCalories{196, BaseType::UInt16};  // kcal
}

namespace Activity {
    constexpr FitWriter::Field Timestamp{253, BaseType::UInt32};
    constexpr FitWriter::Field TotalTimerTime{0, BaseType::UInt32};  // scale 1000, s
    constexpr FitWriter::Field NumSessions{1, BaseType::UInt16};
    constexpr FitWriter::Field Type{2, BaseType::Enum};              // activity
    constexpr FitWriter::Field LocalTimestamp{5, BaseType::UInt32};  // local_date_time
}

namespace Workout {
    constexpr FitWriter::Field MessageIndex{254, BaseType::UInt16};
    constexpr FitWriter::Field Sport{4, BaseType::Enum};
    constexpr FitWriter::Field NumValidSteps{6, BaseType::UInt16};
    constexpr uint8_t          kWktNameNum = 8;  // string, size set by caller
}

namespace WorkoutStep {
    constexpr FitWriter::Field MessageIndex{254, BaseType::UInt16};
    constexpr FitWriter::Field DurationType{1, BaseType::Enum};   // wkt_step_duration
    constexpr FitWriter::Field DurationValue{2, BaseType::UInt32};
    constexpr FitWriter::Field TargetType{3, BaseType::Enum};     // wkt_step_target
    constexpr FitWriter::Field TargetValue{4, BaseType::UInt32};
    constexpr FitWriter::Field Intensity{7, BaseType::Enum};
}

namespace FieldDescription {
    constexpr FitWriter::Field DeveloperDataIndex{0, BaseType::UInt8};
    constexpr FitWriter::Field FieldDefinitionNumber{1, BaseType::UInt8};
    constexpr FitWriter::Field FitBaseTypeId{2, BaseType::UInt8};  // fit_base_type
    constexpr uint8_t          kFieldNameNum = 3;  // string, size set by caller
    constexpr uint8_t          kUnitsNum = 8;      // string, size set by caller
}

namespace DeveloperDataId {
    constexpr FitWriter::Field ApplicationId{1, BaseType::Byte, 16};
    constexpr FitWriter::Field DeveloperDataIndex{3, BaseType::UInt8};
}

}  // namespace field

}  // namespace SDK::Fit

#endif  // __SDK_FIT_PROFILE_HPP
