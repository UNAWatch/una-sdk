/**
 ******************************************************************************
 * @file    AppMemory.hpp
 * @date    04-04-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Mock for IAppMemory interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/IAppMemory.hpp"
#include <cstdio>
#include <memory>

namespace SDK::Simulator::Mock
{

class AppMemory : public SDK::Interface::IAppMemory {
public:
    AppMemory() = default;
    virtual ~AppMemory() = default;

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

} // namespace SDK::Simulator::Mock
