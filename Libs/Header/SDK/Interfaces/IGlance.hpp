/**
 ******************************************************************************
 * @file    IGlance.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   API for use in Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_GLANCE_HPP
#define __I_GLANCE_HPP

#include <cstdint>
#include <cstdbool>
#include <variant>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <type_traits>

namespace SDK::Interface {

    class IGlance {
    public:
        virtual ~IGlance() = default;

        enum Color : uint8_t {
            WHITE        = 0x3F, // C0C0C0 -> 11 11 11
            GRAY         = 0x2A, // 808080 -> 10 10 10
            GRAY_DARK    = 0x15, // 404040 -> 01 01 01
            BLACK        = 0x00, // 000000 -> 00 00 00
            YELLOW_DARK  = 0x38, // C08000 -> 11 10 00
            TEAL         = 0x0A, // 008080 -> 00 10 10
            TEAL_DARK    = 0x05, // 004040 -> 00 01 01
            GREEN        = 0x0C, // 00C000 -> 00 11 00
            DARK_GREEN   = 0x04, // 004000 -> 00 01 00
            BROWN        = 0x39, // C08040 -> 11 10 01
            RED          = 0x30, // C00000 -> 11 00 00
            DARK_RED     = 0x10, // 400000 -> 01 00 00
            BLUE         = 0x03, // 0000C0 -> 00 00 11
            DARK_BLUE    = 0x01, // 000040 -> 00 00 01
            CYAN         = 0x1F, // 40C0C0 -> 01 11 11
            CYAN_LIGHT   = 0x2F, // 80C0C0 -> 10 11 11
        };

        enum class Font {
            POPPINS_REGULAR_18 = 0,
            POPPINS_MEDIUM_10,
            POPPINS_MEDIUM_18,
            POPPINS_MEDIUM_25,
            POPPINS_MEDIUM_50,
            POPPINS_MEDIUM_60,
            POPPINS_SEMIBOLD_18,
            POPPINS_SEMIBOLD_20,
            POPPINS_SEMIBOLD_25,
            POPPINS_SEMIBOLD_30,
            POPPINS_SEMIBOLD_35,
            POPPINS_SEMIBOLD_40,
            POPPINS_SEMIBOLD_60,
            POPPINS_ITALIC_18,
            POPPINS_ITALIC_20,
            POPPINS_LIGHTITALIC_18,
            POPPINS_LIGHT_60,
        };

        class Point {
        public:
            Point()
                : x()
                , y()
            {}

            Point(uint16_t x, uint16_t y)
                : x(x)
                , y(y)
            {}

            uint16_t x;
            uint16_t y;
        };

        class Size {
        public:
            Size()
                : w()
                , h()
            {}

            Size(uint16_t w, uint16_t h)
                : w(w)
                , h(h)
            {}

            uint16_t w;
            uint16_t h;
        };

        //// Payloads
        struct Text {
            static constexpr std::size_t kMax = 64;  ///< Buffer size incl. '\0'

            Point   pos{};                                  ///< Text position
            char    str[kMax]{};                            ///< Storage for text (NUL-terminated)
            Font    fontID = Font::POPPINS_REGULAR_18;      ///< Default font
            uint8_t color  = static_cast<uint8_t>(Color::WHITE);  ///< Default color

            /// @brief Replace text with a C-string, clamped to kMax-1 chars.
            void set(const char* s)
            {
                if (!s) {
                    str[0] = '\0';
                    return;
                }

                // snprintf ensures null-termination
                std::snprintf(str, kMax, "%s", s);
            }

            /// @brief Append C-string, clamped to buffer.
            void append(const char* s)
            {
                if (!s || !*s) {
                    return;
                }

                const std::size_t cur = std::strlen(str);

                if (cur + 1 >= kMax) {
                    return;
                }

                const std::size_t cap = kMax - cur - 1;
                std::strncat(str, s, cap);
                str[kMax - 1] = '\0';
            }

            /// @brief printf-like into the same buffer.
            void print(const char* fmt, ...)
            {
                if (!fmt) {
                    str[0] = '\0';
                    return;
                }

                va_list args;
                va_start(args, fmt);
            #if defined(_MSC_VER)
                _vsnprintf_s(str, kMax, _TRUNCATE, fmt, args);
            #else
                std::vsnprintf(str, kMax, fmt, args);
                str[kMax - 1] = '\0';
            #endif
                va_end(args);
            }
        };

        struct Image {
            Point       pos{};
            Size        size{};
            const char* buff = nullptr; // Points to persistent image data
        };

        struct Line {
            Point   start{};
            Point   stop{};
            uint8_t color = Color::WHITE;
        };

        struct Rectangle {
            Point   pos{};
            Size    size{};
            uint8_t color           = Color::WHITE; // Color Line
            uint8_t colorBackground = Color::BLACK;
            bool    transparent     = false;
        };

        using Data = std::variant<std::monostate, Text, Image, Line, Rectangle>;

        class Tile {
        public:
            Tile() : data(Text{ {}, "", Font::POPPINS_REGULAR_18, Color::WHITE })
            {}

            Data data;

            // --- Full "emplace" setters (init all important fields at once) ---
            Tile& setText(Point pos, const char* s, Font fontID, uint8_t color)
            {
                Text t;

                t.pos    = pos;
                t.fontID = fontID;
                t.color  = color;
                t.set(s);

                data = t;

                return *this;
            }

            Tile& setImage(Point p, Size sz, const char* b)
            {
                data = Image{ p, sz, b };

                return *this;
            }

            Tile& setLine(Point a, Point b, uint8_t color)
            {
                data = Line{ a, b, color };

                return *this;
            }

            Tile& setRectangle(Point pos, Size size, uint8_t lineColor, uint8_t bgColor, bool transparent=false)
            {
                data = Rectangle{ pos, size, lineColor, bgColor, transparent };

                return *this;
            }

            // --- Fluent helpers to tweak geometry after type is chosen ---
            /// @brief Set position depending on active type (ext/Image/Rectangle: pos).
            Tile& at(Point pos)
            {
                std::visit([&](auto& v) {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, Text> || std::is_same_v<T, Image> || std::is_same_v<T, Rectangle>) {
                        v.pos = pos;
                    }
                  }, data);

                return *this;
            }

            /// @brief Resize for types that have Size.
            Tile& size(Size size)
            {
                std::visit([&](auto& v){
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, Image> || std::is_same_v<T, Rectangle>) {
                        v.size = size;
                    }
                }, data);

                return *this;
            }

            /// @brief Update color
            Tile& color(uint8_t color)
            {
                std::visit([&](auto& v){
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, Text> || std::is_same_v<T, Line> || std::is_same_v<T, Rectangle>) {
                        v.color = color;
                    }
                }, data);

                return *this;
            }

            /// @brief Update color
            Tile& bgColor(uint8_t color)
            {
                std::visit([&](auto& v){
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, Rectangle>) {
                        v.colorBackground = color;
                    }
                }, data);

                return *this;
            }

            /// @brief Toggle rectangle transparency.
            Tile& setTransparent(bool t)
            {
                if (auto* v = std::get_if<Rectangle>(&data)) {
                    v->transparent = t;
                }

                return *this;
            }

            // Minimal setText that only updates text on existing Text payload,
            // or creates a default Text once and preserves style/pos next time.
            Tile& setText(const char* s)
            {
                Text& t = ensureText();

                t.set(s);

                return *this;
            }

            Tile& appendText(const char* s)
            {
                Text& t = ensureText();

                t.append(s);

                return *this;
            }

            Tile& printText(const char* fmt, ...)
            {
                Text& t = ensureText();

                va_list args;
                va_start(args, fmt);
            #if defined(_MSC_VER)
                _vsnprintf_s(t.str, Text::kMax, _TRUNCATE, fmt, args);
            #else
                std::vsnprintf(t.str, Text::kMax, fmt, args);
                t.str[Text::kMax - 1] = '\0';
            #endif
                va_end(args);

                return *this;
            }

        private:
            /// Ensure variant holds Text; if not, switch once to default Text.
            Text& ensureText()
            {
                if (auto* t = std::get_if<Text>(&data)) {
                    return *t;
                }

                data = Text{};

                return std::get<Text>(data);
            }
        };

//        std::vector<TileData> tiles;
//        tiles.reserve(8); // не обов'язково, але корисно
//
//        tiles.emplace_back().setText("Hello", Font::Small, /*color*/ 0x2A);
//        tiles.emplace_back().setLine({0,10}, {50,10}, /*color*/ 0x10);
//        tiles.emplace_back().setRectangle({{5,5}}, {{40,20}}, /*line*/0x30, /*bg*/0x00);
//        tiles.emplace_back().setImage(myIconPtr);
//
//        render(tiles); // передав кудись
    };

}

#endif
