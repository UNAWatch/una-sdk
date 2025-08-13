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

#include <cstdbool>
#include <cstdint>
#include <cstddef>
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
        MutexLock(std::shared_ptr<IMutex> mutex)
            : mMutexPtr(std::move(mutex))
            , mMutexRef(*mMutexPtr)
        {
            mMutexRef.lock();
        }

        MutexLock(IMutex& mutex)
            : mMutexPtr(nullptr)
            , mMutexRef(mutex)
        {
            mMutexRef.lock();
        }

        ~MutexLock()
        {
            mMutexRef.unLock();
        }

    private:
        std::shared_ptr<IMutex> mMutexPtr;
        IMutex&                 mMutexRef;
    };
}

#endif /* __INTERFACE_I_MUTEX_HPP */
