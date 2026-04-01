/**
 ******************************************************************************
 * @file    Backlight.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Backlight interface stub.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/IBacklight.hpp"
#include "SDK/Simulator/OS/OneShotTimer.hpp"

namespace SDK::Simulator::Mock
{

/**
 * @brief   Stub implementation of Interface::IBacklight.
 */
class Backlight : public SDK::Interface::IBacklight {

public:
    bool on(uint32_t timeout = 0) override;
    bool off()                    override;
    bool isOn()                   override;
private:
    void timerCallback();

    bool m_isOn = false;
    OS::OneShotTimer&  mTimer = OS::OneShotTimer::getInstance();
    OS::OneShotTimer::TimerId mTimerId = 0;
};

} // namespace SDK::Simulator::Mock
