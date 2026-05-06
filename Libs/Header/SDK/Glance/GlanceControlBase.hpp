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

#include <vector>
#include <stdlib.h>

namespace SDK::Glance {

    class Control {
    public:
        Control()
            : mControls(nullptr)
            , mIndex()
        {}

        Control(std::vector<GlanceControl_t>* controls, std::size_t idx)
            : mControls(controls)
            , mIndex(idx)
        {}

        virtual ~Control() = default;

        void invalidate()
        {
            control().valid = false;
        }

        void hide()
        {
            control().visible = false;
        }

        void show()
        {
            control().visible = true;
        }

        void setVisible(bool status)
        {
            control().visible = status;
        }

        bool isVisible()
        {
            return control().visible;
        }

    protected:
        GlanceControl_t& control() noexcept
        {
            return mControls->at(mIndex);
        }

    private:
        std::vector<GlanceControl_t>* mControls;
        std::size_t                   mIndex;
    };

}

#endif
