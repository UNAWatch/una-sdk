#include "Simulator/Sensors/BME280.hpp"

#include <assert.h>

#define LOG_MODULE_PRX      "BME280"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

//Sensor::BME280::BME280()
//	: mDataTemp(1)
//	, mDriverTemp(Sensor::Type::SENSOR_TYPE_AMBIENT_TEMPERATURE, mDataTemp.getLength(), *this)
//	, mDataPressure(1)
//	, mDriverPressure(Sensor::Type::SENSOR_TYPE_PRESSURE, mDataPressure.getLength(), *this)
//	, mTicks(0)
//{
//}

void Sensor::BME280::refresh(float temp, float pressure)
{
	//mDataTemp.setTimestamp(mTicks);
	//mDataTemp.setValue(0, temp);
	//mDriverTemp.pushData(mDataTemp);

	//mDataPressure.setTimestamp(mTicks++);
	//mDataPressure.setValue(0, pressure);
	//mDriverPressure.pushData(mDataPressure);
}

//Sensor::Driver& Sensor::BME280::getTempDriver()
//{
//	return mDriverTemp;
//}
//
//Sensor::Driver& Sensor::BME280::getPressureDriver()
//{
//	return mDriverPressure;
//}
//
//void Sensor::BME280::onSensorStart(Sensor::Driver* sensor, float period)
//{
//	if (sensor == &mDriverTemp) {
//		LOG_DEBUG("start temp driver\n");
//	} else if (sensor == &mDriverPressure) {
//		LOG_DEBUG("start pressure driver\n");
//	} else {
//		assert(false);
//	}
//}
//
//void Sensor::BME280::onSensorStop(Sensor::Driver* sensor)
//{
//	if (sensor == &mDriverTemp) {
//		LOG_DEBUG("stop temp driver\n");
//	} else if (sensor == &mDriverPressure) {
//		LOG_DEBUG("stop pressure driver\n");
//	} else {
//		assert(false);
//	}
//}
//
//float Sensor::BME280::onSensorNewPeriod(Sensor::Driver* sensor, float period)
//{
//	if (sensor == &mDriverTemp) {
//		LOG_DEBUG("update period temp driver\n");
//	} else if (sensor == &mDriverPressure) {
//		LOG_DEBUG("update period pressure driver\n");
//	} else {
//		assert(false);
//	}
//
//	return period;
//}
