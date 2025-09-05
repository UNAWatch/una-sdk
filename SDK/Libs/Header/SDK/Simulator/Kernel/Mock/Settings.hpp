
#pragma once

#include "SDK/Interfaces/ISettings.hpp"

namespace Mock
{

class Settings : public SDK::Interface::ISettings {
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