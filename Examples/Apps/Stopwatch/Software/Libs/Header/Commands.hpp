/**
 ******************************************************************************
 * @file    Commands.hpp
 * @date    17-07-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Message contract between the Stopwatch service and its GUI.
 ******************************************************************************
 *
 * The service owns the state and answers every command with a full snapshot.
 * Nothing is sent periodically: the GUI extrapolates the running time from the
 * snapshot against its own clock, so traffic only happens on a transition.
 *
 ******************************************************************************
 */

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

#include "SDK/Messages/MessageBase.hpp"
#include "SDK/Messages/MessageTypes.hpp"

#include "Stopwatch.hpp"

// Force 4-byte alignment for all message structures
#pragma pack(push, 4)

namespace CustomMessage
{

// Service --> GUI
constexpr SDK::MessageType::Type STOPWATCH_STATE = 0x00000001;

// GUI --> Service
constexpr SDK::MessageType::Type STOPWATCH_START   = 0x00000002;
constexpr SDK::MessageType::Type STOPWATCH_PAUSE   = 0x00000003;
constexpr SDK::MessageType::Type STOPWATCH_LAP     = 0x00000004;
constexpr SDK::MessageType::Type STOPWATCH_RESET   = 0x00000005;
constexpr SDK::MessageType::Type STOPWATCH_REQUEST = 0x00000006;

/**
 * @brief Full stopwatch state, service to GUI.
 */
struct StopwatchState : public SDK::MessageBase {
    Stopwatch::State state;
    StopwatchState()
        : SDK::MessageBase(STOPWATCH_STATE)
        , state{}
    {}

    explicit StopwatchState(const Stopwatch::State &state)
        : StopwatchState()
    {
        this->state = state;
    }
};

// The fixed lap array keeps the message inside the 256-byte pool block, which
// is the largest the kernel offers, so the path stays allocation free.
static_assert(sizeof(StopwatchState) <= 256,
              "StopwatchState must fit the largest kernel message pool block");

struct StopwatchStart : public SDK::MessageBase {
    StopwatchStart() : SDK::MessageBase(STOPWATCH_START) {}
};

struct StopwatchPause : public SDK::MessageBase {
    StopwatchPause() : SDK::MessageBase(STOPWATCH_PAUSE) {}
};

struct StopwatchLap : public SDK::MessageBase {
    StopwatchLap() : SDK::MessageBase(STOPWATCH_LAP) {}
};

struct StopwatchReset : public SDK::MessageBase {
    StopwatchReset() : SDK::MessageBase(STOPWATCH_RESET) {}
};

/**
 * @brief Ask the service to reply with the current state.
 */
struct StopwatchRequest : public SDK::MessageBase {
    StopwatchRequest() : SDK::MessageBase(STOPWATCH_REQUEST) {}
};

} // namespace CustomMessage

#pragma pack(pop)

#endif // COMMANDS_HPP
