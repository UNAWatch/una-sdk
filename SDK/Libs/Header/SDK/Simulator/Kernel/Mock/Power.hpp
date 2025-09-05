
#pragma once

#include <cstdint>
#include <algorithm>
#include "SDK/Interfaces/IPower.hpp"

namespace Mock 
{

class Power : public SDK::Interface::IPower {
public:

    Power() = default;
    virtual ~Power() = default;

    virtual float getBatteryLevel() override
    {
        return static_cast<float>(mBattLevel);
    }

    // Simulator-specific method to set the battery level
    void set(float level)
    {
        mBattLevel = std::min<float>(level, 100.0f);
        mBattLevel = std::max<float>(mBattLevel, 0.0f);
    }

    // Simulator-specific method to increment the battery level
    void increment()
    {
        mBattLevel = std::min<float>(mBattLevel + 1, 100.0f);
    }

    // Simulator-specific method to decrement the battery level
    void decrement()
    {
        mBattLevel = std::max<float>(mBattLevel - 1, 0.0f);
    }

private:
    float mBattLevel = 73;

};

} // namespace Mock