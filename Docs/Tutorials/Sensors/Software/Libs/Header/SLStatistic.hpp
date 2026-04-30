#ifndef __SENSOR_LAYER_STATISTIC_HPP__
#define __SENSOR_LAYER_STATISTIC_HPP__

#include "SDK/SensorLayer/SensorConnection.hpp"
#include "SDK/Timer/Timer.hpp"

#include <vector>

class SLStatistic
{
public:
    class Item {
    public:
        Item(SDK::Sensor::Connection& connection);

        SDK::Sensor::Connection& connection;
        uint32_t                 packages;      // Amount of packages that were happened after the last update
        uint32_t                 samples;       // Amount of samples that were received after the last update
    };

    SLStatistic();

    ~SLStatistic() = default;

    void registration(SDK::Sensor::Connection& connection);
    bool newTransaction(uint16_t handle, uint32_t received);
    void refresh(bool autoClear = true);

private:
    void clear();
    void show();

    const char* type2string(SDK::Sensor::Type type) const;

private:
    std::vector<Item> mItems;
    SDK::Timer        mTimer;
};

#endif
