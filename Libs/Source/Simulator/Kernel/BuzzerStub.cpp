/**
 ******************************************************************************
 * @file    BuzzerStub.cpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Buzzer interface stub.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "BuzzerStub.hpp"

#define TAG                 "Buzzer"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

bool Stub::Buzzer::play()
{
    LOG_INFO("short beep\n");
    return true;
}

bool Stub::Buzzer::play(Note_t note)
{
    LOG_INFO("note - time = %u ms, level = %u\n", note.time, note.level);
    return true;
}

bool Stub::Buzzer::play(const Note_t melody[], uint32_t len)
{
    LOG_INFO("melody[%u]\n", len);
    for (uint32_t i = 0; i < len; ++i) {
        LOG_INFO("[%u] time=%u ms, level=%u\n", i, melody[i].time, melody[i].level);
    }
    return true;
}

bool Stub::Buzzer::isPlaying()
{
    LOG_INFO("called\n");
    return false;
}

void Stub::Buzzer::stop()
{
    LOG_INFO("called\n");
}
