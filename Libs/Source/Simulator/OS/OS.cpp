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

#include "SDK/Simulator/OS/OS.hpp"

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

std::mutex& Mutex::native() 
{ 
    return mMutex; 
}

////////////////////////////////////////////////////////////////////////////////
//// Semaphore
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize a bounded semaphore using the host platform's synchronization primitives.
 * @param max Maximum number of available permits.
 * @param init Initial permit count, which must not exceed max.
 */
Semaphore::Semaphore(uint32_t max, uint32_t init)
#ifdef _WIN32
{
    mSem = CreateSemaphore(nullptr, init, max, nullptr);
}
#elif defined(__APPLE__)
    : mMax(max), mCount(init)
{}
#else
    : mSem(), mMax(max), mCount(init)
{
    sem_init(&mSem, 0, init);
}
#endif

/** @brief Release native semaphore resources after all waiting threads have stopped. */
Semaphore::~Semaphore()
{
#ifdef _WIN32
    CloseHandle(mSem);
#elif !defined(__APPLE__)
    sem_destroy(&mSem);
#endif
}

/**
 * @brief Wait for and consume one semaphore permit.
 * @param timeout Maximum wait in milliseconds; zero polls and UINT32_MAX waits indefinitely.
 * @return True if a permit was acquired, false on timeout or a native wait failure.
 */
bool Semaphore::take(uint32_t timeout)
{
#ifdef _WIN32
    DWORD res = WaitForSingleObject(mSem, timeout);
    return res == WAIT_OBJECT_0;
#elif defined(__APPLE__)
    std::unique_lock<std::mutex> lock(mSemaphoreMutex);
    if (timeout == UINT32_MAX) {
        mSemaphoreReady.wait(lock, [this] { return mCount > 0; });
    } else if (!mSemaphoreReady.wait_for(lock, std::chrono::milliseconds(timeout), [this] { return mCount > 0; })) {
        return false;
    }
    --mCount;
    return true;
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

/** @brief Return one permit and wake a waiter without exceeding the semaphore's maximum count. */
void Semaphore::give()
{
#ifdef _WIN32
    ReleaseSemaphore(mSem, 1, nullptr);
#elif defined(__APPLE__)
    std::lock_guard<std::mutex> lock(mSemaphoreMutex);
    if (mCount < mMax) {
        ++mCount;
        mSemaphoreReady.notify_one();
    }
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
