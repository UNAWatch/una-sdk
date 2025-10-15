/**
 ******************************************************************************
 * @file    AppCapabilities.hpp
 * @date    14-July-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   AppCapabilities interface implementation for simulator.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/IAppCapabilities.hpp"

namespace SDK::Simulator::Mock
{

/**
 * @brief Implementation of SDK::Interface::IAppCapabilities.
 */
class AppCapabilities : public SDK::Interface::IAppCapabilities {
public:

    virtual void enablePhoneNotification(bool enable) override {}

    virtual void enableMusicControl(bool enable) override {}

    virtual void enableUsbCharging(bool enable) override {}

    virtual void notifyActivityEnd() override {}

    virtual ~AppCapabilities() = default;

private:

};

} // namespace SDK::Simulator::Mock
