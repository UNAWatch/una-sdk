/**
 ******************************************************************************
 * @file    SwTimerPort.cpp
 * @date    17-September-2025
 * @author  Oleksandr Tymoshenko
 * @brief   Software timer porting
 *
 ******************************************************************************
 */

#include "SDK/SwTimer/SwTimer.hpp"
#include "SDK/AppSystem/AppKernel.hpp"

/// Global kernel pointer (defined elsewhere in system).
extern const IKernel* kernel;

namespace SDK
{

/**
 * @brief Get current system tick in milliseconds.
 * @return Current tick count (monotonic, wraps naturally).
 */
uint32_t SwTimer::getTicks(void)
{
    return SDK::Kernel::GetInstance().app.getTimeMs();
}

} // namespace SDK
