/**
 ******************************************************************************
 * @file    MockActivity.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IActivity interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_ACTIVITY_HPP
#define __SIMULATOR_KERNEL_ACTIVITY_HPP

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <random>
#include <ctime>
#include <cmath> // For sin()
#include <windows.h>

#include "IActivity.hpp"

#include "gui/common/GuiConfig.hpp"
//#include "gui/common/GuiCommon.hpp"
#include "WalkTrackGps.hpp"

namespace Simulator
{

class MockActivity : public Interface::IActivity {
public:
    MockActivity()
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        mGpsStartTimestamp = static_cast<uint32_t>(GetTickCount64());
    }

    virtual float heartRate() const override
    {
        return mHeartRate;
    }

    virtual uint32_t steps() const override
    {
        return mSteps;
    }

    virtual uint32_t floors() const override
    {
        return mFloors;
    }

    virtual float distance() const override
    {
        return mDistance;
    }

    virtual float speed() const override
    {
        return mSpeed;
    }

    virtual float elevation() const override
    {
        return mElevation;
    }

    virtual float azimuth() const override
    {
        return mAzimuth;
    }

    virtual bool location(float &lat, float &lon, float &prec) const override
    {
        lat = mLocationLat;
        lon = mLocationLon;
        prec = mLocationPrec;
        return mLocationValid;
    }

    virtual void attachCallback(Interface::IActivity::Callback *pCallback) override
    {
        mpCallback = pCallback;
    }

    virtual void detachCallback(Interface::IActivity::Callback *pCallback) override
    {
        if (mpCallback == pCallback) {
            mpCallback = nullptr;
        }
    }

    Interface::IActivity::Callback *getCallback()
    { 
        return mpCallback;
    }

    void execute() 
    { 
        // Get second interval
        ticksCnt++;
        if (ticksCnt % Gui::Config::kFrameRate == 0) {
            mSecondCnt++;
            secondPeriodicCallback();
            gpsExecute();
        }
    }

private:

    float mHeartRate = 0.0f;
    uint32_t mSteps = 0;
    uint32_t mFloors = 0;
    float mDistance = 0.0f;
    float mSpeed = 0.0f;
    float mElevation = 0.0f;
    float mAzimuth = 0.0f;
    bool mLocationValid = false;
    float mLocationLat = 0.0f;
    float mLocationLon = 0.0f;
    float mLocationPrec = 0.0f;

    Interface::IActivity::Callback *mpCallback = nullptr;
    uint32_t mSecondCnt = 0;
    uint32_t mGpsIndex = 0;

    uint32_t ticksCnt = 0;

    void secondPeriodicCallback()
    {
        uint32_t t = static_cast<uint32_t>(GetTickCount64());

        // Update step count, distance, and calories (they only increase)
        if (randomChance(0.5f)) // 5% chance to increase steps every tick
        {
            mSteps += randomInt(1, 3); // 1-3 steps
            mDistance = 0.0007f * mSteps;      // Approx. 0.7 meters per step
        }
        if (randomChance(0.05f)) // 5% chance to increase steps every tick
        {
            mFloors += randomInt(0, 1);
        }

        // Simulate heart rate oscillation with slight randomness
        mHeartRate = 75.0f + 10.0f * std::sin(t * 0.1f) + randomFloat(-1.0f, 1.0f);

        // Simulate speed (fluctuates between 0 and 15 km/h)
        mSpeed = std::max<float>(0.0f, (15.0f * std::sin(t * 0.05f) + randomFloat(-0.5f, 0.5f)) );

        // Simulate elevation (fluctuates with small noise around 100m)
        mElevation = 100.0f + 2.0f * std::sin(t * 0.02f) + randomFloat(-0.5f, 0.5f);

        mAzimuth = static_cast<float>((static_cast<uint32_t>(mAzimuth) + randomInt(-10, 10)) % 360);

        
        if (mSecondCnt % 5 == 0 && mpCallback) {
            mpCallback->onHeartRate(mHeartRate);
        }
    }



    // Fake GPS
    uint32_t mGpsStartTimestamp = 0;
    std::default_random_engine generator {std::random_device{}()};
    std::normal_distribution<float> distribution {0.0f, 0.0001f}; // Small random offset

    void gpsExecute()
    {
        // Fix in 10 seconds after start
        if (!mLocationValid && (static_cast<uint32_t>(GetTickCount64()) - mGpsStartTimestamp) > 3000) {
            mLocationValid = true;
            mLocationLat = walkTrack[0].latitude; 
            mLocationLon = walkTrack[0].longitude;
        }
        
        if (mLocationValid) {
            mLocationLat = walkTrack[mGpsIndex].latitude;
            mLocationLon = walkTrack[mGpsIndex].longitude;

            mGpsIndex++;
            if (mGpsIndex == (sizeof(walkTrack) / sizeof(walkTrack[0]))) {
                mGpsIndex = 0;
            }
            mLocationPrec = std::fabs(distribution(generator)) * 10.0f;  // Random precision
        }
    }

    float randomFloat(float min, float max) const
    {
        return min + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (max - min)));
    }

    int randomInt(int min, int max) const
    {
        return min + (std::rand() % (max - min + 1));
    }
    
    bool randomChance(float chance) const
    {
        return randomFloat(0.0f, 1.0f) < chance;
    }

};

} /* namespace Simulator */


#endif /* __SIMULATOR_KERNEL_ACTIVITY_HPP */