#pragma once

#include <cstddef>
#include <cstdint>

namespace SDK::AppSystem {

/**
 * @brief   Allocates @p size bytes aligned to @p align through @p alloc, storing
 *          the pointer @p alloc returned in the word just below the result.
 * @param   align A power of two.
 * @param   alloc Takes a byte count; returns a pointer or @c nullptr.
 * @return  @c nullptr on failure or overflow.
 */
template <typename Alloc>
void* alignedAlloc(std::size_t size, std::size_t align, Alloc alloc) noexcept
{
    const std::size_t slack = align - 1 + sizeof(void*);
    if (size > SIZE_MAX - slack) {
        return nullptr;
    }

    void* raw = alloc(size + slack);
    if (!raw) {
        return nullptr;
    }

    const std::uintptr_t mask    = ~static_cast<std::uintptr_t>(align - 1);
    const std::uintptr_t aligned = (reinterpret_cast<std::uintptr_t>(raw) + slack) & mask;
    reinterpret_cast<void**>(aligned)[-1] = raw;
    return reinterpret_cast<void*>(aligned);
}

/// @p ptr must be a non-null result of @ref alignedAlloc.
inline void* alignedRawPointer(void* ptr) noexcept
{
    return static_cast<void**>(ptr)[-1];
}

}
