/**
 ******************************************************************************
 * @file    GpsStepCounterSimulator.hpp
 * @date    28-March-2026
 * @author  Vlad Andriyash
 * @brief   Simulator GPS and Step Counter
 *
 ******************************************************************************
 */
#include "SDK/Simulator/Components/Simulator/GpsStepCounterSimulator.hpp"
#include "SDK/SensorLayer/DataParsers/SensorDataParserGpsLocation.hpp"
#include <cmath>
#include <chrono>
#include <random>
#include <SDK/Simulator/Kernel/Mock/System.hpp>

#define DATA_SAMPLE_COUNT  5

namespace Simulator {

    GpsStepCounterSimulator::GpsStepCounterSimulator()
        : mGen(std::random_device{}()),
        mTimerGpsFix(),
        mTimer(),
        mGpsNoise(0.0f, 1.5f),
        mAltNoise(0.0f, 0.5f),
        mSpeedNoise(0.0f, 0.05f),
        mPrecisionNoise(0.0f, 1.0f),
        mGpsLoss(0.0f, 1.0f),
        mCurrentSpeed(mBaseSpeed),
        mTotalDistanceMeters(0.0f),
        mDriftX(0), mDriftY(0), mBaseAlt(250.0f), mAltDrift(0),
        mDataSample(DATA_SAMPLE_COUNT),
        mRunning(false),
        mPeriodSendGpsData(990),
        mTrackPosition(0.0),
        mLoc{}
    {
        mStart = std::chrono::steady_clock::now();
        mLastTime = mStart;
        mTimerGpsFix.start(5000);
        mTimer.start(mPeriodSendGpsData);
        mThread = std::thread(&GpsStepCounterSimulator::task, this);
    }

    GpsStepCounterSimulator::~GpsStepCounterSimulator()
    {
        mTimerGpsFix.stop();
        mTimer.stop();
        if (mThread.joinable())
            mThread.join();
    }

    bool GpsStepCounterSimulator::enable()
    {
        mRunning = true;
        return true;
    }

    bool GpsStepCounterSimulator::disable()
    {
        mRunning = false;

        return true;
    }

    void GpsStepCounterSimulator::setPeriod(uint32_t seconds)
    {
        mPeriodSendGpsData = seconds;
        mTimer.start(mPeriodSendGpsData);
    }

    void GpsStepCounterSimulator::task()
    {
        static bool flagGpsFix = false;

        while (1)
        {
            if (!SDK::Simulator::Mock::SystemGUI::isAppRunning()) {
                return;
            }

            if (mTimerGpsFix.check()) {
                mTimerGpsFix.stop();
                flagGpsFix = true;
            }

            if (flagGpsFix == false) {
                continue;
            }

            if (!mTimer.check()) {
                continue;
            }

            updateLocation();
            IGps::LocationInfo loc = getLocation();
        }
    }
    
    IGps::LocationInfo GpsStepCounterSimulator::getLocation()
    {
        return mLoc;
    }

    void GpsStepCounterSimulator::updateLocation()
    {
        // Calculate time delta since last update
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - mLastTime).count();
        mLastTime = now;

        // Determine current segment of the track
        float distanceAlongLap = mTrackPosition;

        // Determine speed factor based on track segment (straight / curve)
        float speedFactor = getSpeedFactor(distanceAlongLap);

        // Update current speed smoothly with random jitter
        updateSpeed(speedFactor);

        // Compute step distance based on speed and elapsed time
        float step = mCurrentSpeed * dt;

        // Update total distance traveled
        mTotalDistanceMeters = step;

        // Move position along track
        mTrackPosition += step;

        updateStepCounter(step);

        // Wrap around lap if necessary
        if (mTrackPosition > mLapLength)
            mTrackPosition = std::fmod(mTrackPosition, mLapLength);

        float distance = mTrackPosition;

        // Compute X/Y coordinates on track
        float x = 0.0f;
        float y = 0.0f;

        if (distance < mStraight) // first straight
        {
            x = distance;
            y = mRadius;
        }
        else if (distance < mStraight + mCurveLen) // first curve
        {
            float d = distance - mStraight;
            float angle = d / mRadius;

            x = mStraight + mRadius * std::sin(angle);
            y = mRadius * std::cos(angle);
        }
        else if (distance < 2 * mStraight + mCurveLen) // second straight
        {
            float d = distance - (mStraight + mCurveLen);
            x = mStraight - d;
            y = -mRadius;
        }
        else // second curve
        {
            float d = distance - (2 * mStraight + mCurveLen);
            float angle = d / mRadius;
            x = -mRadius * std::sin(angle);
            y = -mRadius * std::cos(angle);
        }

