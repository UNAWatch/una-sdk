
#pragma once

#include "SDK/Interfaces/ISettings.hpp"

namespace SDK::Simulator::Mock
{

class Settings : public SDK::Interface::ISettings {
public:

    Settings() = default;
    virtual ~Settings() = default;

    virtual bool isUnitsImperial() override
    {
        return unitsImperial;
    }

    virtual std::array<uint8_t, 4> getHrThresholds() override
    {
        return { 150, 170, 190, 210 };
    }

    virtual void getDailyGoals(uint32_t& activityMinutes, uint32_t& steps, uint32_t& floors) override
    {
        activityMinutes = 120;
        steps = 7000;
        floors = 50;
    }

    // Simulator-specific method to set the units
    void setUnitsImperial(bool imperial)
    {
        unitsImperial = imperial;
    }

private:
    bool unitsImperial = false;
};

} // namespace SDK::Simulator::Mock