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
#include <cstdlib>

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

    // strtol rather than sscanf("%d.%d.%d"): the kernel exports strtol, while
    // sscanf would link the toolchain's own scanf into the app.
    char* end = nullptr;
    const char* p = str;
    long major = std::strtol(p, &end, 10);
    bool ok = end != p && *end == '.';
    p = end + 1;
    long minor = ok ? std::strtol(p, &end, 10) : 0;
    ok = ok && end != p && *end == '.';
    p = end + 1;
    long patch = ok ? std::strtol(p, &end, 10) : 0;
    ok = ok && end != p;

    if (ok) {
        v.major = static_cast<uint8_t>(major);
        v.minor = static_cast<uint8_t>(minor);
        v.patch = static_cast<uint8_t>(patch);
    }

    return v;
}

} // namespace SDK
