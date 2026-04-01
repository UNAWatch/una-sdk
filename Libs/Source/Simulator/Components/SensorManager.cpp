/**
 ******************************************************************************
 * @file    SensorManager.cpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorManager class
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Components/SensorManager.hpp"
#include "SDK/Simulator/Kernel/Mock/System.hpp"
//#include "SwTimer.hpp"


#define LOG_MODULE_PRX      "Sensor.Mgr"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

#include <algorithm>
#include <vector>

////////////////////////////////////////////////////////////////////////////////
//// Diagnostic
////////////////////////////////////////////////////////////////////////////////

#if LOG_MODULE_LEVEL == LOG_LEVEL_DEBUG

struct SensorManagerDiagnostic {
public:
    SensorManagerDiagnostic()
        : mDurations()
        , mTimerRefresh()
        , mTimerUpdate()
    {
        mDurations.reserve(32);
        mTimerRefresh.start();
    }

    void prepare()
    {
        mDurations.clear();
    }

    void preRefresh()
    {
        mTimerUpdate.start();
    }

    void postRefresh()
    {
        mDurations.push_back(mTimerUpdate.passed());
    }

    void finalize()
    {
        LOG_INFO("[%u] - ", mTimerRefresh.passed());
        mTimerRefresh.start();

        for (auto d : mDurations) {
            LOG_INFO_WP("%4u ", d);
        }

        LOG_INFO_WP("\n");
    }

private:
    std::vector<uint32_t> mDurations;
    ::Driver::SwTimer     mTimerRefresh;
    ::Driver::SwTimer     mTimerUpdate;
};

#else

class SensorManagerDiagnostic {
public:
    void prepare()     {}
    void preRefresh()  {}
    void postRefresh() {}
    void finalize()    {}
};

#endif

////////////////////////////////////////////////////////////////////////////////
//// Sensor::Manager
////////////////////////////////////////////////////////////////////////////////
Sensor::Manager& Sensor::Manager::getInstance()
{
    static Sensor::Manager mInstance;
    
    return mInstance;
}

Sensor::Manager::Manager()
    : mDrivers()
    , mSensors()
    , mMutex()
    , mSemaphore(1, 0)
    , mPeriodTicks(10)
{
    mThread = std::thread(&Sensor::Manager::thread, this);
}

Sensor::Manager::~Manager()
{
    if (mThread.joinable())
        mThread.join();
}

bool Sensor::Manager::regDriver(Sensor::Driver* driver)
{
    auto it = std::find_if(mDrivers.begin(), mDrivers.end(),
                           [driver](const Sensor::Driver* d) {
                               return d == driver;
                           });

    if (it != mDrivers.end()) {
        LOG_ERROR("sensor is already registered\n");
        return false; // Already exists
    }

    mDrivers.push_back(driver);

    driver->setHandle(static_cast<uint8_t>(mDrivers.size()));

    return true;
}

bool Sensor::Manager::isDriverRegistered(Sensor::Driver* driver) const
{
    auto it = std::find_if(mDrivers.begin(), mDrivers.end(),
                           [driver](const Sensor::Driver* d) {
                               return d == driver;
                           });

    return it != mDrivers.end();
}

bool Sensor::Manager::isDriverRegistered(SDK::Sensor::Type type) const
{
    return std::any_of(mDrivers.begin(), mDrivers.end(),
                       [type](const Sensor::Driver* d) {
                           return d->getType() == type;
                       });
}

void Sensor::Manager::updatePeriod()
{
    mMutex.lock();

    updatePeriodNoLock();

    mMutex.unLock();
}

void Sensor::Manager::regSensor(Interface::ISensor* sensor)
{
    bool inserted = false;

    {
        mMutex.lock();

        auto it = std::find(mSensors.begin(), mSensors.end(), sensor);
        if (it == mSensors.end()) {
            mSensors.push_back(sensor);
            updatePeriodNoLock();
            inserted = true;
        } else {
            LOG_WARNING("the sensor is already registered\n");
        }

        mMutex.unLock();
    }

    if (inserted) {
        mSemaphore.give();
    }
}

void Sensor::Manager::unRegSensor(Interface::ISensor* sensor)
{
    mMutex.lock();

    auto it = std::find_if(mSensors.begin(), mSensors.end(),
                           [sensor](const Interface::ISensor* s) {
                               return s == sensor;
                           });

    if (it != mSensors.end()) {
        mSensors.erase(it);
        updatePeriodNoLock();
    }

    mMutex.unLock();
}

Sensor::Driver* Sensor::Manager::getDefaultSensor(SDK::Sensor::Type type) const
{
    for (auto d : mDrivers) {
        if (d->getType() == type) {
            return d;
        }
    }

    return nullptr;
}

uint16_t Sensor::Manager::getDefaultHandle(SDK::Sensor::Type type) const
{
    for (uint16_t idx = 0; idx < mDrivers.size(); ++idx) {
        if (mDrivers[idx]->getType() == type) {
            return mDrivers[idx]->getHandle();
        }
    }

    return 0;
}

std::vector<Sensor::Driver*> Sensor::Manager::getSensorList(SDK::Sensor::Type type) const
{
    std::vector<Sensor::Driver*> drivers;

    for (auto d : mDrivers) {
        if (d->getType() == type) {
            drivers.push_back(d);
        }
    }

    return drivers;
}

std::vector<uint16_t> Sensor::Manager::getHandleList(SDK::Sensor::Type type) const
{
    std::vector<uint16_t> drivers;

    for (uint16_t idx = 0; idx < mDrivers.size(); ++idx) {
        if (mDrivers[idx]->getType() == type) {
            uint16_t handle = mDrivers[idx]->getHandle();
            drivers.push_back(handle);
        }
    }

    return drivers;
}

Sensor::Driver* Sensor::Manager::getDriverByHandle(uint32_t handle) const
{
    if (handle == 0 || handle > mDrivers.size()) {
        return nullptr;
    }

    return mDrivers[handle - 1];
}

void Sensor::Manager::updatePeriodNoLock()
{
    if (mDrivers.size() == 0) {
        LOG_DEBUG("no drivers\n");
        return;
    }

    float minPeriod = mDrivers[0]->getRefreshPeriod();

    for (auto d : mDrivers) {
        if (minPeriod > d->getRefreshPeriod()) {
            minPeriod = d->getRefreshPeriod();
        }
    }

    LOG_DEBUG("mPeriodTicks=%u\n", mPeriodTicks);

    mPeriodTicks = static_cast<uint32_t>(minPeriod);
}

void Sensor::Manager::thread()
{
    SensorManagerDiagnostic diagnostic;

    for (;;) {
        bool empty = false;

        {
            mMutex.lock();
            empty = mSensors.empty();
            mMutex.unLock();
        }

        if (!SDK::Simulator::Mock::SystemGUI::isAppRunning()) {
            return;
        }

        if (empty) {
            mSemaphore.take(500);
            continue;
        }

        {
            mMutex.lock();
            diagnostic.prepare();

            LOG_DEBUG("refresh ENTRY\n");
            for (auto s : mSensors) {
                diagnostic.preRefresh();
                s->sensorRefresh();
                diagnostic.postRefresh();
            }

            diagnostic.finalize();

            LOG_DEBUG("refresh EXIT\n");

            mMutex.unLock();
        }

        mSemaphore.take(mPeriodTicks + 2);
    }
}
