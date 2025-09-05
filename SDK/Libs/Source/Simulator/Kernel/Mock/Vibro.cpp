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

#include "SDK/Simulator/Kernel/Mock/Vibro.hpp"

#define TAG                 "Vibro"
#define LOG_MODULE_PRX      TAG"::"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "Logger.h"

namespace Mock {

    // TODO: Implement timeouts for play() methods.

    void Vibro::play(uint8_t effect)
    {
        LOG_INFO("play(effect = %u)\n", effect);
        return;
    }

    void Vibro::play(const Note melody[], uint8_t len, uint8_t loop)
    {
        LOG_INFO("play(melody[%u])\n", len);
        for (uint8_t i = 0; i < len; ++i) {
            LOG_INFO("  [%u] effect=%u, loop=%u, pause=%u\n",
                i, melody[i].effect, melody[i].loop, melody[i].pause);
        }
        return;
    }

    void Vibro::stop()
    {
        LOG_INFO("called\n");
        return;
    }

    bool Vibro::isPlaying()
    {
        LOG_INFO("called\n");
        return false;
    }

}