/**
 ******************************************************************************
 * @file    SynchManager.hpp
 * @date    21-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/ISynchManager.hpp"

#include <mutex>
#include <memory>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <semaphore.h>
    #include <ctime>
    #include <atomic>
#endif

namespace OS {

class SynchManager : public SDK::Interface::ISynchManager
{
public:
    virtual ~SynchManager() = default;

    virtual std::shared_ptr<SDK::Interface::IMutex>     createMutex()                                override;
    virtual std::shared_ptr<SDK::Interface::ISemaphore> createSemaphore(uint32_t max, uint32_t init) override;
};

} // namespace OS
