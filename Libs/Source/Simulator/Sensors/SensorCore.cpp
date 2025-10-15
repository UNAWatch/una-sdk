#include "SDK/Simulator/Sensors/SensorCore.hpp"
//#include "SensorLayer/SensorManager.hpp"

#include <assert.h>

#define LOG_MODULE_PRX      "SensorCore"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK\UnaLogger\Logger.h"

SDK::Simulator::Sensors::Core::Core()
	/*: mDS18B20()
	, mBME280()
	, mAltimeter(mDS18B20.getDriver(), mBME280.getPressureDriver())*/
{
	//Sensor::Manager::getInstance().regSensor(&mDS18B20.getDriver());			// Temp default

	//Sensor::Manager::getInstance().regSensor(&mBME280.getPressureDriver());		// Pressure default
	//Sensor::Manager::getInstance().regSensor(&mBME280.getTempDriver());

	//Sensor::Manager::getInstance().regSensor(&mAltimeter.getDriver());			// Altimeter default
}

void SDK::Simulator::Sensors::Core::tick()
{
	//mDS18B20.refresh(25.1f);
	//mBME280.refresh(23.0f, 500.0f);
}
