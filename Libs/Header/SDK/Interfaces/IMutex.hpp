/**
 ******************************************************************************
 * @file    IMutex.hpp
 * @date    09-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects, such as mutex and semaphore.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_MUTEX_HPP
#define __INTERFACE_I_MUTEX_HPP

#include <stdio.h>

#include <cstdbool>
#include <memory>

namespace Interface {

    class IMutex
    {
    public:
        virtual ~IMutex() = default;

        virtual void lock()    = 0;
        virtual void unLock()  = 0;
        virtual bool tryLock() = 0;
    };

    class MutexLock
    {
    public:
        MutexLock(IMutex& mutex) noexcept : mMutex(mutex)
        {
            mMutex.lock();
        }

        MutexLock(const MutexLock&)            = delete;
        MutexLock& operator=(const MutexLock&) = delete;
        MutexLock(MutexLock&&)                 = delete;
        MutexLock& operator=(MutexLock&&)      = delete;

        ~MutexLock() noexcept
        {
            mMutex.unLock();
        }

    private:
        IMutex& mMutex;
    };
}

#endif /* __INTERFACE_I_MUTEX_HPP */
