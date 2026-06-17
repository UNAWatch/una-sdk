/**
 * @file AccessoryMessages.hpp
 * @brief External-accessory acquisition messages (kernel <-> application).
 *
 * Opt-in contract for external BLE sensors (heart-rate straps for v1). An app
 * sends RequestPrepare at its pre-activity screen to ask the kernel to acquire
 * the requested accessory kinds, and RequestRelease at exit. The kernel reports
 * progress via EventStatus. Accessory measurements still arrive through the
 * normal sensor types (e.g. HEART_RATE) — these messages only drive acquisition
 * and surface status; an app that never sends RequestPrepare never triggers
 * scanning and is unaffected.
 *
 * Shared between kernel and applications.
 */

#pragma once

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include <cstdint>

namespace SDK::Accessory
{

/**
 * @brief Accessory-kind bitmask (RequestPrepare/RequestRelease `kinds`).
 *
 * v1 ships HRM only; the other bits are reserved so the contract is stable as
 * cycling cadence/power profiles are added later.
 */
enum Kind : uint32_t {
    HRM     = 1u << 0,   ///< Heart Rate Service (0x180D)
    // Reserved for future profiles (do not reuse):
    // CADENCE = 1u << 1,
    // POWER   = 1u << 2,
};

/**
 * @brief Acquisition state reported by EventStatus.
 */
enum class State : uint8_t {
    UNAVAILABLE = 0,  ///< feature disabled or no remembered accessory
    IDLE        = 1,  ///< not acquiring
    SEARCHING   = 2,  ///< scanning for the accessory
    CONNECTING  = 3,  ///< found; connecting/discovering/subscribing
    CONNECTED   = 4,  ///< streaming
    LOST        = 5,  ///< dropped after being connected (re-acquiring)
};

} // namespace SDK::Accessory

#pragma pack(push, 4)

namespace SDK::Message::Accessory
{

/**
 * @brief Ask the kernel to acquire the given accessory kinds for this activity.
 */
struct RequestPrepare : public MessageBase {
    RequestPrepare()
        : MessageBase(MessageType::REQUEST_ACCESSORY_PREPARE)
        , kinds(0)
    {}

    uint32_t kinds;   ///< bitmask of SDK::Accessory::Kind
};

/**
 * @brief Release accessory links/scanning acquired via RequestPrepare.
 * @note  kinds == 0 releases everything.
 */
struct RequestRelease : public MessageBase {
    RequestRelease()
        : MessageBase(MessageType::REQUEST_ACCESSORY_RELEASE)
        , kinds(0)
    {}

    uint32_t kinds;   ///< bitmask of SDK::Accessory::Kind (0 = all)
};

/**
 * @brief Acquisition status update for a subscribed app (and the kernel GUI).
 */
struct EventStatus : public MessageBase {
    EventStatus()
        : MessageBase(MessageType::EVENT_ACCESSORY_STATUS)
        , kind(0)
        , state(static_cast<uint8_t>(SDK::Accessory::State::UNAVAILABLE))
        , name{}
    {}

    uint32_t kind;     ///< the SDK::Accessory::Kind this status is for
    uint8_t  state;    ///< SDK::Accessory::State
    char     name[24]; ///< device name (empty until known), NUL-terminated
};

} // namespace SDK::Message::Accessory

#pragma pack(pop)
