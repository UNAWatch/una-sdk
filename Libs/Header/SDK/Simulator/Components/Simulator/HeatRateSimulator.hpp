/**
 ******************************************************************************
 * @file    HeartRateSimulator.hpp
 * @date    28-March-2026
 * @author  Vlad Andriyash
 * @brief   HeartRateSimulator file
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __HEAT_RATE_SIMULATOR_HPP
#define __HEAT_RATE_SIMULATOR_HPP

#include <iostream>
#include <random>
#include <chrono>
#include "SDK/Simulator/Components/ISensorsSim/IHeatRate.hpp"


namespace Simulator 
{
    class HeartRateSimulator : public Interface::IHeartRate
    {
    public:

        HeartRateSimulator()
            : mMinHR(70),
            mMaxHR(150),
            mRng(std::random_device{}()),
            mNoise(-1.5, 1.5),
            mTrustNoise(0, 100)
        {
            mCurrentHR = mMinHR;
            mPrevHR = mCurrentHR;
            mRhr = mCurrentHR;
            mMaxRecordedHR = mCurrentHR;
        }

        //// IHeartRate
        virtual void setParam(uint8_t minHr, uint8_t maxHr, uint8_t typeTraining) override
        {
            mActivity = typeTraining;
            mMinHR = minHr;
            mMaxHR = maxHr;
            mCurrentHR = mMinHR;
            mPrevHR = mCurrentHR;
            mRhr = mCurrentHR;
            mMaxRecordedHR = mCurrentHR;
            mPhaseTime = 0;
            mPhase = 0;
        }

        virtual int nextHR() override
        {
            mPrevHR = mCurrentHR;

            int target = getTargetHR();

            double diff = target - mCurrentHR;

            mCurrentHR += static_cast<uint8_t>(diff * 0.10);
            mCurrentHR += static_cast<uint8_t>(mNoise(mRng));

            if (mCurrentHR < mMinHR) mCurrentHR = mMinHR;
            if (mCurrentHR > mMaxHR) mCurrentHR = mMaxHR;

            updateStats();

            mPhaseTime++;

            if (mPhaseTime > getPhaseDuration())
            {
                mPhase++;
                mPhaseTime = 0;
            }

            return (int)mCurrentHR;
        }

        virtual int getTrustLevel() override
        {
            int diff = std::abs(mCurrentHR - mPrevHR);
            int r = mTrustNoise(mRng);

            if (r < 5) return 0;
            if (diff > 10) return 1;
            if (diff > 5) return 2;

            return 3;
        }

        virtual float getRHR() override
        {
            return (float)mRhr;
        }

        virtual float getAHR() override
        {
            if (mSamples == 0) return 0;
            return (float)mSumHR / mSamples;
        }

    private:

        /**
         * @brief   Calculate target heart rate for the current activity phase.
         * @details Target HR is calculated as a percentage of the configured
         *          heart rate range [mMinHR .. mMaxHR] depending on the
         *          selected activity type and phase of the workout.
         * @retval  Target heart rate value in BPM.
         */
        int getTargetHR()
        {
            double percent = 0.5;

            //0 - Cycling, 1 - Hiking, 2 - Running
            switch (mActivity)
            {
            case 0:
                percent = cyclingProfile();
                break;
            case 1:
                percent = hikingProfile();
                break;
            case 2:
                percent = runningProfile();
                break;
            }

            return static_cast<int>(mMinHR + (mMaxHR - mMinHR) * percent);
        }

        /**
         * @brief   Cycling heart rate profile.
         * @details Returns target HR percentage depending on workout phase.
         * @retval  Percentage of heart rate range for cycling activity.
         */
        double cyclingProfile()
        {
            switch (mPhase)
            {
            case 0: return 0.50; // warmup
            case 1: return 0.65;
            case 2: return 0.75;
            case 3: return 0.85; // peak
            case 4: return 0.60; // cooldown
            }
            return 0.55;
        }

        /**
         * @brief   Running heart rate profile.
         * @details Returns target HR percentage depending on workout phase.
         * @retval  Percentage of heart rate range for running activity.
         */
        double runningProfile()
        {
            switch (mPhase)
            {
            case 0: return 0.60;
            case 1: return 0.75;
            case 2: return 0.90;
            case 3: return 0.95;
            case 4: return 0.65;
            }
            return 0.60;
        }

        /**
         * @brief   Hiking heart rate profile.
         * @details Returns target HR percentage depending on workout phase.
         * @retval  Percentage of heart rate range for hiking activity.
         */
        double hikingProfile()
        {
            switch (mPhase)
            {
            case 0: return 0.45;
            case 1: return 0.55;
            case 2: return 0.65;
            case 3: return 0.70;
            case 4: return 0.50;
            }
            return 0.50;
        }

        /**
         * @brief   Get duration of the current simulation phase.
         * @retval  Phase duration in seconds.
         */
        int getPhaseDuration()
        {
            return 10; // seconds
        }

        /**
         * @brief   Update heart rate statistics.
         * @details Updates accumulated statistics including:
         *          - average heart rate (AHR)
         *          - resting heart rate (RHR)
         *          - maximum recorded heart rate
         */
        void updateStats()
        {
            mSumHR += mCurrentHR;
            mSamples++;

            if (mCurrentHR < mRhr)
                mRhr = mCurrentHR;

            if (mCurrentHR > mMaxRecordedHR)
                mMaxRecordedHR = mCurrentHR;
        }

        uint8_t mMinHR;
        uint8_t mMaxHR;

        uint8_t mCurrentHR;
        uint8_t mPrevHR;

        int mRhr;
        int mMaxRecordedHR;

        long long mSumHR = 0;
        int mSamples = 0;

        //0 - Cycling, 1 - Hiking, 2 - Running
        uint8_t mActivity = 0;

        int mPhase = 0;
        int mPhaseTime = 0;

        std::mt19937 mRng;
        std::normal_distribution<double> mNoise;
        std::uniform_int_distribution<int> mTrustNoise;
    };
}

#endif /* __HEAT_RATE_SIMULATOR_HPP */