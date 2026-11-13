/**
 ******************************************************************************
 * @file    ImuStepCounter.hpp
 * @date    14-March-2026
 * @author  Vlad
 * @brief   Sensor for the step counter
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __IMU_STEP_COUNTER_HPP
#define __IMU_STEP_COUNTER_HPP


#include <SDK/Simulator/Components/SensorDriver.hpp>
#include "SDK/Simulator/OS/SwTimer.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IStepCounter.hpp"

#include <atomic>
#include <cstdint>

using namespace Interface;

namespace Sensor
{
    class ImuStepCounter : public Interface::ISensor,
                                    public Sensor::ISensorDriverCtrl
    {
    public:
        ImuStepCounter();
        
        Sensor::Driver& getDriver();
        Sensor::Driver& getDailyDriver();

        //// ISensorDriverCtrl
        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;


        //// ISensor
        void sensorRefresh() override;

    private:

        static constexpr uint16_t mMinPeriod = 500;

        struct Channel {
            ::Driver::SwTimer timer;
            std::atomic<bool> firstSample{false};
        };

        void refresh(Sensor::Driver& driver, Channel& channel);
        void publishData(Sensor::Driver& driver, uint32_t value);

        class DailySensor : public Interface::ISensor {
        public:
            explicit DailySensor(ImuStepCounter& owner) : mOwner(owner) {}
            void sensorRefresh() override { mOwner.refresh(mOwner.mDailyDriver, mOwner.mDailyChannel); }
        private:
            ImuStepCounter& mOwner;
        };

        IStepCounter&     mStepCounterSim;
        Sensor::Driver    mDriver;
        DailySensor       mDailySensor;
        Sensor::Driver    mDailyDriver;
        Channel           mChannel;
        Channel           mDailyChannel;
        OS::Mutex         mDataMutex;
        uint32_t          mActiveDrivers = 0;
    };

} /* namespace Sensor */

#endif /* __IMU_STEP_COUNTER_HPP */
