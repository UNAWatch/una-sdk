#include "Simulator/Sensors/DS18B20.hpp"

#define TAG                 "DS18B20"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

//Sensor::DS18B20::DS18B20()
//	: mData(1)
//	, mDriver(Sensor::Type::SENSOR_TYPE_AMBIENT_TEMPERATURE, mData.getLength(), *this)
//	, mTicks(0)
//{
//}

void Sensor::DS18B20::refresh(float temp)
{
	//mData.setTimestamp(mTicks++);
	//mData.setValue(0, temp);
	//
	//mDriver.pushData(mData);
}

//Sensor::Driver& Sensor::DS18B20::getDriver()
//{
//	return mDriver;
//}
//
//void Sensor::DS18B20::onSensorStart(Sensor::Driver* sensor, float period)
//{
//	LOG_DEBUG("start\n");
//}
//
//void Sensor::DS18B20::onSensorStop(Sensor::Driver* sensor)
//{
//	LOG_DEBUG("stop\n");
//}
//
//float Sensor::DS18B20::onSensorNewPeriod(Sensor::Driver* sensor, float period)
//{
//	LOG_DEBUG("update period\n");
//	return period;
//}
