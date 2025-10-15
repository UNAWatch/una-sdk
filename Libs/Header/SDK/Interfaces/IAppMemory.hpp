/**
 ******************************************************************************
 * @file    IAppMemory.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for the memory allocator of the user application.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstddef>

namespace SDK::Interface
{

/**
 * @brief   Interface for the memory allocator of the user application.
 */
class IAppMemory {
public:
    /**
     * @brief   Allocates a block of memory.
     * @param   size: The size of the memory block to allocate in bytes.
     * @return  A pointer to the allocated memory block, or nullptr if
     *          allocation failed.
     */
    virtual void* malloc(size_t size) = 0;

    /**
     * @brief   Frees a previously allocated memory block.
     * @param   ptr: A pointer to the memory block to free.
     */
    virtual void free(void *ptr) = 0;

    /**
     * @brief Reallocates a memory block
     *
     * @param ptr  Previously allocated pointer or nullptr.
     * @param size New block size in bytes.
     */
    virtual void* realloc(void* ptr, size_t size) = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IAppMemory() = default;
};

} // namespace SDK::Interface
