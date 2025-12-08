/**
 ******************************************************************************
 * @file    ISystem.hpp
 * @date    06-12-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   System interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <cstdbool>


namespace SDK::Interface
{

class ISystem
{
public:

    /**
     * @brief Terminates the application.
     *
     * When this method is called, no additional events will be sent.
     * After execution, the application is removed from memory.
     *
     * @param status Exit status (0 for normal exit, < 0 for errors).
     * @note No return.
     */
    virtual void exit(int status = 0) = 0;

    /**
     * @brief Get the current time in milliseconds.
     * @retval The current time in milliseconds.
     */
    virtual uint32_t getTimeMs() = 0;

    /**
     * @brief Delay for a specified amount of time.
     * @param ms Number of milliseconds to delay.
     */
    virtual void delay(uint32_t ms) = 0;

    /**
     * @brief Yield execution to the kernel.
     * back to the kernel.
     */
    virtual void yield() = 0;

protected:

    /**
     * @brief Virtual destructor.
     */
    virtual ~ISystem() = default;

};

} // namespace SDK::Interface
