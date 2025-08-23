#ifndef __BME280_HPP
#define __BME280_HPP

//#include "SensorLayer/SensorDriver.hpp"

namespace Sensor {

    class BME280 /*: public Sensor::ISensorDriverCtrl*/
    {
    public:
        BME280();

        void refresh(float temp, float pressure);

        //Sensor::Driver& getTempDriver();
        //Sensor::Driver& getPressureDriver();

        //void  onSensorStart(Sensor::Driver* sensor, float period)     override;
        //void  onSensorStop(Sensor::Driver* sensor)                    override;
        //float onSensorNewPeriod(Sensor::Driver* sensor, float period) override;

    private:
        //Sensor::Data   mDataTemp;
        //Sensor::Data   mDataPressure;
        //Sensor::Driver mDriverTemp;
        //Sensor::Driver mDriverPressure;
        //uint32_t       mTicks;
    };

}

#endif
