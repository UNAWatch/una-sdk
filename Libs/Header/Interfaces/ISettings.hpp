/**
 ******************************************************************************
 * @file    ISettings.hpp
 * @date    14-01-2024
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   SETTINGS Interface.
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __INTERFACE_I_SETTINGS_HPP
#define __INTERFACE_I_SETTINGS_HPP

namespace Interface
{

/**
 * @brief   SETTINGS Interface.
 */
class ISettings
{

public:


    virtual bool isUnitsImperial() = 0;

protected:

    /**
     * @brief   Destructor.
     */
    virtual ~ISettings() = default;

};

} /* namespace Interface */

#endif /* __INTERFACE_I_SETTINGS_HPP */
