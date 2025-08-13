/**
 ******************************************************************************
 * @file    ISynch.hpp
 * @date    09-June-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   A set of synchronization objects, such as mutex and semaphore.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_SEMAPHORE_HPP
#define __INTERFACE_I_SEMAPHORE_HPP

#include <cstdbool>
#include <cstdint>
#include <cstddef>
#include <memory>

namespace Interface {

    class ISemaphore
    {
    public:
        virtual ~ISemaphore() = default;

        virtual bool take(uint32_t timeout) = 0;
        virtual void give() = 0;
    };

}

#endif /* __INTERFACE_I_SEMAPHORE_HPP */
