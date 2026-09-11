/**
 ******************************************************************************
 * @file    BodySubSensorHeartRate.hpp
 * @date    05-January-2026
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SubSensor for the Heart Rate
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __BODY_SUB_SENSOR_HEART_RATE_HPP
#define __BODY_SUB_SENSOR_HEART_RATE_HPP

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IHeatRate.hpp"

#include <cstdint>
#include <vector>

using namespace Interface;

namespace Sensor
{
    class HeartRate : public ISensor,
                      public Sensor::ISensorDriverCtrl
    {
    public:
        HeartRate();
        
        /// HEART_RATE: bpm and trust.
        Sensor::Driver& getDriver();

        /// HEART_RATE_EX: the kernel's arbitrated reading plus the raw optical
        /// and strap values. The activity apps subscribe to this one; the
        /// simulator reports the optical source only.
        Sensor::Driver& getDriverEx();

        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;

        //// ISensor
        void sensorRefresh() override;

    private:
        static constexpr float    mMinPeriod = 1000.0f; // In ms

        Sensor::Driver                  mDriver;
        Sensor::Driver                  mDriverEx;
        Interface::IHeartRate&          mpHeatRateSim;
        ::Driver::SwTimer               mTimer;
        uint8_t                         mHr;
        uint8_t                         mTrustLevel;
        bool                            mStarted   = false;   // HEART_RATE connected
        bool                            mStartedEx = false;   // HEART_RATE_EX connected
    };

} /* namespace Sensor */

#endif /* __BODY_SUB_SENSOR_HEART_RATE_HPP */
