/**
 ******************************************************************************
 * @file    IUserAppMemAllocator.hpp
 * @date    04-02-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Interface for the memory allocator of the user application.
 *
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_USER_APP_MEM_ALLOCATOR_HPP
#define __INTERFACE_I_USER_APP_MEM_ALLOCATOR_HPP

#include <cstddef>

namespace Interface
{

/**
 * @brief   Interface for the memory allocator of the user application.
 */
class IUserAppMemAllocator {
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

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IUserAppMemAllocator() = default;
};

} /* namespace Interface */

#endif /* __INTERFACE_I_USER_APP_MEM_ALLOCATOR_HPP */

