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

#include "SDK/Interfaces/ISensorData.hpp"

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
    /**
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     * @param data Reference to sensor data with
     */
    explicit BatteryMetrics(const Interface::ISensorData& data) : mData(&data) {}

    /**
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     * @param data Pointer to sensor data
     */
    explicit BatteryMetrics(const Interface::ISensorData* data) : mData(data) {}

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
        return (mData != nullptr) &&
               (mData->getLength() == static_cast<uint8_t>(Field::kCOUNT));
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
        if (!isDataValid()) {
            return -1.0f;
        }

        return mData->getAsFloat(static_cast<uint8_t>(Field::kVOLTAGE));
    }
        
    /**
     * @brief  Instantaneous current (mA)
     * @return Field value as float; returns 0.0f if data is invalid.
     */
    float getCurrent() const
    {
        if (!isDataValid()) {
            return 0.0f;
        }

        return mData->getAsFloat(static_cast<uint8_t>(Field::kCURRENT));
    }

    /**
     * @brief  Averaged/filtered current (mA).
     * @return Field value as float; returns 0.0f if data is invalid.
     */
    float getAverageCurrent() const
    {
        if (!isDataValid()) {
            return 0.0f;
        }
        
        return mData->getAsFloat(static_cast<uint8_t>(Field::kAVERAGE_CURRENT));
    }

    /**
     * @brief Remaining capacity (mAh).
     * @return Field value as float; returns -1.0f if data is invalid.
     */
    float getCapacity() const
    {
        if (!isDataValid()) {
            return -1.0f;
        }
        
        return mData->getAsFloat(static_cast<uint8_t>(Field::kCAPACITY));
    }

    /**
     * @brief Full design capacity (mAh).
     * @return Field value as float; returns -1.0f if data is invalid.
     */
    float getDesignCapacity() const
    {
        if (!isDataValid()) {
            return -1.0f;
        }
        
        return mData->getAsFloat(static_cast<uint8_t>(Field::kDESIGN_CAPACITY));
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
     * @brief   SensorData parser for the Battery Metrics (V/I/mAh) sensor
     */
    enum Field : uint8_t {
        kVOLTAGE = 0,      ///< Battery voltage (V)
        kCURRENT,          ///< Instantaneous current (mA); sign per firmware contract
        kAVERAGE_CURRENT,  ///< Averaged/filtered current (mA)
        kCAPACITY,         ///< Remaining capacity (mAh)
        kDESIGN_CAPACITY,  ///< Full charge (design) capacity (mAh)
        kCOUNT             ///< Number of fields (must be last)
    };

    const Interface::ISensorData* mData { nullptr };
}; /* class BatteryMetrics */

} /* namespace SDK::SensorDataParser */

#endif /* __SENSOR_DATA_PARSER_BATTERY_METRICS_HPP */


