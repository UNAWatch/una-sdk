/**
 ******************************************************************************
 * @file    AppTypes.hpp
 * @date    24-07-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Common application types and link interface between GUI and Backend.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __APP_TYPES_HPP
#define __APP_TYPES_HPP

#include <cstdint>
#include <cstdbool>
#include <cstring>
#include <variant>
#include <memory>
#include <ctime>

/**
 * @namespace AppType
 * @brief Namespace for application-specific types.
 */
namespace AppType
{

/**
  * @brief   Data types to send GUI --> Backend
  */
namespace G2BEvent
{

struct Shutdown {};
struct VibroShort {};
struct VibroLong {};
struct BuzzShort {};
struct BuzzLong {};
struct Backlight {
    bool status;
};

// Define the data types that can be sent from GUI to Backend
using Data = std::variant<
        Shutdown,
        VibroShort,
        VibroLong,
        BuzzShort,
        BuzzLong,
        Backlight
        >;
};

/**
 * @brief   Data types to send Backend --> GUI
 */
namespace B2GEvent 
{

struct Booted {};

struct BatteryLevel {
    float level;
};
struct BatteryCharge {
    bool status;
};
struct Time {
    struct std::tm time;
};

// Define the data types that can be sent from Backend to GUI
using Data = std::variant<
        Booted,
        BatteryLevel,
        BatteryCharge,
        Time
        >;
};

/**
 * @class IGuiBackend
 * @brief Interface for GUI backend communication.
 *
 * This interface defines methods for receiving GUI events from the backend
 * and sending events to the backend.
 */
class IGuiBackend {
public:

    /**
     * @brief Receive GUI event from the backend.
     * @param data The event data to be received.
     * @return true if the event was successfully received, false otherwise.
     */
    virtual bool receiveGuiEvent(AppType::B2GEvent::Data &data) = 0;

    /**
     * @brief Send event to the backend.
     * @param data The event data to be sent.
     * @return true if the event was successfully sent, false otherwise.
     */
    virtual bool sendEventToBackend(const AppType::G2BEvent::Data &data) = 0;

    /**
     * @brief Default destructor.
     */
    virtual ~IGuiBackend() = default;

};

} // namespace AppTypes

#endif /* __APP_TYPES_HPP */
