/**
 ******************************************************************************
 * @file    Sensor.cpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorDriver class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DRIVER_HPP
#define __SENSOR_DRIVER_HPP

#include "SDK/Simulator/Components/SensorDataSample.hpp"
#include "SDK/Simulator/Components/SensorDataQueue.hpp"
#include "SDK/Simulator/Components/Sensors/ISensor.hpp"
#include "SDK/SensorLayer/SensorTypes.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"
#include "SDK/Simulator/OS/OS.hpp"

#include <list>

namespace Sensor
{
    class Driver;

    class ISensorDriverCtrl
    {
    public:
		/**
         * @brief   ISensorDriverCtrl.
         */
        virtual ~ISensorDriverCtrl() = default;
	
        virtual float sdcStart(Sensor::Driver* driver, float period)        = 0;
        virtual void  sdcStop(Sensor::Driver* driver)                       = 0;
        virtual float sdcUpdatePeriod(Sensor::Driver* driver, float period) = 0;
        virtual float sdcGetMinPeriod(Sensor::Driver* driver)               = 0;
        virtual void  sdcNewConnection(Sensor::Driver* driver) { (void) driver; };
        virtual const char* sdcGetDescription(Sensor::Driver* driver)       = 0;
    }; /* class ISensorDriverCtrl */

    class Driver {
    public:
        enum class Mode {
            PERIODIC_BASED,
            EVENT_BASED,
        };

        Driver(Interface::ISensor&        sensor,
               SDK::Sensor::Type          t,
               uint16_t                   fieldCount,
               Sensor::ISensorDriverCtrl& control,
               Mode                       mode = Mode::PERIODIC_BASED);

        Interface::ISensor& getSensor();

        void pushDataSample();
        
        // ISensorDriver interface
        bool connect(SDK::Interface::ISensorDataListener* listener,
                     float                                period  = 0,
                     uint32_t                             latency = 0);
        void disconnect(SDK::Interface::ISensorDataListener* listener);
        SDK::Sensor::Type getType() const;
        float getRefreshPeriod() const;

        uint32_t getCountOfListeners();

        Sensor::DataSample& getDataSample();

        float getMinPeriod();

        void    setHandle(uint8_t value);
        uint8_t getHandle();

        const char* getDescription();

    private:
        static constexpr float sMinPeriod = 0.5f;

        float applyPeriod(float requiredPeriod, uint32_t latency);
        float updatePeriod(float value, uint32_t latency);
        void  setPeriod(float value);
        Sensor::DataQueue* findListenerWithMinPeriod();

        float calcUpdatePeriod(float period, uint32_t latency) const;

        Interface::ISensor&          mSensor;
        OS::Mutex                    mMutex;
        std::list<Sensor::DataQueue> mListeners;
        SDK::Sensor::Type            mType;
        Sensor::ISensorDriverCtrl&   mControl;
        Sensor::DataSample           mDataSample;
        Sensor::DataQueue*           mNewConnectionListener;
        Mode                         mMode;
        float                        mRefreshPeriod;
        uint8_t                      mHandle;
    }; /* class Driver */

} /* namespace Sensor */

#endif /* __SENSOR_DRIVER_HPP */
