#include "Simulator/Sensors/Altimeter.hpp"

#include <assert.h>

#define TAG                 "Altimeter"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

//Sensor::Altimeter::Altimeter(Sensor::Driver& driverTemp, Sensor::Driver& driverPressure)
//	: mDataAltimeter(1)
//	, mCurrentTemperature(0)
//	, mDriverAltimeter(Sensor::Type::SENSOR_TYPE_ALTIMETER, 1, *this)
//	, mDriverTemp(driverTemp)
//	, mDriverPressure(driverPressure)
//	, mTicks(0)
//{
//}

//Sensor::Driver& Sensor::Altimeter::getDriver()
//{
//	return mDriverAltimeter;
//}
//
//void Sensor::Altimeter::onSensorStart(Sensor::Driver* sensor, float period)
//{
//	mDriverTemp.connect(this, nullptr, 1000, 5000);
//	mDriverPressure.connect(this, nullptr, 1000, 10000);
//	LOG_DEBUG("start\n");
//}
//
//void Sensor::Altimeter::onSensorStop(Sensor::Driver* sensor)
//{
//	mDriverTemp.disconnect(this);
//	mDriverPressure.disconnect(this);
//	LOG_DEBUG("stop\n");
//}
//
//float Sensor::Altimeter::onSensorNewPeriod(Sensor::Driver* sensor, float period)
//{
//	LOG_DEBUG("update period\n");
//	
//	return period;
//}
//
void Sensor::Altimeter::onNewSensorData(const Interface::ISensorDriver*             sensor,
										const std::vector<Interface::ISensorData*>& data,
										bool                                        first)
{
//	float summa = 0;
//	for (uint16_t idx = 0; idx < data.size(); ++idx) {
//		summa += data[idx]->getValue(0);
//	}
//
//	if (sensor == &mDriverTemp) {
//		mCurrentTemperature = summa / data.size();
//	} else if (sensor == &mDriverPressure) {
//		float pressure = summa / data.size();
//		mDataAltimeter.setTimestamp(mTicks++);
//		mDataAltimeter.setValue(0, mCurrentTemperature + pressure);
//		mDriverAltimeter.pushData(mDataAltimeter);
	}
//}
