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

#ifndef __OS_HPP
#define __OS_HPP

#include "SDK/Interfaces/IMutex.hpp"
#include "SDK/Interfaces/ISemaphore.hpp"

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

    class Mutex : public SDK::Interface::IMutex
    {
    public:
        Mutex();
        virtual ~Mutex() = default;

        void lock()    override;
        void unLock()  override;
        bool tryLock() override;

    private:
        std::mutex mMutex;
    };

    class MutexCS
    {
    public:
        MutexCS(SDK::Interface::IMutex& mutex) : mMutex(mutex)
        {
            mMutex.lock();
        }

        ~MutexCS()
        {
            mMutex.unLock();
        }

    private:
        SDK::Interface::IMutex& mMutex;
    };

    class Semaphore : public SDK::Interface::ISemaphore {
    public:
        Semaphore(uint32_t max, uint32_t init);
        ~Semaphore();

        bool take(uint32_t timeout) override;
        void give()                 override;

    private:
    #ifdef _WIN32
        HANDLE                mSem;
    #else
        sem_t                 mSem;
        const uint32_t        mMax;
        std::atomic<uint32_t> mCount;
    #endif
    };

}

#endif /* __OS_HPP */
