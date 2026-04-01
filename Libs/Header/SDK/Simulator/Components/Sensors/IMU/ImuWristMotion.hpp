/**
 ******************************************************************************
 * @file    ImuWristMotion.hpp
 * @date    14-November-2025
 * @author  Vlad
 * @brief   Sensor for the accelerometer
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __IMU_WRIST_MOTION_HPP
#define __IMU_WRIST_MOTION_HPP

#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <SDK/Simulator/Components/SensorDriver.hpp>

#include <cstdint>

namespace Sensor
{
    class ImuWristMotion : public Interface::ISensor,
                           public Sensor::ISensorDriverCtrl
    {
    public:
        ImuWristMotion();
        
        Sensor::Driver& getDriver();

        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;

        //// ISensor
        void sensorRefresh() override;

        void handleWristMotion(uint64_t timestamp);

    private:
        static constexpr uint16_t mMinPeriod = 200;

        Sensor::Driver    mDriver;
        ::Driver::SwTimer mTimer;

        OS::Queue<uint64_t, 3> mQueue;
    };

}

#endif /* __IMU_WRIST_MOTION_HPP */
