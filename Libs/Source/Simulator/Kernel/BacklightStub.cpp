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

#include "BacklightStub.hpp"

#define TAG                 "Backlight"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

bool Stub::Backlight::on(uint32_t timeout)
{
    LOG_INFO("called, timeout = %u\n", timeout);
    return true;
}

bool Stub::Backlight::off()
{
    LOG_INFO("called\n");
    return true;
}

bool Stub::Backlight::isOn()
{
    LOG_INFO("called\n");
    return false;
}
