
#include "SDK/Simulator/Kernel/Mock/System.hpp"
#include <cstdint>

// Fix: GetTickCount64() and Sleep() are Windows-only. Provide portable replacements.
#ifndef _WIN32
#include <cstdint>
#include <time.h>
#include <unistd.h>
static inline uint64_t GetTickCount64()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000ULL +
           static_cast<uint64_t>(ts.tv_nsec) / 1000000ULL;
}
static inline void Sleep(uint32_t ms)
{
    usleep(ms * 1000U);
}
#endif

#define LOG_MODULE_PRX      "Mock.System"
#define LOG_MODULE_LEVEL    LOG_LEVEL_DEBUG
#include "SDK/UnaLogger/Logger.h"

namespace SDK::Simulator::Mock
{
    bool SystemGUI::mAppRunning = true;

    System::System()
    {
        GetTimeMs();

    }

    uint32_t System::GetTimeMs()
    {
        static auto start = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
    }

    ////////////////////////////////////
	//// SystemGUI implementation
    ////////////////////////////////////

    bool SystemGUI::isAppRunning()
    {
        return mAppRunning;
	}

    void SystemGUI::exit(int status)
    {
        LOG_DEBUG("status = %d\n", status);

        mAppRunning = false;

        static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->stopApplication();
    }

    uint32_t SystemGUI::getTimeMs()
    {
        return System::GetTimeMs();
    }

    void SystemGUI::delay(uint32_t ms)
    {
        Sleep(ms);
    }

    void SystemGUI::yield()
    {}

    ////////////////////////////////////
    //// SystemService implementation
    ////////////////////////////////////

    bool SystemService::isAppRunning() const
    {
        return mAppRunning;
    }

    void SystemService::exit(int status)
    {
        LOG_DEBUG("status = %d\n", status);

        mAppRunning = false;
    }

    uint32_t SystemService::getTimeMs()
    {
        return System::GetTimeMs();
    }

    void SystemService::delay(uint32_t ms)
    {
        Sleep(ms);
    }

    void SystemService::yield()
    {}

} // namespace SDK::Simulator::Mock