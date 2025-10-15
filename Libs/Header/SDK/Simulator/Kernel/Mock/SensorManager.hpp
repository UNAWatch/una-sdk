
#pragma once

#include "SDK/Interfaces/ISensorManager.hpp"

namespace SDK::Simulator::Mock
{

class SensorManager : public SDK::Interface::ISensorManager
{
public:
    SensorManager() = default;

    virtual ~SensorManager() = default;

    virtual SDK::Interface::ISensorDriver* getDefaultSensor(SDK::Sensor::Type type)
    {
        return nullptr; // Default implementation returns null
    }

    virtual std::vector<SDK::Interface::ISensorDriver*> getSensorList(SDK::Sensor::Type type)
    {
        return std::vector<SDK::Interface::ISensorDriver*>(); // Default implementation returns empty vector
    }

private:

};

} // namespace SDK::Simulator::Mock