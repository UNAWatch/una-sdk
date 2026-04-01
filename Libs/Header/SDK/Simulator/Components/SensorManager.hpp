/**
 ******************************************************************************
 * @file    SensorManager.hpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorManager class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_MANAGER_HPP
#define __SENSOR_MANAGER_HPP

#include "SDK/Simulator/Components/SensorDriver.hpp"
#include "SDK/Simulator/Components/Sensors/ISensor.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Simulator/OS/OS.hpp"
#include <thread>
#include <vector>

namespace Sensor
{
    class Manager {
    public:
        static Manager& getInstance();

        bool regDriver(Sensor::Driver* driver);
        bool isDriverRegistered(Sensor::Driver* driver) const;
        bool isDriverRegistered(SDK::Sensor::Type type) const;

        void updatePeriod();
        void thread();
        void regSensor(Interface::ISensor* sensor);
        void unRegSensor(Interface::ISensor* sensor);

        Sensor::Driver*              getDefaultSensor(SDK::Sensor::Type type) const;
        uint16_t                     getDefaultHandle(SDK::Sensor::Type type) const;
        std::vector<Sensor::Driver*> getSensorList(SDK::Sensor::Type type) const;
        std::vector<uint16_t>        getHandleList(SDK::Sensor::Type type) const;

        Sensor::Driver*              getDriverByHandle(uint32_t handle) const;

    private:
        Manager();
        ~Manager();

        void updatePeriodNoLock();
       
        std::vector<Sensor::Driver*>     mDrivers;
        std::vector<Interface::ISensor*> mSensors;
        OS::Mutex                        mMutex;
        OS::Semaphore                    mSemaphore;
        uint32_t                         mPeriodTicks;
        std::thread                      mThread;
    };

} /* namespace Sensor */

#endif /* __SENSOR_MANAGER_HPP */
