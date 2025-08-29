/**
 ******************************************************************************
 * @file    Synch.hpp
 * @date    21-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SYNCH_MANAGER_HPP
#define __SYNCH_MANAGER_HPP

#include "ISynchManager.hpp"

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

    class SynchManager : public Interface::ISynchManager
    {
    public:
        virtual ~SynchManager() = default;

        std::shared_ptr<Interface::IMutex>     createMutex()                                override;
        std::shared_ptr<Interface::ISemaphore> createSemaphore(uint32_t max, uint32_t init) override;
    };

}

#endif /* __SYNCH_MANAGER_HPP */
