/**
 ******************************************************************************
 * @file    ISensorCore.hpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorCore interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

namespace SDK::Simulator::Sensors {

class ISensorCore
{
public:
    virtual ~ISensorCore() = default;

    virtual void tick() = 0;
};

} // namespace SDK::Simulator::Sensors
