/**
 ******************************************************************************
 * @file    SensorDataParserBatteryLevel.hpp
 * @date    23-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Battery Level sensor
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_BATTERY_LEVEL_HPP
#define __SENSOR_DATA_PARSER_BATTERY_LEVEL_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK {
namespace SensorDataParser {

/**
 * @brief Helper class to parse battery level sensor data from ISensorData
 *
 * Expected data layout:
 * - [0] float battery charge level in percent (0..100)
 */
class BatteryLevel
{
public:
    enum Field : uint8_t {
        LEVEL = 0,  ///< Battery charge level in percent (float, 0..100)
        COUNT       ///< Number of fields (must be last)
    };

    /**
     * @brief Construct a new BatteryLevel parser over the given ISensorData
     * @param data Reference to sensor data containing 1 float field
     */
    explicit BatteryLevel(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief Check if data is valid (1 field, level within 0..100)
     * @return true if valid
     */
    bool isDataValid() const
    {
        return ((mData.getFieldCount() == Field::COUNT) &&
                (mData.f[Field::LEVEL] >= 0.0f) &&
                (mData.f[Field::LEVEL] <= 100.0f));
    }

    /**
     * @brief Get battery charge level in percent
     * @return Charge level 0..100 (-1.0 if invalid)
     */
    float getCharge() const
    {
        return isDataValid() ? mData.f[Field::LEVEL] : -1.0f;
    }

    /**
     * @brief Get data timestamp in ms
     * @return Data timestamp in ms (0 if invalid)
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
     * @brief Get number of expected fields (always 1)
     */
    static constexpr uint8_t getFieldsNumber()
    {
        return Field::COUNT;
    }

private:
    const SDK::Sensor::DataView mData;
}; /* class BatteryLevel */

} /* namespace SensorDataParser */
} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_BATTERY_LEVEL_HPP */
