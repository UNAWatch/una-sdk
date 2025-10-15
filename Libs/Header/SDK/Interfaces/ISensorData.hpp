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

#pragma once

#include <cstdint>

namespace SDK::Interface
{
    
class ISensorData
{
public:
    virtual uint32_t getTimestamp()                 const = 0;
    virtual uint16_t getLength()                    const = 0;
    virtual float    getAsFloat(uint16_t index = 0) const = 0;
    virtual uint32_t getAsU32(uint16_t index = 0)   const = 0;
    virtual int32_t  getAsI32(uint16_t index = 0)   const = 0;
};

} // namespace SDK::Interface
