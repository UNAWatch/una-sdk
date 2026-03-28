/**
 ******************************************************************************
 * @file    Logger.hpp
 * @date    14-July-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Logger interface implementation for simulator.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/ILogger.hpp"

#include "SDK/Simulator/OS/OS.hpp"
#include "touchgfx/Utils.hpp"

// Fix: GetTickCount64() is Windows-only. Provide a portable wrapper.
#ifndef _WIN32
#include <cstdint>
#include <time.h>
static inline uint64_t GetTickCount64()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
           static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
}
#endif

namespace SDK::Simulator::Mock
{

/**
 * @brief Implementation of SDK::Interface::ILogger.
 */
class Logger : public SDK::Interface::ILogger {
public:

    virtual void printf(const char* format, ...) override
    {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    virtual void vprintf(const char* format, va_list args) override
    {
        mvprintf(nullptr, nullptr, nullptr, 0, format, args);
    }

    virtual void mvprintf(const char* level, const char* module_name, const char* func, int line,
        const char* fmt, va_list args) override
    {
        svlog(level, module_name, func, line, fmt, args);
    }

    virtual ~Logger() = default;

private:
    static inline OS::Mutex mMutexLog;

    static void svlog(const char* level, const char* module_name, const char* func, int line,
        const char* fmt, va_list args)
    {
        OS::MutexCS cs(mMutexLog);

        uint32_t time = static_cast<uint32_t>(GetTickCount64());

        char timeBuff[16] = { 0 };
        sprintf(timeBuff, "%10u ", time);

        static char levelBuff[10]{};
        if (level) {
            sprintf(levelBuff, "-%s- ", level);
        }

        static char meta[64]{};
        int idx = 0;
        if (module_name) {
            idx += snprintf(&meta[idx], sizeof(meta) - idx, "%s::", module_name);
        }
        if (func) {
            idx += snprintf(&meta[idx], sizeof(meta) - idx, "%s", func);
        }
        if (line != 0) {
            idx += snprintf(&meta[idx], sizeof(meta) - idx, "%s%d", func ? "::" : "", line);
        }

        char userMsg[2048];
        vsnprintf(userMsg, sizeof(userMsg), fmt, args);

        if (idx) {
            touchgfx_printf("%s%s%-36s: %s", timeBuff, levelBuff, meta, userMsg);
        } else {
            touchgfx_printf("%s%s%%s", timeBuff, level, userMsg);
        } 
    }
};

} // namespace SDK::Simulator::Mock
