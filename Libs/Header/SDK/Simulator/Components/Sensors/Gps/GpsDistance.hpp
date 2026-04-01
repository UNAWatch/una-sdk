/**
 ******************************************************************************
 * @file    GpsDistance.hpp
 * @date    28-October-2025
 * @author  Vlad
 * @brief   Sensor for the GPS Distance
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GPS_DISTANCE_HPP
#define __GPS_DISTANCE_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>
#include <cstdint>
#include <cmath>

namespace Sensor
{
    class GpsDistance : public Interface::ISensor,
                                 public Sensor::ISensorDriverCtrl
    {
    public:
        GpsDistance();
        
        Sensor::Driver& getDriver();

        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;

        //// ISensor
        void sensorRefresh() override;

    private:
        struct GpsPoint {
            float latitude;  ///< Latitude in degrees.
            float longitude; ///< Longitude in degrees.

            GpsPoint() = default;
            GpsPoint(float lat, float lon) : latitude(lat), longitude(lon) {}
            ~GpsPoint() = default;

            /**
             * @brief Checks if the GPS point has valid latitude and longitude.
             * @return True if valid, false otherwise.
             */
            bool valid() const
            {
                return latitude >= -90.0f && latitude <= 90.f && longitude >= -180.0f
                    && longitude <= 180.0f;
            }

            /**
             * @brief Scales the latitude and longitude by given factors.
             * @param lat: Scaling factor for latitude.
             * @param lon: Scaling factor for longitude.
             */
            void scale(float lat, float lon)
            {
                latitude *= lat;
                longitude *= lon;
            }

            /**
             * @brief Adds two GpsPoint objects.
             * @param other: Another GpsPoint to add.
             * @return Resulting GpsPoint after addition.
             */
            GpsPoint operator+(const GpsPoint &other) const
            {
                return {latitude + other.latitude, longitude + other.longitude};
            }

            /**
             * @brief Subtracts another GpsPoint from this one.
             * @param other: Another GpsPoint to subtract.
             * @return Resulting GpsPoint after subtraction.
             */
            GpsPoint operator-(const GpsPoint &other) const
            {
                return {latitude - other.latitude, longitude - other.longitude};
            }

            /**
             * @brief Adds another GpsPoint to this one.
             * @param other Another GpsPoint to add.
             * @return Reference to this GpsPoint after addition.
             */
            GpsPoint &operator+=(const GpsPoint &other)
            {
                latitude += other.latitude;
                longitude += other.longitude;
                return *this;
            }

            /**
             * @brief Subtracts another GpsPoint from this one.
             * @param other Another GpsPoint to subtract.
             * @return Reference to this GpsPoint after subtraction.
             */
            GpsPoint &operator-=(const GpsPoint &other)
            {
                latitude -= other.latitude;
                longitude -= other.longitude;
                return *this;
            }

        };

        float deg2rad(float degrees)
        {
            // Conversion factor from degrees to radians.
            return static_cast<float>((degrees * M_PI) / 180.0);
        }

        // Great-circle distance (Haversine), returns meters.
        float distanceHaversine(const GpsPoint& p1, const GpsPoint& p2)
        {
            // Mean Earth radius in meters
            static constexpr float kEarthRadiusM = 6371000.0f;

            const float lat1        = deg2rad(p1.latitude);
            const float lat2        = deg2rad(p2.latitude);
            const float dLat        = lat2 - lat1;
            const float dLon        = deg2rad(p2.longitude - p1.longitude);
            const float sinHalfDLat = std::sin(dLat * 0.5f);
            const float sinHalfDLon = std::sin(dLon * 0.5f);

            const float a = sinHalfDLat * sinHalfDLat + std::cos(lat1) * std::cos(lat2) * sinHalfDLon * sinHalfDLon;
            const float c = 2.0f * std::atan2(std::sqrt(a), std::sqrt(1.0f - a));

            return kEarthRadiusM * c;
        }

        Sensor::Driver     mDriver;
        ::Driver::SwTimer  mTimer;
        Interface::IGps&   mGps;

        static constexpr uint32_t mMinPeriod = 1000; // In ms

        float                  mDistance;
        GpsPoint               mGpsPoint0;
        bool                   mGpsPoint0Valid;
        OS::Queue<GpsPoint, 5> mPoints;
    }; /* class GpsDistance */

} /* namespace Sensor */

#endif /* __GPS_DISTANCE_HPP */
