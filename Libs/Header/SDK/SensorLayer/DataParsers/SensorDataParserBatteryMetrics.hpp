/**
 ******************************************************************************
 * @file    SensorDataParserBatteryMetrics.hpp
 * @date    23-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_BATTERY_METRICS_HPP
#define __SENSOR_DATA_PARSER_BATTERY_METRICS_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK::SensorDataParser {

/**
 * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
 *
 * @details
 * The parser reads float fields from ISensorData and provides type-safe accessors.
 * It does not own the underlying storage.
 * No unit conversions are performed here; values are returned as-is from firmware.
 */
class BatteryMetrics
{
public:
    enum Field : uint8_t {
        VOLTAGE = 0,      ///< Battery voltage (V)
        CURRENT,          ///< Instantaneous current (mA); sign per firmware contract
        AVERAGE_CURRENT,  ///< Averaged/filtered current (mA)
        CAPACITY,         ///< Remaining capacity (mAh)
        DESIGN_CAPACITY,  ///< Full charge (design) capacity (mAh)
        COUNT             ///< Number of fields (must be last)
    };

    /**
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     * @param data Reference to sensor data with
     */
    explicit BatteryMetrics(const SDK::Sensor::DataView data) : mData(data) {}

    /**
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     *
     * @details
     * The parser reads float fields from ISensorData and provides type-safe accessors.
     * It does not own the underlying storage.
     * No unit conversions are performed here; values are returned as-is from firmware.
     */
    bool isDataValid() const
    {
        return (mData.getFieldCount() == Field::COUNT);
    }

    /**
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     *
     * @details
     * The parser reads float fields from ISensorData and provides type-safe accessors.
     * It does not own the underlying storage.
     * No unit conversions are performed here; values are returned as-is from firmware.
     */
    float getVoltage() const
    {
        return isDataValid() ? mData.f[Field::VOLTAGE] : -1.0f;
    }
        
    /**
     * @brief  Instantaneous current (mA)
     * @return Field value as float; returns 0.0f if data is invalid.
     */
    float getCurrent() const
    {
        return isDataValid() ? mData.f[Field::CURRENT] : 0.0f;
    }

    /**
     * @brief  Averaged/filtered current (mA).
     * @return Field value as float; returns 0.0f if data is invalid.
     */
    float getAverageCurrent() const
    {
        return isDataValid() ? mData.f[Field::AVERAGE_CURRENT] : 0.0f;
    }

    /**
     * @brief Remaining capacity (mAh).
     * @return Field value as float; returns -1.0f if data is invalid.
     */
    float getCapacity() const
    {
        return isDataValid() ? mData.f[Field::CAPACITY] : -1.0f;
    }

    /**
     * @brief Full design capacity (mAh).
     * @return Field value as float; returns -1.0f if data is invalid.
     */
    float getDesignCapacity() const
    {
        return isDataValid() ? mData.f[Field::DESIGN_CAPACITY] : -1.0f;
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
}; /* class BatteryMetrics */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_BATTERY_METRICS_HPP */