        // Add GPS drift and noise
        mDriftX += mGpsNoise(mGen) * 0.05f;
        mDriftY += mGpsNoise(mGen) * 0.05f;

        x += mGpsNoise(mGen) + mDriftX;
        y += mGpsNoise(mGen) + mDriftY;

        // Convert meters to GPS degrees
        const float metersPerDegLat = 111320.0f;
        const float metersPerDegLon = static_cast<float>(metersPerDegLat * std::cos(mCenterLat * M_PI / 180.0f));

        LocationInfo loc;

        // Simulate occasional GPS signal loss
        if (mGpsLoss(mGen) < 0.02f)
        {
            loc.valid = false;
            mLoc = loc;
        }

        loc.valid = true;

        loc.lat = mCenterLat + y / metersPerDegLat;
        loc.lon = mCenterLon + x / metersPerDegLon;

        // Simulate altitude with small random drift
        float altitudeVariation = static_cast<float>(0.5f * std::sin(mTrackPosition / mLapLength * 2.0f * M_PI));
        mAltDrift += mAltNoise(mGen) * 0.02f;
        loc.alt = mBaseAlt + altitudeVariation + mAltDrift + mAltNoise(mGen);

        // Simulate measurement precision (accuracy)
        float p = 3.0f + std::fabs(mGpsNoise(mGen));
        if (mPrecisionNoise(mGen) < 0.05f)
            p += 10.0f;
        loc.precision = p;

        mLoc = loc;
  
    }

    void GpsStepCounterSimulator::setParamSimulation(float speedMin, float speedMidle, float speedMax, uint32_t seachSatteliteMs)
    {
        mMinSpeed = speedMin / 3.6f;
        mBaseSpeed  = speedMidle / 3.6f;
        mMaxSpeed = speedMax / 3.6f;

        mTimerGpsFix.stop();
        mTimerGpsFix.start(seachSatteliteMs*1000);
    }

    bool GpsStepCounterSimulator::init()
    {
        return true;
    }

    bool GpsStepCounterSimulator::deinit()
    {
        return true;
    }

    time_t GpsStepCounterSimulator::getTime()
    {
        auto now = std::chrono::system_clock::now();
        time_t t = std::chrono::system_clock::to_time_t(now);
        return t;
    }

    bool GpsStepCounterSimulator::hasFix()
    {
        return mLoc.valid;
    }

    bool GpsStepCounterSimulator::isEnabled()
    {
        return mRunning;
    }

    float GpsStepCounterSimulator::getSpeed()
    {
        return mCurrentSpeed;
    }

    float GpsStepCounterSimulator::getDistance()
    {
        return mTotalDistanceMeters;
    }

    float GpsStepCounterSimulator::getAltitude()
    {
        return mLoc.alt;
    }

    void GpsStepCounterSimulator::startStepCounter()
    {
    }

    void GpsStepCounterSimulator::stopStepCounter()
    {
    }

    void GpsStepCounterSimulator::setParamStepCounter(float strideLength)
    {
        mStrideLength = strideLength;
    }

    uint32_t GpsStepCounterSimulator::getStepCounter()
    {
        return mTotalSteps;
    }

    float GpsStepCounterSimulator::getSpeedFactor(float distanceAlongLap)
    {
        if (distanceAlongLap < mStraight) return 1.0f;
        else if (distanceAlongLap < mStraight + mCurveLen) return 0.75f;
        else if (distanceAlongLap < 2 * mStraight + mCurveLen) return 1.0f;
        else return 0.75f;
    }

    void GpsStepCounterSimulator::updateSpeed(float factor)
    {
        float targetSpeed = mBaseSpeed * factor;
        float delta = targetSpeed - mCurrentSpeed;

        mCurrentSpeed += delta * 0.1f;
        mCurrentSpeed += mSpeedNoise(mGen);

        if (mCurrentSpeed < mMinSpeed) mCurrentSpeed = mMinSpeed;
        if (mCurrentSpeed > mMaxSpeed) mCurrentSpeed = mMaxSpeed;
    }

    uint16_t GpsStepCounterSimulator::calcStride(uint16_t fieldCount)
    {
        assert(fieldCount > 0);

        const std::size_t headerSize = sizeof(SDK::Sensor::Data); // with mValue[1] inside
        const std::size_t extraFields = (fieldCount - 1) * sizeof(SDK::Sensor::Data::Field);

        return static_cast<uint16_t>(headerSize + extraFields);
    }

    void GpsStepCounterSimulator::updateStepCounter(float distance)
    {
        mStepAccumulator += distance;

        while (mStepAccumulator >= mStrideLength)
        {
            mTotalSteps++;
            mStepAccumulator -= mStrideLength;
        }

    }
}