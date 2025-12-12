/**
 ******************************************************************************
 * @file    SensorData.hpp
 * @date    08-December-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Sensor data
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SENSOR_DATA_HPP
#define __SENSOR_DATA_HPP

#include <cstdint>

#pragma pack(push, 4)

namespace SDK::Sensor
{
    struct Data
    {
        union Field {
            float    f;
            uint32_t u32;
            int32_t  i32;
        };

        uint32_t mTimeStamp;
        uint32_t mTimeStampUs;
        Field    mValue[1];
    };

    static_assert(sizeof(Data) == 12);

} /* namespace SDK::Sensor */

#pragma pack(pop)

#endif /* __SENSOR_DATA_HPP */
