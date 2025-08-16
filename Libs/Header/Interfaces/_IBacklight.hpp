/**
 ******************************************************************************
 * @file    IBacklight.hpp
 * @date    14-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Backlight Interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_BACKLIGHT_HPP
#define __INTERFACE_I_BACKLIGHT_HPP

#include <stdint.h>

namespace Interface
{

/**
 * @brief   Backlight Interface.
 */
class IBacklight
{
public:

    /**
     * @brief   Turn on backlight.
     * @param   timeout: Timeout for automatic turn off. 0 - no automatic turn off.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool on(uint32_t timeout = 0) = 0;

    /**
     * @brief   Turn off backlight.
     * @retval  Execution status. 'true' - success, 'false' - otherwise.
     */
    virtual bool off() = 0;

    /**
     * @brief   Check if the backlight is on.
     * @retval  'true' - backlight is on, 'false' - otherwise.
     */
    virtual bool isOn() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IBacklight() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_BACKLIGHT_HPP */
