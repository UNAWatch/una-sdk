/**
 ******************************************************************************
 * @file    GpsStepCounterSimulator.hpp
 * @date    28-March-2026
 * @author  Vlad Andriyash
 * @brief   Simulator GPS and Step Counter
 *
 ******************************************************************************
 */

#ifndef __GPS_SIMULATOR_HPP
#define __GPS_SIMULATOR_HPP

#include "SDK/Simulator/Components/ISensorsSim/IGps.hpp"
#include "SDK/Simulator/Components/ISensorsSim/IStepCounter.hpp"
#include "SDK/Simulator/Components/SensorDataSample.hpp"
#include "SDK/Simulator/OS/SwTimer.hpp"
#include <cstdint>
#include <chrono>
#include <random>
#include <thread>
#include <atomic>

using namespace Interface;

namespace Simulator
{
    class GpsStepCounterSimulator : public Interface::IGps, public Interface::IStepCounter
    {
    public:
        GpsStepCounterSimulator();
        ~GpsStepCounterSimulator();

        //// IGps
        virtual void setParamSimulation(float speedMin, float speedMidle, float speedMax, uint32_t seachSatteliteMs) override;

        virtual bool init() override;

        virtual bool deinit() override;

        virtual bool enable() override;

        virtual bool disable() override;

        virtual bool isEnabled() override;

        virtual void setPeriod(uint32_t seconds) override;

        virtual bool hasFix() override;

        virtual time_t getTime() override;

        virtual IGps::LocationInfo getLocation() override;

        virtual float getSpeed() override;

        virtual float getDistance() override;

        virtual float getAltitude() override;

        //// 
        virtual void startStepCounter() override;

        virtual void stopStepCounter() override;

        virtual void setParamStepCounter(float strideLength) override;

        virtual uint32_t getStepCounter() override;

    private:

        void task();

        void updateLocation();

        /// Calculates the speed factor based on the current segment of the track
        float getSpeedFactor(float distanceAlongLap);

        /// Updates the current speed considering the track segment and random jitter
        void updateSpeed(float factor);

        void updateStepCounter(float distance);

        uint16_t calcStride(uint16_t fieldCount);

        Sensor::DataSample           mDataSample;
        std::thread mThread;
        std::atomic<bool> mRunning;
        uint32_t mPeriodSendGpsData;
        IGps::LocationInfo mLoc;
        ::Driver::SwTimer  mTimerGpsFix;
        ::Driver::SwTimer  mTimer;

        //Step Counter
        float mStrideLength = 0.65f;     // meters per step
        uint32_t mTotalSteps = 0;
        float mStepAccumulator = 0.0f;

        // Track geometry
        const float mCenterLat = 49.2331f; ///< Track center latitude
        const float mCenterLon = 28.4682f; ///< Track center longitude
        const float mStraight = 84.39f;    ///< Length of a straight segment (meters)
        const float mRadius = 36.5f;       ///< Radius of curved segment (meters)
        const float mCurveLen = 3.14159265f * mRadius; ///< Length of a semicircle (meters)
        const float mLapLength = 2 * mStraight + 2 * mCurveLen; ///< Total lap length (meters)

        // Speed
        float mBaseSpeed = 25.0f / 3.6f; ///< Middlle speep (m/s)
        float mMinSpeed = 20.0f / 3.6f;   ///< Minimum speed (m/s)
        float mMaxSpeed = 34.0f / 3.6f;  ///< Maximum speed (m/s)
        float mCurrentSpeed;                    ///< Current simulated speed
        float mTrackPosition;

        // Distance tracking
        float mTotalDistanceMeters; ///< Total distance traveled (meters)

        // Timing
        std::chrono::steady_clock::time_point mStart;    ///< Simulation start time
        std::chrono::steady_clock::time_point mLastTime; ///< Last update time

        // Drift for GPS simulation
        float mDriftX;
        float mDriftY;
        float mBaseAlt;
        float mAltDrift;

        // Random number generators
        std::default_random_engine mGen;
        std::normal_distribution<float> mGpsNoise;       ///< Noise for latitude/longitude
        std::normal_distribution<float> mAltNoise;       ///< Noise for altitude
        std::normal_distribution<float> mSpeedNoise;     ///< Random variation of speed
        std::uniform_real_distribution<float> mGpsLoss;  ///< Simulate GPS signal loss
        std::uniform_real_distribution<float> mPrecisionNoise; ///< Noise for precision

    };
}

#endif /* __GPS_SIMULATOR_HPP */