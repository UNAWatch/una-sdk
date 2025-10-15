
#pragma once

#include "SDK/Simulator/Sensors/ISensorCore.hpp"
//#include "Simulator/Sensors/DS18B20.hpp"
//#include "Simulator/Sensors/BME280.hpp"
//#include "Simulator/Sensors/Altimeter.hpp"

namespace SDK::Simulator::Sensors {

class Core : public SDK::Simulator::Sensors::ISensorCore
{
public:
    Core();

    void tick() override;

private:
    //Sensor::DS18B20   mDS18B20;
    //Sensor::BME280    mBME280;
    //Sensor::Altimeter mAltimeter;
};

} // namespace SDK::Simulator::Sensors
