/**
 ******************************************************************************
 * @file    OS.cpp
 * @date    21-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#include "SDK/Platform/OS/OS.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sched.h>
#endif

namespace OS {

//void Yield()
//{
//#if defined(_WIN32)
//        ::SwitchToThread();
//#elif defined(__linux__)
//        ::sched_yield();
//#else
//#error "Yield not implemented for this platform"
//#endif
//}

////////////////////////////////////////////////////////////////////////////////
//// Mutex
////////////////////////////////////////////////////////////////////////////////

Mutex::Mutex() : mMutex()
{}

void Mutex::lock()
{
    mMutex.lock();
}

void Mutex::unLock()
{
    mMutex.unlock();
}

bool Mutex::tryLock()
{
    return mMutex.try_lock();
}

////////////////////////////////////////////////////////////////////////////////
//// Semaphore
////////////////////////////////////////////////////////////////////////////////

Semaphore::Semaphore(uint32_t max, uint32_t init)
#ifdef _WIN32
{
    mSem = CreateSemaphore(nullptr, init, max, nullptr);
}
#else
    : mMax(max), mCount(init), mSem()
{
    sem_init(&mSem, 0, init);
}
#endif

Semaphore::~Semaphore()
{
#ifdef _WIN32
    CloseHandle(mSem);
#else
    sem_destroy(&mSem);
#endif
}

bool Semaphore::take(uint32_t timeout)
{
#ifdef _WIN32
    DWORD res = WaitForSingleObject(mSem, timeout);
    return res == WAIT_OBJECT_0;
#else
    int result;
    if (timeout == (uint32_t)-1) {
        result = sem_wait(&mSem);
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout / 1000;
        ts.tv_nsec += (timeout % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        result = sem_timedwait(&mSem, &ts);
    }

    if (result == 0) {
        mCount--;
        return true;
    }

    return false;
#endif
}

void Semaphore::give()
{
#ifdef _WIN32
    ReleaseSemaphore(mSem, 1, nullptr);
#else
    uint32_t old = mCount.load();
    while (true) {
        if (old >= mMax) {
            return;
        }

        if (mCount.compare_exchange_weak(old, old + 1)) {
            sem_post(&mSem);
            return;
        }
    }
#endif
}

}
