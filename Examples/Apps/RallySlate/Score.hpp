#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace Rally {

// A casual round rule, not certification for any official sport.
struct Score {
    static constexpr uint8_t targets[3] = {11, 15, 21};
    uint8_t targetIndex = 0;
    uint8_t count = 0;
    std::array<uint8_t, 60> points{};

    unsigned side(unsigned who) const {
        unsigned n = 0;
        for (unsigned i = 0; i < count; ++i) n += points[i] == who;
        return n;
    }
    int winner() const {
        const unsigned a = side(0), b = side(1), target = targets[targetIndex];
        if (a == 30 || (a >= target && a >= b + 2)) return 0;
        if (b == 30 || (b >= target && b >= a + 2)) return 1;
        return -1;
    }
    bool add(unsigned who) {
        if (who > 1 || winner() >= 0 || count >= 59) return false;
        points[count++] = static_cast<uint8_t>(who);
        return true;
    }
    bool undo() {
        if (!count) return false;
        points[--count] = 0;
        return true;
    }
    void reset(uint8_t target) {
        targetIndex = target < 3 ? target : 0;
        count = 0;
        points.fill(0);
    }
};

using Record = std::array<uint8_t, 76>;
inline uint32_t checksum(const Record& r) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < 72; ++i) h = (h ^ r[i]) * 16777619u;
    return h; // Accidental-damage detection, not authentication.
}
inline void put32(Record& r, unsigned at, uint32_t x) {
    for (unsigned i = 0; i < 4; ++i) r[at + i] = static_cast<uint8_t>(x >> (8 * i));
}
inline uint32_t get32(const Record& r, unsigned at) {
    uint32_t x = 0;
    for (unsigned i = 0; i < 4; ++i) x |= uint32_t(r[at + i]) << (8 * i);
    return x;
}
inline Record encode(const Score& s, uint32_t serial) {
    Record r{};
    r[0] = 'R'; r[1] = 'S'; r[2] = 'L'; r[3] = '1'; r[4] = 1;
    r[5] = s.targetIndex; r[6] = s.count;
    put32(r, 8, serial);
    for (unsigned i = 0; i < s.count; ++i) r[12 + i] = s.points[i];
    put32(r, 72, checksum(r));
    return r;
}
inline bool decode(const Record& r, Score& out, uint32_t& serial) {
    if (r[0] != 'R' || r[1] != 'S' || r[2] != 'L' || r[3] != '1' ||
        r[4] != 1 || r[5] > 2 || r[6] > 59 || r[7] != 0 ||
        checksum(r) != get32(r, 72)) return false;
    Score s; s.reset(r[5]);
    for (unsigned i = 0; i < r[6]; ++i)
        if (!s.add(r[12 + i])) return false;
    for (unsigned i = r[6]; i < 60; ++i) if (r[12 + i]) return false;
    out = s; serial = get32(r, 8);
    return true;
}
inline bool newer(uint32_t a, uint32_t b) {
    const uint32_t delta = a - b;
    return delta != 0 && delta < 0x80000000u;
}
} // namespace Rally
