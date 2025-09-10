/**
 ******************************************************************************
 * @file    SensorDataParserGPS.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for GPS sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GPS_HPP
#define __SENSOR_DATA_PARSER_GPS_HPP

#include "SDK/Interfaces/ISensorData.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse GPS sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] uint32_t mask (bitmask of valid fields)
         * - [1] uint32_t time (e.g., UNIX timestamp)
         * - [2] float latitude
         * - [3] float longitude
         * - [4] float altitude
         * - [5] float speed (m/s)
         *
         * Validity of each field is checked via corresponding mask bit.
         */
        class GPS
        {
        public:
            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Reference to sensor data containing GPS fields
             */
            GPS(const SDK::Interface::ISensorData& data) : mData(&data) {}

            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Pointer to sensor data containing GPS fields
             */
            GPS(const SDK::Interface::ISensorData* data) : mData(data) {}

            /**
             * @brief Check if datais valid
             * @return true if data length is Field::kCount
             */
            bool isDataValid() const
            {
                return (mData != nullptr) && (mData->getLength() == Field::kCount);
            }

            /**
             * @brief Get mask
             * @return Mask
             */
            uint32_t getMask() const
            {
                return mData->getAsU32(Field::kMask);
            }

            /**
             * @brief Check if time field is valid
             * @return true if time is valid
             */
            bool isTimeValid() const
            {
                return ((mData->getAsU32(Field::kMask) & mMaskTime) != 0) && isDataValid();
            }

            /**
             * @brief Get GPS time
             * @return Time as uint32_t (e.g., UNIX timestamp)
             */
            uint32_t getTime() const
            {
                return mData->getAsU32(Field::kTime);
            }

            /**
             * @brief Check if coordinates (lat/lon/alt) are valid
             * @return true if coordinates are valid
             */
            bool isCoordinatesValid() const
            {
                return ((mData->getAsU32(Field::kMask) & mMaskCoords) != 0) && isDataValid();
            }

            /**
             * @brief Get GPS coordinates
             * @param lat [out] Latitude in decimal degrees
             * @param lon [out] Longitude in decimal degrees
             * @param alt [out] Altitude in meters
             */
            void getCoordinates(float& lat, float& lon, float& alt) const
            {
                lat = mData->getAsFloat(Field::kLat);
                lon = mData->getAsFloat(Field::kLon);
                alt = mData->getAsFloat(Field::kAlt);
            }

            /**
             * @brief Get latitude
             * @return Latitude in decimal degrees
             */
            float getLatitude() const
            {
                return mData->getAsFloat(Field::kLat);
            }

            /**
             * @brief Get longitude
             * @return Longitude in decimal degrees
             */
            float getLongitude() const
            {
                return mData->getAsFloat(Field::kLon);
            }

            /**
             * @brief Get altitude
             * @return Altitude in meters
             */
            float getAltitude() const
            {
                return mData->getAsFloat(Field::kAlt);
            }

            /**
             * @brief Check if speed is valid
             * @return true if speed is valid
             */
            bool isSpeedValid() const
            {
                return ((mData->getAsU32(Field::kMask) & mMaskSpeed) != 0) && isDataValid();
            }

            /**
             * @brief Get GPS speed
             * @return Speed in meters per second
             */
            float getSpeed() const
            {
                return mData->getAsFloat(Field::kSpeed);
            }

            /**
             * @brief Get data timestamp in ms
             * @return Data timestamp in ms (0 if invalid)
             */
            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData->getTimestamp() : 0;
            }

            /**
             * @brief Get total number of expected fields
             * @return Field count (6)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::kCount;
            }

        private:
            /**
             * @brief Field layout indices
             */
            enum Field : uint8_t {
                kMask = 0,  ///< Bitmask field (uint32_t)
                kTime,      ///< Timestamp (uint32_t)
                kLat,       ///< Latitude (float)
                kLon,       ///< Longitude (float)
                kAlt,       ///< Altitude (float)
                kSpeed,     ///< Speed (float)
                kCount      ///< Total number of fields
            };

            static constexpr uint8_t mMaskTime   = 0x01;
            static constexpr uint8_t mMaskCoords = 0x02;
            static constexpr uint8_t mMaskSpeed  = 0x04;

            const SDK::Interface::ISensorData* mData;
        }; /* class GPS */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GPS_HPP */
