/**
 ******************************************************************************
 * @file    SensorDataParserGpsLocation.hpp
 * @date    02-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorData parser for GPS Location sensor
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_PARSER_GPS_LOCATION_HPP
#define __SENSOR_DATA_PARSER_GPS_LOCATION_HPP

#include "SDK/SensorLayer/SensorDataView.hpp"

#include <cstdint>

namespace SDK
{
    namespace SensorDataParser
    {
        /**
         * @brief Helper class to parse GPS sensor data from ISensorData
         *
         * Expected data layout:
         * - [0] float - precision
         * - [1] bool  - flag 'coordinates are valid'
         * - [2] float - latitude
         * - [3] float - longitude
         * - [4] float - altitude
         *
         * Validity of each field is checked via corresponding mask bit.
         */
        class GpsLocation
        {
        public:
            enum Field : uint8_t {
                PRECISION = 0,  ///< Precision (in meters)
                COORDS_VALID,   ///< Coordinates are valid
                LAT,            ///< Latitude,m (float)
                LON,            ///< Longitude,m (float)
                ALT,            ///< Altitude,m (float)
                COUNT           ///< Total number of fields
            };

            /**
             * @brief Construct a new GPS parser over given ISensorData
             * @param data Reference to sensor data containing GPS fields
             */
            GpsLocation(const SDK::Sensor::DataView data) : mData(data) {}

            /**
             * @brief Check if datais valid
             * @return true if data length is Field::COUNT
             */
            bool isDataValid() const
            {
                return ((mData.u[Field::COORDS_VALID] <= 1) &&
                        (mData.getFieldCount() == Field::COUNT));
            }

            /**
             * @brief Get GPS time
             * @return Time as uint32_t (e.g., UNIX timestamp)
             */
            float getPrecision() const
            {
                return isDataValid() ? mData.f[Field::PRECISION] : 0.0f;
            }

            /**
             * @brief Check if Coordinates are valid
             * @return true if valide false otherwise
             */
            bool isCoordinatesValid() const
            {
                return isDataValid() && (mData.u[Field::COORDS_VALID] == 1);
            }

            /**
             * @brief Get GPS coordinates
             * @param lat [out] Latitude in decimal degrees
             * @param lon [out] Longitude in decimal degrees
             * @param alt [out] Altitude in meters
             */
            void getCoordinates(float& lat, float& lon, float& alt) const
            {
                lat = getLatitude();
                lon = getLongitude();
                alt = getAltitude();
            }

            /**
             * @brief Get latitude
             * @return Latitude in decimal degrees
             */
            float getLatitude() const
            {
                return isDataValid() ? mData.f[Field::LAT] : 0.0f;
            }

            /**
             * @brief Get longitude
             * @return Longitude in decimal degrees
             */
            float getLongitude() const
            {
                return isDataValid() ? mData.f[Field::LON] : 0.0f;
            }

            /**
             * @brief Get altitude
             * @return Altitude in meters
             */
            float getAltitude() const
            {
                return isDataValid() ? mData.f[Field::ALT] : 0.0f;
            }

            /**
             * @brief Get data timestamp in ms
             * @return Data timestamp in ms (0 if invalid)
             */
            uint32_t getTimestamp() const
            {
                return isDataValid() ? mData.getTimestamp() : 0;
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
             * @brief Get total number of expected fields
             * @return Field count (6)
             */
            static constexpr uint8_t getFieldsNumber()
            {
                return Field::COUNT;
            }

        private:
            const SDK::Sensor::DataView mData;
        }; /* class GpsLocation */
    }; /* namespace SensorDataParser */

} /* namespace SDK */

#endif /* __SENSOR_DATA_PARSER_GPS_LOCATION_HPP */
