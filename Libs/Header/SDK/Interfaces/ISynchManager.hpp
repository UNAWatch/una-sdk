/**
 ******************************************************************************
 * @file    ISynchManager.hpp
 * @date    09-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects, such as mutex and semaphore.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/IMutex.hpp"
#include "SDK/Interfaces/ISemaphore.hpp"

#include <cstdbool>
#include <cstdint>
#include <cstddef>
#include <memory>

namespace SDK::Interface {

class ISynchManager
{
public:
    virtual ~ISynchManager() = default;

    virtual std::shared_ptr<SDK::Interface::IMutex>     createMutex()                                = 0;    // create a mutex
    virtual std::shared_ptr<SDK::Interface::ISemaphore> createSemaphore(uint32_t max, uint32_t init) = 0;    // create a semaphore
};

} // namespace SDK::Interface
