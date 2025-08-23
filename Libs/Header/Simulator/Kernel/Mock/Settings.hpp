
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

    // Simulator-specific method to set the units
    void setUnitsImperial(bool imperial)
    {
        unitsImperial = imperial;
    }

private:
    bool unitsImperial = false;
};

} // namespace Mock