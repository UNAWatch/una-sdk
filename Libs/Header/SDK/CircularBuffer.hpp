/**
 ******************************************************************************
 * @file    OS.hpp
 * @date    15-06-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   OS CPP wrappers.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __CIRCULAR_BUFFER_HPP
#define __CIRCULAR_BUFFER_HPP

#include "Interfaces/IKernel.hpp"

#include <array>
#include <memory>

template<typename T, size_t N>
class CircularBuffer {
public:
    using ValueType = T;

    CircularBuffer(const IKernel& kernel)
        : mKernel(kernel)
        , mMutex{}
        , mNotEmpty{}
        , mNotFull{}
        , mBuffer{}
        , mHead(0)
        , mSize(0)
    {}

    virtual ~CircularBuffer()
    {
        deinit();
    }

    bool init()
    {
        mMutex    = mKernel.synchManager.createMutex();
        mNotEmpty = mKernel.synchManager.createSemaphore(N, 0);
        mNotFull  = mKernel.synchManager.createSemaphore(N, N);

        return mMutex && mNotEmpty && mNotFull;
    }

    void deinit()
    {
        mMutex.reset();
        mNotEmpty.reset();
        mNotFull.reset();
    }

    size_t capacity() const
    {
        return N;
    }

    bool push(const T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotFull->take(timeout)) {
            return false;
        }

        mMutex->lock();
        size_t tail = (mHead + mSize) % N;
        mBuffer[tail] = item;
        ++mSize;
        mMutex->unLock();

        mNotEmpty->give();

        return true;
    }

    bool pop(T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotEmpty->take(timeout)) {
            return false;
        }

        mMutex->lock();
        item = std::move(mBuffer[mHead]);
        mBuffer[mHead] = T {};
        mHead = (mHead + 1) % N;
        --mSize;

        mMutex->unLock();
        mNotFull->give();

        return true;
    }

    bool pushToFront(const T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotFull->take(timeout)) {
            return false;
        }

        mMutex->lock();
        // Обчислюємо нову позицію head
        mHead = (mHead + N - 1) % N;
        mBuffer[mHead] = item;
        ++mSize;
        mMutex->unLock();

        mNotEmpty->give();

        return true;
    }

    bool popFromBack(T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotEmpty->take(timeout)) {
            return false;
        }

        mMutex->lock();
        // Обчислюємо позицію хвоста
        size_t tailIndex = (mHead + mSize - 1) % N;
        item = std::move(mBuffer[tailIndex]);
        mBuffer[tailIndex] = T { };
        --mSize;

        mMutex->lock();
        mNotFull->give();

        return true;
    }

    bool empty() const
    {
        Interface::MutexLock lock(mMutex);

        return (mSize == 0);
    }

    size_t size() const
    {
        Interface::MutexLock lock(mMutex);

        return mSize;
    }

    uint32_t available() const
    {
        Interface::MutexLock lock(mMutex);

        return (N - mSize);
    }

    void clear()
    {
        Interface::MutexLock lock(mMutex);

        for (size_t i = 0; i < N; ++i) {
            mBuffer[i] = T{};
        }
        mHead = 0;
        mSize = 0;

        // Clear all semaphores:
        // mNotEmpty: reduce to 0
        for (;;) {
            if (mNotEmpty->take(0))
                break;
        }

        // mNotFull: increase to N
        for (int i = 0; i < N; ++i) {
            mNotFull->give();
        }
    }

private:
    const IKernel&                         mKernel;
    std::shared_ptr<Interface::IMutex>     mMutex;
    std::shared_ptr<Interface::ISemaphore> mNotEmpty;
    std::shared_ptr<Interface::ISemaphore> mNotFull;
    std::array<T, N>                       mBuffer;
    size_t                                 mHead;
    size_t                                 mSize;
};

#endif /* __CIRCULAR_BUFFER_HPP */
