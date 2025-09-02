
#pragma once

#include "API/Settings.hpp"

namespace Mock
{

class Settings : public sdk::api::Settings {
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

    // Simulator-specific method to set the units
    void setUnitsImperial(bool imperial)
    {
        unitsImperial = imperial;
    }

private:
    bool unitsImperial = false;
};

} // namespace Mock