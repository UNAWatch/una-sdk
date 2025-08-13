/**
 ******************************************************************************
 * @file    IPower.hpp
 * @date    05-02-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Power Interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_POWER_HPP
#define __INTERFACE_I_POWER_HPP

namespace Interface
{

/**
 * @brief   Power Interface.
 */
class IPower {

public:

    /**
     * @brief   Get actual battery level.
     * @retval  Actual battery level (0 - 100%).
     */
    virtual float getBatteryLevel() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~IPower() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_POWER_HPP */
