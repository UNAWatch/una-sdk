/**
 ******************************************************************************
 * @file    GlanceControlText.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Text control for Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_TEXT_HPP
#define __GLANCE_CONTROL_TEXT_HPP

#include "SDK/Glance/GlanceControlBase.hpp"

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>

/**
 * @namespace SDK::Glance
 * @brief SDK components for Glance UI rendering.
 */
namespace SDK::Glance {

    /**
     * @brief Typed view over a text control record.
     *
     * Lightweight wrapper bound to a specific element inside a contiguous
     * storage of @c GlanceControl_t records. Mutating operations write
     * directly into the underlying record and call @c invalidate() to mark
     * the control as needing redraw.
     *
     * @note This class does not own the underlying storage or text buffer.
     *       The internal text is a fixed-size array of @c GLANCE_TEXT_SIZE
     *       bytes stored in the record's payload.
     */
    class ControlText : public Control {
    public:
        ControlText()
            : Control(nullptr, 0)
        {}

        /**
         * @brief Binds this view to an existing control record.
         *
         * @param controls Reference to the owning vector of control records.
         * @param idx      Index of the text record inside @p controls.
         */
        ControlText(std::vector<GlanceControl_t>* controls, std::size_t idx)
            : Control(controls, idx)
        {}

        /// @brief Virtual dtor from base; nothing to do.
        ~ControlText() override = default;

        /**
         * @brief Initializes text in a single call.
         *
         * Copies @p s into the internal fixed-size buffer if it fits,
         * sets position, font and color, then invalidates the control.
         *
         * @param pos    Top-left position.
         * @param size   Size (width/height).
         * @param s      C-string to copy (if @c nullptr, treated as empty).
         * @param fontID Font identifier.
         * @param color  Text color (RGB222 or palette index).
         * @param align  Text horizontal alignment. Default LEFT.
         * @return Reference to @c *this for fluent chaining.
         *
         * @note If @p s length is @c >= GLANCE_TEXT_SIZE, the call is ignored.
         */
        ControlText& init(GlancePoint_t pos, GlanceSize_t  size, const char* s, GlanceFont_t fontID, uint8_t color, GlanceAlignH_t align = GLANCE_ALIGN_H_LEFT)
        {
            const char* str = (s == nullptr) ? "" : s;

            if ((strlen(str) >= GLANCE_TEXT_SIZE)) {
                return *this;
            }
            
            GlanceText_t& text = getText();

            text.pos    = pos;
            text.size   = size;
            text.font   = fontID;
            text.color  = color;
            text.align  = align;
            strcpy(text.str, str);

            invalidate();

            return *this;
        }

        /**
         * @brief Replaces the text content.
         *
         * Copies @p s into the internal buffer if it fits and invalidates
         * the control. If @p s is @c nullptr, an empty string is used.
         *
         * @param s New C-string (may be @c nullptr).
         * @return Reference to @c *this for fluent chaining.
         *
         * @note If the length of @p s is @c >= GLANCE_TEXT_SIZE, the call is ignored.
         */
        ControlText& setText(const char* s = nullptr)
        {
            const char* str = (s == nullptr) ? "" : s;

            if ((strlen(str) >= GLANCE_TEXT_SIZE)) {
                return *this;
            }
            
            strcpy(getText().str, str);

            invalidate();

            return *this;
        }

        /**
         * @brief Sets the top-left position of the text and size.
         * @param pos New position.
         * @param sz Size (width/height).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlText& pos(GlancePoint_t pos, GlanceSize_t sz)
        {
            getText().pos = pos;
            getText().size = sz;
            invalidate();
            return *this;
        }

        /**
         * @brief Sets the top-left position of the text.
         *
         * @param pos New position.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlText& pos(GlancePoint_t pos)
        {
            getText().pos = pos;
            invalidate();
            return *this;
        }

        /**
         * @brief Gets the current top-left position of the text.
         * @return Current position.
         */
        GlancePoint_t pos()
        {
            return getText().pos;
        }

        /**
         * @brief Set size
         * @param sz      Size (width/height).
         * @return *this for fluent chaining.
         */
        ControlText& size(GlanceSize_t sz)
        {
            getText().size = sz;
            invalidate();
            return *this;
        }

        /**
         * @brief Returns size
         * @return Current size (w/h).
         */
        GlanceSize_t size()
        {
            return getText().size;
        }

        /**
         * @brief Sets the font of the text.
         *
         * @param id New font identifier.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlText& font(GlanceFont_t id)
        {
            getText().font = id;

            invalidate();

            return *this;
        }

        /**
         * @brief Sets the text color.
         *
         * @param color New color (RGB222 or palette index).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlText& color(uint8_t color)
        {
            getText().color = color;

            invalidate();

            return *this;
        }

        /**
         * @brief Set alignment
         * @param align New horizontal alignment.
         * @return *this for fluent chaining.
         */
        ControlText& alignment(GlanceAlignH_t align)
        {
            getText().align = align;
            invalidate();
            return *this;
        }

        /**
         * @brief Returns alignment
         * @return Current alignment.
         */
        GlanceAlignH_t alignment()
        {
            return getText().align;
        }

        /**
         * @brief Appends a C-string to the current text if space allows.
         *
         * Appends up to available capacity (leaving room for the
         * terminating @c '\0') and invalidates the control.
         *
         * @param s C-string to append (ignored if @c nullptr).
         * @return Reference to @c *this for fluent chaining.
         *
         * @note If the current length is already @c >= GLANCE_TEXT_SIZE-1,
         *       the call is ignored.
         */
        ControlText& appendText(const char* s)
        {
            if (s == nullptr) {
                return *this;
            }

            const std::size_t cur = std::strlen(getText().str);

            if (cur + 1 >= GLANCE_TEXT_SIZE) {
                return *this;
            }

            const std::size_t cap = GLANCE_TEXT_SIZE - cur - 1;
            std::strncat(getText().str, s, cap);

            invalidate();

            return *this;
        }

        /**
         * @brief Formats text printf-style into the internal buffer.
         *
         * Writes formatted output into the fixed-size buffer using
         * @c vsnprintf (or @c _vsnprintf_s on MSVC), guarantees NUL
         * termination, and invalidates the control.
         *
         * @param fmt printf-style format string.
         * @param ... Arguments for the format string.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlText& print(const char* fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
        #if defined(_MSC_VER)
            _vsnprintf_s(getText().str, GLANCE_TEXT_SIZE, _TRUNCATE, fmt, args);
        #else
            std::vsnprintf(getText().str, GLANCE_TEXT_SIZE, fmt, args);
            getText().str[GLANCE_TEXT_SIZE - 1] = '\0';
        #endif
            va_end(args);

            invalidate();

            return *this;
        }

    private:
        /**
         * @brief Accessor for the underlying text payload of the bound record.
         * @return Reference to @c GlanceText_t inside the current @c GlanceControl_t.
         *
         * @warning The returned reference remains valid while the owning storage
         *          is alive and the record is not removed.
         */
        GlanceText_t& getText()
        {
            return control().payload.text;
        }
    };

}

#endif
