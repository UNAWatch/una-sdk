/**
 ******************************************************************************
 * @file    FitCrc.hpp
 * @brief   FIT CRC-16 used to protect the FIT file header and trailer.
 *
 * The FIT format protects each file with a 16-bit CRC stored little-endian as
 * the final two bytes of the file, computed over every preceding byte. The
 * algorithm is the standard FIT nibble-table CRC defined by the public FIT
 * Protocol specification.
 ******************************************************************************
 */

#ifndef __SDK_FIT_CRC_HPP
#define __SDK_FIT_CRC_HPP

#include <cstddef>
#include <cstdint>

namespace SDK::Fit {

/// Fold one byte into a running FIT CRC-16. Seed with 0 for a fresh file.
uint16_t fitCrcByte(uint16_t crc, uint8_t byte);

/// Fold a buffer into a running FIT CRC-16.
uint16_t fitCrcUpdate(uint16_t crc, const void* data, size_t len);

}  // namespace SDK::Fit

#endif  // __SDK_FIT_CRC_HPP
