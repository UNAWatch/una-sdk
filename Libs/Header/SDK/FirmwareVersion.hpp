/**
 ******************************************************************************
 * @file    FirmwareVersion.hpp
 * @date    24-07-2025
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Firmware version type.
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include <cstdint>
#include <cstdio>

namespace SDK
{

#pragma pack(push, 1)

/**
 * @brief   Firmware version
 */
union FirmwareVersion {
    struct {
        uint8_t patch;
        uint8_t minor;
        uint8_t major;
    };
    uint32_t u32;
} ;

#pragma pack(pop)

static inline FirmwareVersion ParseVersion(const char* str)
{
    FirmwareVersion v {};

    if (str == nullptr) {
        return v;
    }

    int major, minor, patch;

    if (sscanf(str, "%d.%d.%d", &major, &minor, &patch) == 3) {
        v.major = static_cast<uint8_t>(major);
        v.minor = static_cast<uint8_t>(minor);
        v.patch = static_cast<uint8_t>(patch);
    }

    return v;
}

} // namespace SDK
