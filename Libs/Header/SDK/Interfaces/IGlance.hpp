/**
 ******************************************************************************
 * @file    IGlance.hpp
 * @date    19-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Glance application's interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Glance/GlanceControl.h"

#include <cstdint>
#include <cstdbool>

namespace SDK::Interface {

class IGlance {
public:
    virtual ~IGlance() = default;

    struct Info {
        uint8_t count;
        GlanceControl_t* ctrls;
        const char* altname;
    };

    virtual Info glanceGetInfo() = 0;
    virtual void glanceUpdate()  = 0;
    virtual void glanceClose()   = 0;
};

} // namespace SDK::Interface
