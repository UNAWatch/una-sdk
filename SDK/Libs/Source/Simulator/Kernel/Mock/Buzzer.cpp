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

#include "SDK/Simulator/Kernel/Mock/Buzzer.hpp"

#define TAG                 "Buzzer"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

namespace Mock {

    //TODO: Implement timeouts for play() methods.

    bool Buzzer::play()
    {
        LOG_INFO("short beep\n");
        return true;
    }

    bool Buzzer::play(Note note)
    {
        LOG_INFO("note - time = %u ms, level = %u\n", note.time, note.level);
        return true;
    }

    bool Buzzer::play(const Note melody[], uint8_t len)
    {
        LOG_INFO("melody[%u]\n", len);
        for (uint32_t i = 0; i < len; ++i) {
            LOG_INFO("[%u] time=%u ms, level=%u\n", i, melody[i].time, melody[i].level);
        }
        return true;
    }

    bool Buzzer::isPlaying()
    {
        LOG_INFO("called\n");
        return false;
    }

    void Buzzer::stop()
    {
        LOG_INFO("called\n");
    }

} // namespace Mock