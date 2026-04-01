/**
 ******************************************************************************
 * @file    BacklightStub.cpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Backlight interface stub.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Simulator/Kernel/Mock/Backlight.hpp"

#define LOG_MODULE_PRX      "Mock.Backlight"
#define LOG_MODULE_LEVEL    LOG_LEVEL_INFO
#include "SDK/UnaLogger/Logger.h"

namespace SDK::Simulator::Mock {

    bool Backlight::on(uint32_t timeout)
    {
        LOG_INFO("on backlight, timeout = %u\n", timeout);
        m_isOn = true;
        if (mTimer.isActive(mTimerId)) {
            mTimer.stop(mTimerId);
        }
        mTimerId = mTimer.start(timeout, std::bind(&Backlight::timerCallback, this));
        return true;
    }

    bool Backlight::off()
    {
        LOG_INFO("off backlight\n");
        m_isOn = false;
        return true;
    }

    bool Backlight::isOn()
    {
        LOG_INFO("called\n");
        return m_isOn;
    }

    void Backlight::timerCallback()
    {
        off();
    }
}