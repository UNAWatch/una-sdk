
#pragma once

#include "Interfaces/ISensorManager.hpp"

namespace Mock 
{

class SensorManager : public Interface::ISensorManager
{
public:
    SensorManager() = default;

    virtual ~SensorManager() = default;

    virtual Interface::ISensorDriver* getDefaultSensor(Sensor::Type type)
    {
        return nullptr; // Default implementation returns null
    }

    virtual std::vector<Interface::ISensorDriver*> getSensorList(Sensor::Type type)
    {
        return std::vector<Interface::ISensorDriver*>(); // Default implementation returns empty vector
    }

private:

};

} /* namespace Mock */