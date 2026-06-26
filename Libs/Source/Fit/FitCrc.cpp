/**
 ******************************************************************************
 * @file    FitCrc.cpp
 * @brief   FIT CRC-16 implementation (standard nibble-table algorithm).
 ******************************************************************************
 */

#include "SDK/Fit/FitCrc.hpp"

namespace SDK::Fit {

namespace {
    constexpr uint16_t kCrcTable[16] = {
        0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
        0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400,
    };
}  // namespace

uint16_t fitCrcByte(uint16_t crc, uint8_t byte)
{
    // Lower nibble.
    uint16_t tmp = kCrcTable[crc & 0x0F];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ kCrcTable[byte & 0x0F];

    // Upper nibble.
    tmp = kCrcTable[crc & 0x0F];
    crc = (crc >> 4) & 0x0FFF;
    crc = crc ^ tmp ^ kCrcTable[(byte >> 4) & 0x0F];

    return crc;
}

uint16_t fitCrcUpdate(uint16_t crc, const void* data, size_t len)
{
    const auto* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; ++i) {
        crc = fitCrcByte(crc, bytes[i]);
    }
    return crc;
}

}  // namespace SDK::Fit
