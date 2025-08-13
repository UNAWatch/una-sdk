/**
 ******************************************************************************
 * @file    MockPower.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IPower interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_POWER_HPP
#define __SIMULATOR_KERNEL_POWER_HPP

#include <cstdint>
#include <algorithm>
#include "IPower.hpp"

namespace Simulator { 

class MockPower : public Interface::IPower {
public:
    MockPower()
        : mBattLevel(73)
    {
    }

    virtual ~MockPower() = default;

    virtual float getBatteryLevel() override
    {
        return static_cast<float>(mBattLevel);
    }


    void set(float level)
    {
        mBattLevel = std::min<float>(level, 100.0f);
        mBattLevel = std::max<float>(mBattLevel, 0.0f);
    }

    void increment()
    {
        mBattLevel = std::min<float>(mBattLevel + 1, 100.0f);
    }

    void decrement()
    {
        mBattLevel = std::max<float>(mBattLevel - 1, 0.0f);
    }

private:
    float mBattLevel;

};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_POWER_HPP */