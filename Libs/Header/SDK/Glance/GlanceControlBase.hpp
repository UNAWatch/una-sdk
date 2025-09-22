/**
 ******************************************************************************
 * @file    GlanceControl.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Base class for all controls to use in Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_BASE_HPP
#define __GLANCE_CONTROL_BASE_HPP

#include "SDK/Glance/GlanceControl.h"

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>

namespace SDK::Glance {

    class Control {
    public:
        Control(std::vector<GlanceControl_t>& controls, std::size_t idx)
            : mControls(controls)
            , mIndex(idx)
        {}

        virtual ~Control() = default;

        void invalidate()
        {
            control().valid = false;
        }

    protected:
        GlanceControl_t& control() noexcept
        {
            return mControls[mIndex];
        }

    private:
        std::vector<GlanceControl_t>& mControls;
        std::size_t                   mIndex;
    };

}

#endif
