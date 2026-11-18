/**
 * @file Png.hpp
 * @brief Minimal PNG writer for simulator screenshots.
 *
 * Simulator only. Writes 8-bit RGB with uncompressed (stored) deflate blocks:
 * no library needed, readable by every image viewer and by coding agents,
 * which often can't read BMP. A 240x240 screenshot is about 170 KB.
 */

#ifndef SDK_SIMULATOR_AUTOMATION_PNG_HPP
#define SDK_SIMULATOR_AUTOMATION_PNG_HPP

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace SDK::Simulator::Automation::Png
{

namespace detail
{

inline uint32_t crc32(const uint8_t* data, size_t size, uint32_t crc = 0)
{
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t n = 0; n < 256; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            t[n] = c;
        }
        return t;
    }();
    crc = ~crc;
    for (size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

inline void put32(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

inline void chunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& data)
{
    put32(out, static_cast<uint32_t>(data.size()));
    const size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    put32(out, crc32(out.data() + start, out.size() - start));
}

} // namespace detail

/**
 * @brief Write an image as PNG.
 * @param path   File to write.
 * @param width  Pixels per row.
 * @param height Rows.
 * @param rgb    width * height pixels, 3 bytes each, R G B, rows top to bottom.
 * @return False if the file can't be written.
 */
inline bool writeRgb(const std::string& path, int width, int height, const uint8_t* rgb)
{
    using namespace detail;
    std::vector<uint8_t> out = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

    std::vector<uint8_t> header;
    put32(header, static_cast<uint32_t>(width));
    put32(header, static_cast<uint32_t>(height));
    header.insert(header.end(), { 8, 2, 0, 0, 0 }); // 8 bit, RGB, deflate, no filter, no interlace
    chunk(out, "IHDR", header);

    // Raw scanlines, each led by filter type 0.
    const size_t rowBytes = static_cast<size_t>(width) * 3;
    std::vector<uint8_t> raw;
    raw.reserve((rowBytes + 1) * height);
    for (int y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb + y * rowBytes, rgb + (y + 1) * rowBytes);
    }

    // zlib stream of stored blocks (at most 65535 bytes each) and an Adler-32.
    std::vector<uint8_t> z = { 0x78, 0x01 };
    size_t pos = 0;
    do {
        const size_t n = (raw.size() - pos > 65535) ? 65535 : raw.size() - pos;
        const bool last = (pos + n == raw.size());
        z.push_back(last ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n));
        z.push_back(static_cast<uint8_t>(n >> 8));
        z.push_back(static_cast<uint8_t>(~n));
        z.push_back(static_cast<uint8_t>(~n >> 8));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    } while (pos < raw.size());
    uint32_t a = 1;
    uint32_t b = 0;
    for (uint8_t v : raw) {
        a = (a + v) % 65521;
        b = (b + a) % 65521;
    }
    put32(z, (b << 16) | a);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});

    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr) {
        return false;
    }
    const bool ok = std::fwrite(out.data(), 1, out.size(), f) == out.size();
    return (std::fclose(f) == 0) && ok;
}

} // namespace SDK::Simulator::Automation::Png

#endif // SDK_SIMULATOR_AUTOMATION_PNG_HPP
