#ifndef FILE_CRC32_HPP
#define FILE_CRC32_HPP

#include <cstddef>
#include <cstdint>

/// CRC-32/ISO-HDLC, the one FTS `DIGEST` (0x70) reports and zlib's `crc32` computes, so a
/// phone can predict the value this app records for a file it wrote.
constexpr uint32_t fileCrc32(const char* data, size_t len, uint32_t crc = 0)
{
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint8_t>(data[i]);
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

#endif // FILE_CRC32_HPP
