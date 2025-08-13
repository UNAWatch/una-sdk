/**
 ******************************************************************************
 * @file    ISensorData.hpp
 * @date    01-August-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   ISensorData interface for user applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_SENSOR_DATA_HPP
#define __I_SENSOR_DATA_HPP

#include <cstdint>

namespace Interface
{
    class ISensorData
    {
    public:
        virtual uint32_t getTimestamp()               const = 0;
        virtual uint16_t getLength()                  const = 0;
        virtual float    getValue(uint16_t index = 0) const = 0;
    }; /* class ISensorData */

} /* namespace Interface */

#endif /* __I_SENSOR_DATA_HPP */
