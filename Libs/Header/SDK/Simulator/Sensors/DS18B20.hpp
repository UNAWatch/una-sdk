#ifndef __DS18B20_HPP
#define __DS18B20_HPP

//#include "SensorLayer/SensorDriver.hpp"

namespace Sensor {

    class DS18B20 /* : public Sensor::ISensorDriverCtrl */
    {
    public:
        DS18B20();

        void refresh(float temp);

        //Sensor::Driver& getDriver();

        //void  onSensorStart(Sensor::Driver* sensor, float period)     override;
        //void  onSensorStop(Sensor::Driver* sensor)                    override;
        //float onSensorNewPeriod(Sensor::Driver* sensor, float period) override;

    private:
        //Sensor::Data   mData;
        //Sensor::Driver mDriver;
        //uint32_t       mTicks;
    };

}

#endif
