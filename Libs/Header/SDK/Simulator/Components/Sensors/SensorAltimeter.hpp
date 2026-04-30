/**
 ******************************************************************************
 * @file    SensorAltimeter.hpp
 * @date    11-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor Altimeter
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_ALTIMETER_HPP
#define __SENSOR_ALTIMETER_HPP

#include "SensorLayer/SensorDriver.hpp"
#include "SensorLayer/ISensor.hpp"
#include "SensorLayer/SensorConnection.hpp"

#include "SDK/Interfaces/ISensorDataListener.hpp"

#include "SwTimer.hpp"
#include "Filters/SimpleLPF.hpp"

#include <cstdint>

namespace Sensor
{
    class Altimeter : public Interface::ISensor,
                      public Sensor::ISensorDriverCtrl,
                      public SDK::Interface::ISensorDataListener
    {
    public:
        Altimeter();
        
        Sensor::Driver& getDriverAltimeter();

        float       sdcStart(Sensor::Driver* driver, float period)        override;
        void        sdcStop(Sensor::Driver* driver)                       override;
        float       sdcUpdatePeriod(Sensor::Driver* driver, float period) override;
        float       sdcGetMinPeriod(Sensor::Driver* driver)               override;
        const char* sdcGetDescription(Sensor::Driver* driver)             override;

        void onSdlNewData(uint16_t                 sensor,
                          const SDK::Sensor::Data* base,
                          uint16_t                 count,
                          uint16_t                 stride) override;

        void sensorRefresh() override;

    private:
        float altitudeISA(float P_pa);

        static constexpr uint16_t mMinPeriod = 100;

        Sensor::Connection mDriverPressure;
        Sensor::Driver     mDriverAltimeter;
        ::Driver::SwTimer  mUpdateTimer;

        volatile bool      mPressureAvailable;
        float              mPressure;

        Filter::SimpleLPF  mFilter;
    }; /* class Altimeter */

} /* namespace Sensor */

#endif /* __SENSOR_ALTIMETER_HPP */
