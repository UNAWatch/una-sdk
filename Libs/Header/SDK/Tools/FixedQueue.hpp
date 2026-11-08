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
public:
#if defined(SIMULATOR)
    FixedQueue() = default;

    FixedQueue(const FixedQueue& other) {
        std::lock_guard<std::mutex> lock(other.mMutex);
        copyFrom(other);
    }

    FixedQueue& operator=(const FixedQueue& other) {
        if (this == &other) {
            return *this;
        }
        std::lock(mMutex, other.mMutex);
        std::lock_guard<std::mutex> lockA(mMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lockB(other.mMutex, std::adopt_lock);
        copyFrom(other);
        return *this;
    }

    FixedQueue(FixedQueue&& other) {
        std::lock_guard<std::mutex> lock(other.mMutex);
        takeFrom(other);
    }

    FixedQueue& operator=(FixedQueue&& other) {
        if (this == &other) {
            return *this;
        }
        std::lock(mMutex, other.mMutex);
        std::lock_guard<std::mutex> lockA(mMutex, std::adopt_lock);
        std::lock_guard<std::mutex> lockB(other.mMutex, std::adopt_lock);
        takeFrom(other);
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
    void copyFrom(const FixedQueue& other) {
        data_ = other.data_;
        mHead = other.mHead;
        mTail = other.mTail;
        mSize = other.mSize;
    }

    void takeFrom(FixedQueue& other) {
        data_ = std::move(other.data_);
        mHead = other.mHead;
        mTail = other.mTail;
        mSize = other.mSize;
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
