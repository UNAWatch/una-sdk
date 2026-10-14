/**
 ******************************************************************************
 * @file    WorkoutUi.cpp
 * @brief   Text and colours shared by the workout screens.
 ******************************************************************************
 */

#include "gui/WorkoutUi.hpp"
#include "gui/Format.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "SDK/GUI/Color.hpp"
#include "SDK/Utils/Utils.hpp"
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

namespace WorkoutUi
{

namespace
{

// U+00C0 to U+017F without their accents: one or two letters each.
const char kLatin[][3] = {
    "A\0", "A\0", "A\0", "A\0", "A\0", "A\0", "AE", "C\0",  // U+00C0
    "E\0", "E\0", "E\0", "E\0", "I\0", "I\0", "I\0", "I\0",  // U+00C8
    "D\0", "N\0", "O\0", "O\0", "O\0", "O\0", "O\0", "x\0",  // U+00D0
    "O\0", "U\0", "U\0", "U\0", "U\0", "Y\0", "Th", "ss",    // U+00D8
    "a\0", "a\0", "a\0", "a\0", "a\0", "a\0", "ae", "c\0",  // U+00E0
    "e\0", "e\0", "e\0", "e\0", "i\0", "i\0", "i\0", "i\0",  // U+00E8
    "d\0", "n\0", "o\0", "o\0", "o\0", "o\0", "o\0", "/\0",  // U+00F0
    "o\0", "u\0", "u\0", "u\0", "u\0", "y\0", "th", "y\0",  // U+00F8
    "A\0", "a\0", "A\0", "a\0", "A\0", "a\0", "C\0", "c\0",  // U+0100
    "C\0", "c\0", "C\0", "c\0", "C\0", "c\0", "D\0", "d\0",  // U+0108
    "D\0", "d\0", "E\0", "e\0", "E\0", "e\0", "E\0", "e\0",  // U+0110
    "E\0", "e\0", "E\0", "e\0", "G\0", "g\0", "G\0", "g\0",  // U+0118
    "G\0", "g\0", "G\0", "g\0", "H\0", "h\0", "H\0", "h\0",  // U+0120
    "I\0", "i\0", "I\0", "i\0", "I\0", "i\0", "I\0", "i\0",  // U+0128
    "I\0", "i\0", "IJ", "ij", "J\0", "j\0", "K\0", "k\0",    // U+0130
    "k\0", "L\0", "l\0", "L\0", "l\0", "L\0", "l\0", "L\0",  // U+0138
    "l\0", "L\0", "l\0", "N\0", "n\0", "N\0", "n\0", "N\0",  // U+0140
    "n\0", "n\0", "N\0", "n\0", "O\0", "o\0", "O\0", "o\0",  // U+0148
    "O\0", "o\0", "OE", "oe", "R\0", "r\0", "R\0", "r\0",    // U+0150
    "R\0", "r\0", "S\0", "s\0", "S\0", "s\0", "S\0", "s\0",  // U+0158
    "S\0", "s\0", "T\0", "t\0", "T\0", "t\0", "T\0", "t\0",  // U+0160
    "U\0", "u\0", "U\0", "u\0", "U\0", "u\0", "U\0", "u\0",  // U+0168
    "U\0", "u\0", "U\0", "u\0", "W\0", "w\0", "Y\0", "y\0",  // U+0170
    "Y\0", "Z\0", "z\0", "Z\0", "z\0", "Z\0", "z\0", "s\0",  // U+0178
};

/// U+2026, which the fonts carry for text cut to fit.
constexpr const char kEllipsis[] = "\xE2\x80\xA6";
constexpr size_t     kEllipsisLen = sizeof(kEllipsis) - 1;

// The next code point of @p s, moving past it; '?' for a malformed sequence.
uint32_t nextCodePoint(const unsigned char*& s)
{
    const unsigned char c = *s++;
    if (c < 0x80) {
        return c;
    }
    int more = 0;
    uint32_t cp = 0;
    if ((c & 0xE0) == 0xC0) {
        more = 1;
        cp = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        more = 2;
        cp = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        more = 3;
        cp = c & 0x07;
    } else {
        return '?';
    }
    for (int i = 0; i < more; ++i) {
        if ((*s & 0xC0) != 0x80) {
            return '?';
        }
        cp = (cp << 6) | (*s++ & 0x3F);
    }
    return cp;
}

// What @p cp is drawn as, as UTF-8 into @p out (up to three bytes); returns
// how many bytes.
size_t replacement(uint32_t cp, char* out)
{
    if (cp >= 0x20 && cp < 0x7F) {
        out[0] = static_cast<char>(cp);
        return 1;
    }
    if (cp >= 0xC0 && cp < 0x180) {
        const char* r = kLatin[cp - 0xC0];
        out[0] = r[0];
        if (r[1] != '\0') {
            out[1] = r[1];
            return 2;
        }
        return 1;
    }
    switch (cp) {
    case 0x09: case 0x0A: case 0x0D: case 0xA0:
        out[0] = ' ';
        return 1;
    case 0x2018: case 0x2019: case 0x201A: case 0x201B:
        out[0] = '\'';
        return 1;
    case 0x201C: case 0x201D: case 0x201E:
        out[0] = '"';
        return 1;
    case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014: case 0x2212:
        out[0] = '-';
        return 1;
    case 0xB7: case 0x2022:
        out[0] = '.';
        return 1;
    case 0x2026:
        std::memcpy(out, kEllipsis, kEllipsisLen);
        return kEllipsisLen;
    default:
        out[0] = '?';
        return 1;
    }
}

/// Back @p len off any incomplete UTF-8 sequence at the end of @p s.
size_t wholeChars(const char* s, size_t len)
{
    size_t start = len;
    while (start > 0 && (static_cast<unsigned char>(s[start - 1]) & 0xC0) == 0x80) {
        --start;
    }
    if (start == 0) {
        return len;  // continuation bytes only: nothing to keep whole
    }
    const unsigned char lead = static_cast<unsigned char>(s[start - 1]);
    if (lead < 0x80) {
        return len;
    }
    const size_t need = (lead & 0xE0) == 0xC0 ? 2 : (lead & 0xF0) == 0xE0 ? 3 : 4;
    return len - (start - 1) >= need ? len : start - 1;
}

/// Copy at most @p n bytes of @p src into @p dst (of @p cap bytes), ending
/// on a whole character.
void copyText(char* dst, size_t cap, const char* src, size_t n)
{
    if (cap == 0) {
        return;
    }
    if (n > cap - 1) {
        n = cap - 1;
    }
    std::memcpy(dst, src, n);
    dst[wholeChars(dst, n)] = '\0';
}

int32_t halfChord(int32_t y)
{
    constexpr int32_t kRadius = 120;
    const int32_t dy = y < kRadius ? kRadius - y : y - kRadius;
    if (dy >= kRadius) {
        return 0;
    }
    return static_cast<int32_t>(std::sqrt(static_cast<float>(kRadius * kRadius - dy * dy)));
}

} // namespace

void toText(const char* utf8, char* out, size_t cap)
{
    if (cap == 0) {
        return;
    }
    size_t n = 0;
    const unsigned char* s = reinterpret_cast<const unsigned char*>(utf8 ? utf8 : "");
    while (*s != '\0') {
        char r[4];
        const size_t k = replacement(nextCodePoint(s), r);
        if (n + k >= cap) {
            break;
        }
        std::memcpy(out + n, r, k);
        n += k;
    }
    out[n] = '\0';
}

int32_t textWidth(const char* text, const lv_font_t* font)
{
    lv_point_t size;
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

void fitWidth(char* buf, size_t cap, const lv_font_t* font, int32_t maxWidth)
{
    if (font == nullptr || maxWidth <= 0 || cap == 0) {
        return;
    }
    if (textWidth(buf, font) <= maxWidth) {
        return;
    }
    const int32_t room = maxWidth - textWidth(kEllipsis, font);
    size_t len = std::strlen(buf);
    // Leave room in the buffer for the ellipsis, then cut whole characters
    // until the rest fits beside it.
    if (len + kEllipsisLen + 1 > cap) {
        len = cap > kEllipsisLen + 1 ? cap - kEllipsisLen - 1 : 0;
        len = wholeChars(buf, len);
        buf[len] = '\0';
    }
    while (len > 0 && textWidth(buf, font) > room) {
        do {
            --len;
        } while (len > 0 && (static_cast<unsigned char>(buf[len]) & 0xC0) == 0x80);
        buf[len] = '\0';
    }
    // Not after a space: "Long ..." reads worse than "Long...".
    while (len > 0 && buf[len - 1] == ' ') {
        buf[--len] = '\0';
    }
    if (len + kEllipsisLen + 1 <= cap) {
        std::memcpy(buf + len, kEllipsis, kEllipsisLen + 1);
    }
}

size_t wrapTwo(const char* text, char* line1, char* line2, size_t cap, const lv_font_t* font,
               int32_t width1, int32_t width2)
{
    if (cap == 0) {
        return 0;
    }
    line1[0] = '\0';
    line2[0] = '\0';
    if (font == nullptr) {
        return 0;
    }
    const char* start = text;
    while (*text == ' ') {
        ++text;
    }
    // The longest run of whole words that fits the first line.
    const size_t len = std::strlen(text);
    size_t cut = 0;
    for (size_t i = 0; i <= len && i < cap; ++i) {
        if (i == len || text[i] == ' ') {
            copyText(line1, cap, text, i);
            if (textWidth(line1, font) > width1) {
                break;
            }
            cut = i;
        }
    }
    if (cut == 0) {
        // One word too long for the line: cut it there, and the rest of the
        // word is dropped with it.
        copyText(line1, cap, text, len);
        fitWidth(line1, cap, font, width1);
        size_t used = 0;
        while (text[used] != '\0' && text[used] != ' ') {
            ++used;
        }
        return static_cast<size_t>(text - start) + used;
    }
    copyText(line1, cap, text, cut);
    const char* rest = text + cut;
    while (*rest == ' ') {
        ++rest;
    }
    copyText(line2, cap, rest, std::strlen(rest));
    fitWidth(line2, cap, font, width2);
    return static_cast<size_t>(rest - start);
}

int32_t lineWidth(int32_t top, int32_t bottom, int32_t margin)
{
    const int32_t half = halfChord(top) < halfChord(bottom) ? halfChord(top) : halfChord(bottom);
    const int32_t w    = 2 * (half - margin);
    return w > 0 ? w : 0;
}

const char* kindTitle(uint8_t intensity)
{
    switch (static_cast<SDK::Workout::Intensity>(intensity)) {
    case SDK::Workout::Intensity::Rest:     return "REST";
    case SDK::Workout::Intensity::Warmup:   return "WARM UP";
    case SDK::Workout::Intensity::Cooldown: return "COOL DOWN";
    default:                                return "RUN";
    }
}

const char* kindLabel(uint8_t intensity)
{
    switch (static_cast<SDK::Workout::Intensity>(intensity)) {
    case SDK::Workout::Intensity::Rest:     return "Rest";
    case SDK::Workout::Intensity::Warmup:   return "Warm Up";
    case SDK::Workout::Intensity::Cooldown: return "Cool Down";
    default:                                return "Run";
    }
}

uint32_t kindColor(uint8_t intensity)
{
    return static_cast<SDK::Workout::Intensity>(intensity) == SDK::Workout::Intensity::Active
        ? SDK::GUI::Color::CYAN
        : SDK::GUI::Color::WHITE;
}

uint32_t zoneColor(uint8_t zone)
{
    switch (static_cast<SDK::Workout::Zone>(zone)) {
    case SDK::Workout::Zone::In:    return SDK::GUI::Color::CHARTREUSE;
    case SDK::Workout::Zone::Above: return SDK::GUI::Color::RED;
    case SDK::Workout::Zone::Below: return SDK::GUI::Color::YELLOW_DARK;
    default:                        return SDK::GUI::Color::GRAY;
    }
}

void formatDate(uint32_t ymd, char* buf, size_t cap)
{
    static const char* const kDays[]   = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    static const char* const kMonths[] = { "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                           "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
    int y = static_cast<int>(ymd / 10000u);
    const int m = static_cast<int>(ymd / 100u % 100u);
    const int d = static_cast<int>(ymd % 100u);
    if (m < 1 || m > 12 || d < 1 || d > 31) {
        std::snprintf(buf, cap, "%s", "");
        return;
    }
    // Sakamoto's day of the week.
    static const int kOffsets[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3) {
        --y;
    }
    const int wd = (y + y / 4 - y / 100 + y / 400 + kOffsets[m - 1] + d) % 7;
    std::snprintf(buf, cap, "%s %d %s", kDays[wd], d, kMonths[m - 1]);
}

void formatTotals(const SDK::Workout::Library::Entry& entry, bool imperial, char* buf, size_t cap)
{
    char dist[16] = "";
    char time[16] = "";
    if (entry.distanceCm > 0) {
        float v = static_cast<float>(entry.distanceCm) / 100000.0f;  // km
        if (imperial) {
            v = SDK::Utils::kmToMiles(v);
        }
        char number[12];
        Fmt::fixed(number, sizeof(number), v, v < 10.0f ? 1 : 0);
        std::snprintf(dist, sizeof(dist), "%s %s", number, imperial ? "mi" : "km");
    }
    if (entry.timeMs > 0) {
        const uint32_t min = (entry.timeMs + 30000u) / 60000u;
        if (min >= 60) {
            std::snprintf(time, sizeof(time), "%lu h %02lu", static_cast<unsigned long>(min / 60),
                          static_cast<unsigned long>(min % 60));
        } else {
            std::snprintf(time, sizeof(time), "%lu min", static_cast<unsigned long>(min));
        }
    }
    const char* open = entry.openSteps > 0 ? "~" : "";
    if (dist[0] != '\0' && time[0] != '\0') {
        std::snprintf(buf, cap, "%s%s, %s", open, dist, time);
    } else if (dist[0] != '\0' || time[0] != '\0') {
        std::snprintf(buf, cap, "%s%s", open, dist[0] != '\0' ? dist : time);
    } else {
        std::snprintf(buf, cap, "%s", "");
    }
}

void splitDuration(const char* duration, char* number, size_t numberCap, char* unit, size_t unitCap)
{
    const char* space = std::strrchr(duration, ' ');
    if (space == nullptr) {
        std::snprintf(number, numberCap, "%s", duration);
        std::snprintf(unit, unitCap, "%s", "");
        return;
    }
    std::snprintf(number, numberCap, "%.*s", static_cast<int>(space - duration), duration);
    std::snprintf(unit, unitCap, "%s", space + 1);
}

} // namespace WorkoutUi
