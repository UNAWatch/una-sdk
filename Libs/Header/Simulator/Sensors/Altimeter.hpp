#ifndef __Altimeter_HPP
#define __Altimeter_HPP

#include "SensorLayer/ISensorDriver.hpp"
#include "SensorLayer/ISensorDataListener.hpp"

namespace Sensor
{
    class Altimeter : /*public Sensor::ISensorDriverCtrl,*/
        public Interface::ISensorDataListener
    {
    public:
        //Altimeter(Sensor::Driver& driverTemp, Sensor::Driver& driverPressure);

        //Sensor::Driver& getDriver();

        //void  onSensorStart(Sensor::Driver* sensor, float period)     override;
        //void  onSensorStop(Sensor::Driver* sensor)                    override;
        //float onSensorNewPeriod(Sensor::Driver* sensor, float period) override;

        void onNewSensorData(const Interface::ISensorDriver* sensor,
            const std::vector<Interface::ISensorData*>& data,
            bool                                        first) override;

    private:
        //Sensor::Data    mDataAltimeter;
        //float           mCurrentTemperature;
        //Sensor::Driver  mDriverAltimeter;
        //Sensor::Driver& mDriverTemp;
        //Sensor::Driver& mDriverPressure;
        //uint32_t        mTicks;
    };

}

#endif
