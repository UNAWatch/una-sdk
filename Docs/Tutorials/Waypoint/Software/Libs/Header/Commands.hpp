/**
 ******************************************************************************
 * @file    Commands.hpp
 * @brief   Messages between the Waypoint service and its GUI.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "AppConfigFields.hpp"

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"
#include "SDK/Messages/CommandMessages.hpp"

#include <cstdint>

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage {

// Service --> GUI
constexpr SDK::MessageType::Type NAV_UPDATE = 0x00000001;
constexpr SDK::MessageType::Type TARGET_SAVED = 0x00000002;

// GUI --> Service
constexpr SDK::MessageType::Type SAVE_TARGET_HERE = 0x00000010;

/**
 * @brief   Everything the screen needs to draw one frame.
 *
 * Kept as plain data rather than as message fields: a MessageBase is
 * pool-allocated and non-copyable, so the GUI could not keep a copy of the
 * last update to redraw from.
 */
struct NavState {
    char    waypointName[WaypointConfig::kNameBytes];
    float   distanceM;      ///< Distance to the target; valid only with a fix.
    float   bearingDeg;     ///< 0..360 from true north; valid only with a fix.
    float   targetLatitude;
    float   targetLongitude;
    int32_t arrivalRadiusM;
    bool    hasFix;
    bool    arrived;
    /// True when the target came from the user rather than the app's default.
    bool    targetIsConfigured;
};

/**
 * @brief   Service --> GUI: a new navigation state.
 *
 * Sent on every GPS fix, and once when the GUI starts so the screen has the
 * configured name and target before the first fix arrives.
 */
struct NavUpdate : public SDK::MessageBase {
    NavState nav;

    NavUpdate()
        : SDK::MessageBase(NAV_UPDATE)
        , nav {}
    {}
};

/// Why a SAVE_TARGET_HERE request ended the way it did.
enum class SaveOutcome : uint8_t {
    Saved = 0,      ///< Written to the file and re-read.
    NoFix,          ///< No GPS fix, so there was no position to store.
    WriteFailed,    ///< There was a fix, but the file could not be written.
};

/// Service --> GUI: the outcome of a SAVE_TARGET_HERE request.
struct TargetSaved : public SDK::MessageBase {
    SaveOutcome outcome;
    float       targetLatitude;      ///< The target now in effect.
    float       targetLongitude;

    TargetSaved()
        : SDK::MessageBase(TARGET_SAVED)
        , outcome(SaveOutcome::NoFix)
        , targetLatitude(0.0f)
        , targetLongitude(0.0f)
    {}

    explicit TargetSaved(SaveOutcome outcome, float latitude, float longitude)
        : TargetSaved()
    {
        this->outcome = outcome;
        this->targetLatitude = latitude;
        this->targetLongitude = longitude;
    }
};

/// GUI --> Service: store the current position as the new target.
struct SaveTargetHere : public SDK::MessageBase {
    SaveTargetHere()
        : SDK::MessageBase(SAVE_TARGET_HERE)
    {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
