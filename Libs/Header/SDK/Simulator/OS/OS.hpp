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
    #include <array>
#else
    #include <semaphore.h>
    #include <ctime>
    #include <atomic>
    #include <array>  // Fix: <array> was missing in Linux build (std::array used by Queue<T,N>)
#endif

namespace OS {

    inline void Delay(uint32_t ms)
	{
        #ifdef _WIN32
    		Sleep(ms);  
        #else
            struct timespec ts;
            ts.tv_sec  = ms / 1000;
            ts.tv_nsec = (ms % 1000) * 1000000;
            nanosleep(&ts, nullptr);
        #endif
	}

    class Mutex : public SDK::Interface::IMutex
    {
    public:
        Mutex();
        virtual ~Mutex() = default;

        void lock()    override;
        void unLock()  override;
        bool tryLock() override;

        //For condition_variable
        std::mutex& native();

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

        virtual ~MutexCS()
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

    template<typename T, size_t N>
    class Queue
    {
    public:
        using ValueType = T;

        bool init(const char* /*name*/ = nullptr)
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            mInited = true;
            mHead = mTail = mCount = 0;
            return true;
        }

        void deinit()
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            mInited = false;
            mHead = mTail = mCount = 0;
        }

        constexpr size_t capacity() const
        {
            return N;
        }

        bool push(const T& item)
        {
            std::lock_guard<std::mutex> lock(mMutex.native());

            if (!mInited || mCount == N) {
                return false;
            }

            mBuffer[mTail] = item;
            mTail = (mTail + 1) % N;
            ++mCount;

            mCondVar.notify_one();
            return true;
        }

        bool pop(T& item, uint32_t timeoutMs = 0)
        {
            std::unique_lock<std::mutex> lock(mMutex.native());

            if (!mInited)
                return false;

            if (timeoutMs == UINT32_MAX)
            {
                mCondVar.wait(lock, [&] {return mCount > 0 || !mInited;});
            }
            else
            {
                if (!mCondVar.wait_for(lock,std::chrono::milliseconds(timeoutMs),[&] { return mCount > 0 || !mInited; })){
                    return false; // timeout
                }
            }

            if (!mInited || mCount == 0) {
                return false;
            }

            item = mBuffer[mHead];
            mHead = (mHead + 1) % N;
            --mCount;

            return true;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            return mCount == 0;
        }

        uint32_t size() const
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            return static_cast<uint32_t>(mCount);
        }

        uint32_t available() const
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            return static_cast<uint32_t>(N - mCount);
        }

        void clear()
        {
            std::lock_guard<std::mutex> lock(mMutex.native());
            mHead = mTail = mCount = 0;
        }

    private:
        mutable OS::Mutex mMutex;
        std::condition_variable mCondVar;
        std::array<T, N>  mBuffer{};
        size_t            mHead{0};
        size_t            mTail{0};
        size_t            mCount{0};
        bool              mInited{false};
    };
}

#endif /* __OS_HPP */
