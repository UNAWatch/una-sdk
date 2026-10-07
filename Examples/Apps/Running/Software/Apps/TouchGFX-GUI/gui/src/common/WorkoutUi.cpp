#include <gui/common/WorkoutUi.hpp>

#include <touchgfx/TypedText.hpp>
#include <touchgfx/Font.hpp>
#include <SDK/GUI/Color.hpp>
#include <SDK/Utils/Utils.hpp>
#include "SDK/Workout/TargetGauge.hpp"
#include "SDK/Workout/WorkoutProgram.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

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

constexpr touchgfx::Unicode::UnicodeChar kEllipsis = 0x2026;

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

// What @p cp is drawn as: up to two characters into @p out; returns how many.
int replacement(uint32_t cp, touchgfx::Unicode::UnicodeChar* out)
{
    if (cp >= 0x20 && cp < 0x7F) {
        out[0] = static_cast<touchgfx::Unicode::UnicodeChar>(cp);
        return 1;
    }
    if (cp >= 0xC0 && cp < 0x180) {
        const char* r = kLatin[cp - 0xC0];
        out[0] = static_cast<unsigned char>(r[0]);
        if (r[1] != '\0') {
            out[1] = static_cast<unsigned char>(r[1]);
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
        out[0] = kEllipsis;
        return 1;
    default:
        out[0] = '?';
        return 1;
    }
}

int16_t halfChord(int16_t y)
{
    constexpr int16_t kRadius = 120;
    const int16_t dy = y < kRadius ? static_cast<int16_t>(kRadius - y) : static_cast<int16_t>(y - kRadius);
    if (dy >= kRadius) {
        return 0;
    }
    return static_cast<int16_t>(std::sqrt(static_cast<float>(kRadius * kRadius - dy * dy)));
}

} // namespace

void toText(const char* utf8, touchgfx::Unicode::UnicodeChar* out, uint16_t cap)
{
    if (cap == 0) {
        return;
    }
    uint16_t n = 0;
    const unsigned char* s = reinterpret_cast<const unsigned char*>(utf8 ? utf8 : "");
    while (*s != '\0') {
        touchgfx::Unicode::UnicodeChar r[2];
        const int k = replacement(nextCodePoint(s), r);
        if (n + k >= cap) {
            break;
        }
        for (int i = 0; i < k; ++i) {
            out[n++] = r[i];
        }
    }
    out[n] = 0;
}

void fitWidth(touchgfx::Unicode::UnicodeChar* buf, touchgfx::TypedTextId typography, int16_t maxWidth)
{
    const touchgfx::Font* font = touchgfx::TypedText(typography).getFont();
    if (font == nullptr || maxWidth <= 0) {
        return;
    }
    uint16_t len = touchgfx::Unicode::strlen(buf);
    if (font->getStringWidth(buf) <= maxWidth) {
        return;
    }
    const touchgfx::Unicode::UnicodeChar ellipsis[] = { kEllipsis, 0 };
    const int16_t room = static_cast<int16_t>(maxWidth - font->getStringWidth(ellipsis));
    while (len > 0) {
        buf[--len] = 0;
        if (font->getStringWidth(buf) <= room) {
            break;
        }
    }
    // Not after a space: "Long ..." reads worse than "Long...".
    while (len > 0 && buf[len - 1] == ' ') {
        buf[--len] = 0;
    }
    buf[len]     = kEllipsis;
    buf[len + 1] = 0;
}

void wrapTwo(const touchgfx::Unicode::UnicodeChar* text, touchgfx::Unicode::UnicodeChar* line1,
             touchgfx::Unicode::UnicodeChar* line2, uint16_t cap, touchgfx::TypedTextId typography,
             int16_t width1, int16_t width2)
{
    line1[0] = 0;
    line2[0] = 0;
    const touchgfx::Font* font = touchgfx::TypedText(typography).getFont();
    if (font == nullptr || cap == 0) {
        return;
    }
    while (*text == ' ') {
        ++text;
    }
    // The longest run of whole words that fits the first line.
    uint16_t len = touchgfx::Unicode::strlen(text);
    uint16_t cut = 0;
    for (uint16_t i = 0; i <= len && i < cap; ++i) {
        if (i == len || text[i] == ' ') {
            touchgfx::Unicode::strncpy(line1, text, i);
            line1[i] = 0;
            if (font->getStringWidth(line1) > width1) {
                break;
            }
            cut = i;
        }
    }
    if (cut == 0) {
        // One word too long for the line: cut it there.
        touchgfx::Unicode::strncpy(line1, text, static_cast<uint16_t>(cap - 1));
        line1[cap - 1] = 0;
        fitWidth(line1, typography, width1);
        return;
    }
    touchgfx::Unicode::strncpy(line1, text, cut);
    line1[cut] = 0;
    const touchgfx::Unicode::UnicodeChar* rest = text + cut;
    while (*rest == ' ') {
        ++rest;
    }
    touchgfx::Unicode::strncpy(line2, rest, static_cast<uint16_t>(cap - 1));
    line2[cap - 1] = 0;
    fitWidth(line2, typography, width2);
}

int16_t lineWidth(int16_t top, int16_t bottom, int16_t margin)
{
    const int16_t half = halfChord(top) < halfChord(bottom) ? halfChord(top) : halfChord(bottom);
    const int16_t w    = static_cast<int16_t>(2 * (half - margin));
    return w > 0 ? w : 0;
}

touchgfx::TypedTextId kindTitle(uint8_t intensity)
{
    switch (static_cast<SDK::Workout::Intensity>(intensity)) {
    case SDK::Workout::Intensity::Rest:     return T_TEXT_REST_UC;
    case SDK::Workout::Intensity::Warmup:   return T_TEXT_WARM_UP_UC;
    case SDK::Workout::Intensity::Cooldown: return T_TEXT_COOL_DOWN_UC;
    default:                                return T_TEXT_RUN_UC;
    }
}

touchgfx::TypedTextId kindLabel(uint8_t intensity)
{
    switch (static_cast<SDK::Workout::Intensity>(intensity)) {
    case SDK::Workout::Intensity::Rest:     return T_TEXT_REST;
    case SDK::Workout::Intensity::Warmup:   return T_TEXT_WARM_UP;
    case SDK::Workout::Intensity::Cooldown: return T_TEXT_COOL_DOWN;
    default:                                return T_TEXT_RUN;
    }
}

touchgfx::colortype kindColor(uint8_t intensity)
{
    return static_cast<SDK::Workout::Intensity>(intensity) == SDK::Workout::Intensity::Active
        ? touchgfx::colortype(SDK::GUI::Color::CYAN)
        : touchgfx::colortype(SDK::GUI::Color::WHITE);
}

touchgfx::colortype zoneColor(uint8_t zone)
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
        std::snprintf(dist, sizeof(dist), v < 10.0f ? "%.1f %s" : "%.0f %s",
                      static_cast<double>(v), imperial ? "mi" : "km");
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
