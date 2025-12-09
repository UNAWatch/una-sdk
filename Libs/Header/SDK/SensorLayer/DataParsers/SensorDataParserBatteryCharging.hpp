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

#include "SDK/SensorLayer/SensorDataView.hpp"

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
    enum Field : uint8_t {
        CONNECTED = 0,  ///< USB cable connect status
        CHARGING,       ///< Charging status
        COUNT           ///< Number of fields (must be last)
    };

    /**
     * @brief   SensorData parser for the Battery Charging state/event
     * @param data Reference to sensor data
     */
    explicit BatteryCharging(const SDK::Sensor::DataView data) : mData(data) {}

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
        return ((mData.getFieldCount() == Field::COUNT) &&
                (mData.u[Field::CONNECTED] <= 1)        &&
                (mData.u[Field::CHARGING] <= 1));
    }

    /**
     * @brief   SensorData parser for the USB cable connect state
     *
     */
    bool isUsbConnected() const
    {
        return isDataValid() ? static_cast<bool>(mData.u[Field::CONNECTED]) : false;
    }

    /**
     * @brief   SensorData parser for charging state
     *
     */
    bool isCharging() const
    {
        return isDataValid() ? static_cast<bool>(mData.u[Field::CHARGING]) : false;
    }

    /**
     * @brief  Get timestamp of the data
     * @return Data timestamp in ms (0 if data pointer is null).
     */
    uint32_t getTimestamp() const
    {
        return isDataValid() ? mData.getTimestamp() : 0U;
    }

    /**
     * @brief Get data timestamp in us
     * @return Data timestamp in us (0 if invalid)
     */
    uint64_t getTimestampUs() const
    {
        return isDataValid() ? mData.getTimestampUs() : 0;
    }

    /**
     * @brief  Returns the number of fields
     * @return The number of fields
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
}; /* class BatteryCharging */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_BATTERY_CHARGING_HPP */


