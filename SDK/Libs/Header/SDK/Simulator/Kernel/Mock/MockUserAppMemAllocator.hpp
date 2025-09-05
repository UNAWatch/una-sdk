/**
 ******************************************************************************
 * @file    MockAppUserMemAllocator.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IUserAppMemAllocator interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_USER_MEM_ALLOCATOR_HPP
#define __SIMULATOR_KERNEL_USER_MEM_ALLOCATOR_HPP

#include "SDK/Interfaces/IUserAppMemAllocator.hpp"



namespace Simulator
{

class MockUserAppMemAllocator : public SDK::Interface::IUserAppMemAllocator {
public:
    MockUserAppMemAllocator() = default;
    virtual ~MockUserAppMemAllocator() = default;

    virtual void *malloc(size_t size) override
    { 
        void *p = std::malloc(size);
        printf("malloc %p %d bytes\n", p, size);
        return p;
    }

    virtual void free(void *ptr) override
    {
        printf("free  %p\n", ptr);
        std::free(ptr);
    }

    void* realloc(void* ptr, size_t size) override
    {
        printf("realloc  %p:%d\n", ptr, size);
        return std::realloc(ptr, size);
    }

private:

};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_USER_MEM_ALLOCATOR_HPP */