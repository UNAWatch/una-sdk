/**
 ******************************************************************************
 * @file    Synch.cpp
 * @date    21-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/SynchManager/SynchManager.hpp"
#include "SDK/Platform/OS/OS.hpp"

namespace OS {

std::shared_ptr<SDK::Interface::IMutex> SynchManager::createMutex()
{
    return std::make_shared<OS::Mutex>();
}

std::shared_ptr<SDK::Interface::ISemaphore> SynchManager::createSemaphore(uint32_t max, uint32_t init)
{
    return std::make_shared<OS::Semaphore>(max, init);
}

}
