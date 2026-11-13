#pragma once

#include <array>
#include <cstddef>
#include <optional>
#if defined(SIMULATOR)
#include <mutex>
#include <utility>
#endif

namespace SDK::Tools {

// On the watch one thread pushes and pops. The TouchGFX simulator does not:
// App::Core::runGuiComm pumps on its own thread while handleTickEvent drains
// the same queue on the SDL thread. The lock exists only in that build.
// The watch binary does not define SIMULATOR, so this stays a plain array.
template <typename T, size_t Capacity>
class FixedQueue {
    static_assert(Capacity > 0, "FixedQueue capacity must be greater than zero");

public:
#if defined(SIMULATOR)
    FixedQueue() = default;

    // Construct elements while the source is locked. Doing this in the
    // body would default-construct data_ first and then assign, so a T
    // that can be constructed but not assigned would no longer copy.
    FixedQueue(const FixedQueue& other)
        : FixedQueue(other, std::lock_guard<std::mutex>(other.mMutex)) {}

    FixedQueue& operator=(const FixedQueue& other) {
        if (this == &other) {
            return *this;
        }
        std::lock(mMutex, other.mMutex);
        std::lock_guard<std::mutex> lockA(mMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lockB(other.mMutex, std::adopt_lock);
        data_ = other.data_;
        mHead = other.mHead;
        mTail = other.mTail;
        mSize = other.mSize;
        return *this;
    }

    FixedQueue(FixedQueue&& other)
        : FixedQueue(std::move(other), std::lock_guard<std::mutex>(other.mMutex)) {}

    FixedQueue& operator=(FixedQueue&& other) {
        if (this == &other) {
            return *this;
        }
        std::lock(mMutex, other.mMutex);
        std::lock_guard<std::mutex> lockA(mMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lockB(other.mMutex, std::adopt_lock);
        data_ = std::move(other.data_);
        mHead = other.mHead;
        mTail = other.mTail;
        mSize = other.mSize;
        other.mHead = 0;
        other.mTail = 0;
        other.mSize = 0;
        return *this;
    }
#endif

    bool push(const T& value) {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        if (mSize == Capacity) return false;
        data_[mTail] = value;
        mTail = (mTail + 1) % Capacity;
        ++mSize;
        return true;
    }

    // Insert value. When the queue is full, remove the oldest element first
    // and return it. Nothing is removed when there was room. The check, the
    // removal, and the insert share one lock: full(), pop(), and push() each
    // take their own, so the other thread can drain a slot after full()
    // returns and the caller then drops an element that still had room.
    std::optional<T> pushEvictingOldest(const T& value) {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        std::optional<T> evicted;
        if (mSize == Capacity) {
            evicted = data_[mHead];
            mHead = (mHead + 1) % Capacity;
            --mSize;
        }
        data_[mTail] = value;
        mTail = (mTail + 1) % Capacity;
        ++mSize;
        return evicted;
    }

    std::optional<T> pop() {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        if (mSize == 0) return std::nullopt;
        T value = data_[mHead];
        mHead = (mHead + 1) % Capacity;
        --mSize;
        return value;
    }

    bool empty() const {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        return mSize == 0;
    }

    bool full() const {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        return mSize == Capacity;
    }

    size_t size() const {
#if defined(SIMULATOR)
        std::lock_guard<std::mutex> lock(mMutex);
#endif
        return mSize;
    }

private:
#if defined(SIMULATOR)
    FixedQueue(const FixedQueue& other, const std::lock_guard<std::mutex>&)
        : data_(other.data_)
        , mHead(other.mHead)
        , mTail(other.mTail)
        , mSize(other.mSize) {}

    FixedQueue(FixedQueue&& other, const std::lock_guard<std::mutex>&)
        : data_(std::move(other.data_))
        , mHead(other.mHead)
        , mTail(other.mTail)
        , mSize(other.mSize) {
        other.mHead = 0;
        other.mTail = 0;
        other.mSize = 0;
    }

    mutable std::mutex mMutex;
#endif
    std::array<T, Capacity> data_{};
    size_t mHead = 0;
    size_t mTail = 0;
    size_t mSize = 0;
};

}  // namespace SDK::Tools
