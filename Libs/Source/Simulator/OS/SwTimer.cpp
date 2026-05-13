#include "SDK/Simulator/OS/SwTimer.hpp"
#include <cstdint>
#include <chrono>

/**
  * @brief  Get the current millisecond counter value
  * @param  none
  * @retval amount of milliseconds
  */
uint32_t Driver::SwTimer::getTicks(void)
{
    static auto start = std::chrono::steady_clock::now();

    auto now = std::chrono::steady_clock::now();

    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count());
}
