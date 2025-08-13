/**
 ******************************************************************************
 * @file    MockSettings.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for ISettings interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_SETTINGS_HPP
#define __SIMULATOR_KERNEL_SETTINGS_HPP

#include "ISettings.hpp"

namespace Simulator
{

class MockSettings : public Interface::ISettings {
public:
    MockSettings()
        : unitsImperial(false)
    {
    }

    virtual ~MockSettings() = default;

    virtual bool isUnitsImperial() override
    {
        return unitsImperial;
    }

    void setUnitsImperial(bool imperial)
    {
        unitsImperial = imperial;
    }


private:
    bool unitsImperial;
};


} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_SETTINGS_HPP */