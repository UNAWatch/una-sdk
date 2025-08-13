/**
 ******************************************************************************
 * @file    VibroStub.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Vibro motor interface stub.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "VibroStub.hpp"

#define TAG                 "Vibro"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

bool Stub::Vibro::play(uint8_t effect)
{
    LOG_INFO("play(effect = %u)\n", effect);
    return true;
}

bool Stub::Vibro::play(Note_t melody[], uint8_t len)
{
    LOG_INFO("play(melody[%u])\n", len);
    for (uint8_t i = 0; i < len; ++i) {
        LOG_INFO("  [%u] effect=%u, loop=%u, pause=%u\n",
                 i, melody[i].effect, melody[i].loop, melody[i].pause);
    }
    return true;
}

bool Stub::Vibro::stop()
{
    LOG_INFO("called\n");
    return true;
}

bool Stub::Vibro::isPlaying()
{
    LOG_INFO("called\n");
    return false;
}
