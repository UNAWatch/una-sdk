/**
 ******************************************************************************
 * @file    BacklightStub.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Backlight interface stub.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "IBacklight.hpp"

namespace Stub {

/**
 * @brief   Stub implementation of Interface::IBacklight.
 */
class Backlight : public Interface::IBacklight {

public:
    bool on(uint32_t timeout = 0) override;
    bool off()                    override;
    bool isOn()                   override;
};

} // namespace Stub
