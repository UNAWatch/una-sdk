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

    /**
     * @brief Push a copy of an item to the back of the queue.
     *
     * This overload copies the provided item into the queue.
     * Use this when the caller still needs to keep and modify
     * its own copy of the object after the call.
     *
     * Example usage (caller keeps its own variable):
     * @code
     * MyType msg{data};
     * queue.push(msg); // copy
     * // 'msg' and 'data' can still be used/modified
     * @endcode
     *
     * @param item: The item to copy into the queue.
     * @param timeout: Maximum wait time in ticks if the queue is full
     *                (default: infinite).
     * @return true if the item was successfully pushed, false otherwise.
     */
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

    /**
     * @brief Push an item to the back of the queue by moving it.
     *
     * This overload moves the provided item into the queue.
     * Use this when the item is no longer needed by the caller
     * after the call. This avoids copying and can eliminate
     * heap allocations (e.g. for vectors inside the item).
     *
     * Example usage (one-shot event object):
     * @code
     * MyType ev{data};
     * queue.push(std::move(ev)); // move
     * // 'ev' is now in a valid but unspecified state
     * @endcode
     *
     * @param item: The item to move into the queue.
     * @param timeout: Maximum wait time in ticks if the queue is full
     *                (default: infinite).
     * @return true if the item was successfully pushed, false otherwise.
     */
    bool push(T&& item, uint32_t timeout = 0xFFFFFFFF) 
    {
        if (!mNotFull->take(timeout)) {
            return false;
        }

        mMutex->lock();
        size_t tail = (mHead + mSize) % N;
        mBuffer[tail] = std::move(item);  // move
        ++mSize;
        mMutex->unLock();

        mNotEmpty->give();
        return true;
    }

    /**
     * @brief Pop an item from the front of the queue.
     *
     * Retrieves and removes the item at the head of the queue.
     * The value is move-assigned into @p item to avoid unnecessary copies.
     *
     * @param item: Reference where the popped item will be stored.
     * @param timeout: Maximum wait time in ticks if the queue is empty
     *                (default: infinite).
     * @return true if an item was successfully popped, false if timeout expired.
     */
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

    /**
     * @brief Push a copy of an item to the front of the queue.
     *
     * Similar to push(const T&), but places the item at the
     * front (head) of the queue, so it will be popped first.
     *
     * @param item: The item to copy into the queue.
     * @param timeout: Maximum wait time in ticks if the queue is full
     *                (default: infinite).
     * @return true if the item was successfully pushed, false otherwise.
     */
    bool pushToFront(const T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotFull->take(timeout)) {
            return false;
        }

        mMutex->lock();
        mHead = (mHead + N - 1) % N;
        mBuffer[mHead] = item;
        ++mSize;
        mMutex->unLock();

        mNotEmpty->give();

        return true;
    }

    /**
     * @brief Push an item to the front of the queue by moving it.
     *
     * Similar to push(T&&), but places the item at the front (head)
     * of the queue. Use this for urgent, one-shot events that should
     * be handled before other queued items.
     * 
     * @param item: The item to move into the queue.
     * @param timeout: Maximum wait time in ticks if the queue is full
     *                (default: infinite).
     * @return true if the item was successfully pushed, false otherwise.
     */
    bool pushToFront(T&& item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotFull->take(timeout)) {
            return false;
        }

        mMutex->lock();
        mHead = (mHead + N - 1) % N;
        mBuffer[mHead] = std::move(item);  // move
        ++mSize;
        mMutex->unLock();

        mNotEmpty->give();

        return true;
    }

    /**
     * @brief Pop an item from the back of the queue.
     *
     * Retrieves and removes the item at the tail of the queue.
     * The value is move-assigned into @p item to avoid unnecessary copies.
     *
     * @param item: Reference where the popped item will be stored.
     * @param timeout: Maximum wait time in ticks if the queue is empty
     *                (default: infinite).
     * @return true if an item was successfully popped, false if timeout expired.
     */
    bool popFromBack(T &item, uint32_t timeout = 0xFFFFFFFF)
    {
        if (!mNotEmpty->take(timeout)) {
            return false;
        }

        mMutex->lock();
        size_t tailIndex = (mHead + mSize - 1) % N;
        item = std::move(mBuffer[tailIndex]);
        mBuffer[tailIndex] = T { };
        --mSize;
        mMutex->unLock();

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
