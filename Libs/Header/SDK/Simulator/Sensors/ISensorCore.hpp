/**
 ******************************************************************************
 * @file    ISensorCore.hpp
 * @date    29-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   SensorCore interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_SENSOR_CORE_HPP
#define __I_SENSOR_CORE_HPP

namespace Interface {

    class ISensorCore
    {
    public:
        virtual ~ISensorCore() = default;

        virtual void tick() = 0;
    };

}

#endif
