/**
 ******************************************************************************
 * @file    GpsLocation.hpp
 * @date    28-October-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor for the GPS Location
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GPS_LOCATION_HPP
#define __GPS_LOCATION_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/Filter/SimpleHPF.hpp"
#include "SDK/Simulator/Components/Filter/SimpleLPF.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>
#include <cstdint>
#include <cmath>

using namespace Interface;

namespace Sensor
{
    class GpsLocation : public Interface::ISensor,
                        public Sensor::ISensorDriverCtrl
    {
    public:
        GpsLocation();
        
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
        float deg2rad(float degrees)
        {
            // Conversion factor from degrees to radians.
           return static_cast<float>((degrees * M_PI) / 180.0);
        }

        // This method uses a simplified (Euclidean) flat-earth formula assuming
        // small distances, which is appropriate for local map rendering purposes.
        float distanceFlatEarth(const IGps::LocationInfo& p1, const IGps::LocationInfo& p2)
        {
            /// Approximate meters per degree of latitude.
            static constexpr float kMetersPerDegree = 111320.0f;

            float dx = (p2.lon - p1.lon) * std::cos(deg2rad((p1.lat + p2.lat) * 0.5f));
            float dy = (p2.lat - p1.lat);
            return kMetersPerDegree* std::hypot(dx, dy);
        }

        float distanceFlatEarth(const IGps::LocationInfo& p1, float lat, float lon)
        {
            IGps::LocationInfo p2{};
            p2.lat = lat;
            p2.lon = lon;

            return distanceFlatEarth(p1, p2);
        }

        static constexpr uint32_t mMinPeriod = 1000; // In ms

        Sensor::Driver                   mDriver;
        ::Driver::SwTimer                mTimer;
        Filter::SimpleLPF                mFilterLat;
        Filter::SimpleLPF                mFilterLon;
        Interface::IGps&                 mGps;
        bool                             mPoint0Inited;

    }; /* class GpsLocation */

} /* namespace Sensor */

#endif /* __GPS_LOCATION_HPP */
