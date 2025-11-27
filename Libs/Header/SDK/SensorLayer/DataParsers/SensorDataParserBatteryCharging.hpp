/**
 ******************************************************************************
 * @file    SensorDataParserBatteryCharging.hpp
 * @date    23-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Battery Charging state/event
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_BATTERY_CHARGING_HPP
#define __SENSOR_DATA_PARSER_BATTERY_CHARGING_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK::SensorDataParser {

/**
 * @brief   SensorData parser for the Battery Charging state/event
 *
 * @details
 * The parser provides type-safe accessors for the charging state/event.
 * It reads from ISensorData without owning the underlying storage.
 * No conversions are performed beyond basic type reads.*/
class BatteryCharging
{
public:
    enum class State {
        USB_CONNNECTED,
        USB_DISCONNNECTED,
        CHARGING,
        NO_CHARGING,
    };

    /**
     * @brief   SensorData parser for the Battery Charging state/event
     * @param data Reference to sensor data
     */
    explicit BatteryCharging(const Interface::ISensorData& data) : mData(&data) {}

    /**
     * @brief   SensorData parser for the Battery Charging state/event
     * @param data Pointer to sensor data
     */
    explicit BatteryCharging(const Interface::ISensorData* data) : mData(data) {}

    /**
     * @brief   SensorData parser for the Battery Charging state/event
     *
     * @details
     * The parser provides type-safe accessors for the charging state/event.
     * It reads from ISensorData without owning the underlying storage.
     * No conversions are performed beyond basic type reads.
     */
    bool isDataValid() const
    {
        return (mData != nullptr) &&
               (mData->getLength() == static_cast<uint8_t>(Field::kCOUNT) &&
               (mData->getAsU32(static_cast<uint8_t>(Field::kCHARGING)) <= 1));
    }

    /**
     * @brief   SensorData parser for the Battery Charging state/event
     *
     * @details
     * The parser provides type-safe accessors for the charging state/event.
     * It reads from ISensorData without owning the underlying storage.
     * No conversions are performed beyond basic type reads.
     */
    State getState() const
    {
        if (!isDataValid()) {
            return State::USB_DISCONNNECTED;
        }

        return static_cast<State>(mData->getAsU32(static_cast<uint8_t>(Field::kCHARGING)));
    }

    /**
     * @brief  Get timestamp of the data
     * @return Data timestamp in ms (0 if data pointer is null).
     */
    uint32_t getTimestamp() const
    {
        return (mData != nullptr) ? mData->getTimestamp() : 0U;
    }

    /**
     * @brief Get data timestamp in us
     * @return Data timestamp in us (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData->getTimestampUs() : 0;
    }

    /**
     * @brief  Returns the number of fields
     * @return The number of fields
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return static_cast<uint8_t>(Field::kCOUNT);
    }

private:
    /**
     * @brief   SensorData parser for the Battery Charging state/event
     */
    enum Field : uint8_t {
        kCHARGING = 0,  ///< Raw charge value
        kCOUNT          ///< Number of fields (must be last)
    };

    const Interface::ISensorData* mData { nullptr };
}; /* class BatteryCharging */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_BATTERY_CHARGING_HPP */


