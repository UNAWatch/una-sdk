/**
 ******************************************************************************
 * @file    GlanceControlLine.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Line control for Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_LINE_HPP
#define __GLANCE_CONTROL_LINE_HPP

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
     * @brief Typed view over a line control record.
     *
     * A lightweight wrapper bound to a specific element inside a contiguous
     * storage of @c GlanceControl_t records. All mutating operations write
     * directly into the underlying record and call @c invalidate() to mark
     * the control as needing redraw.
     *
     * @note This class does not own the underlying storage. It is expected
     * to be created by a form/builder that owns a @c std::vector<GlanceControl_t>.
     * The view remains valid while the owning storage is alive and the indexed
     * record is not removed.
     */
    class ControlLine : public Control {
    public:
        /**
         * @brief Binds this view to an existing control record.
         *
         * @param controls Reference to the owning vector of control records.
         * @param idx      Index of the line record inside @p controls.
         */
        ControlLine(std::vector<GlanceControl_t>& controls, std::size_t idx)
            : Control(controls, idx)
        {}

        /// @brief Virtual dtor from base; nothing to do.
        ~ControlLine() override = default;

        /**
         * @brief Initializes the line in a single call.
         *
         * Sets start/stop points and color, and invalidates the control.
         *
         * @param start Start point of the line.
         * @param stop  End point of the line.
         * @param color Line color (RGB222 or palette index).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlLine& init(GlancePoint_t start, GlancePoint_t stop, uint8_t color)
        {
            GlanceLine_t& line = getLine();

            line.start = start;
            line.stop  = stop;
            line.color = color;

            invalidate();

            return *this;
        }

        /**
         * @brief Sets the start point of the line.
         *
         * Invalidates the control after updating the payload.
         *
         * @param start New start point.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlLine& pos(GlancePoint_t start)
        {
            getLine().start = start;

            invalidate();

            return *this;
        }

        /**
         * @brief Gets the current start point of the line.
         * @return Current start point.
         */
        GlancePoint_t pos()
        {
            return getLine().start;
        }

        /**
         * @brief Sets the end point of the line.
         *
         * Invalidates the control after updating the payload.
         *
         * @param stop New end point.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlLine& to(GlancePoint_t stop)
        {
            getLine().stop = stop;

            invalidate();

            return *this;
        }
        
        /**
         * @brief Sets the line color.
         *
         * Invalidates the control after updating the payload.
         *
         * @param color New color value (RGB222 or palette index).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlLine& color(uint8_t color)
        {
            getLine().color = color;

            invalidate();

            return *this;
        }

    private:
        /**
         * @brief Accessor for the underlying line payload of the bound record.
         * @return Reference to @c GlanceLine_t inside the current @c GlanceControl_t.
         *
         * @warning The returned reference is valid while the owning storage
         *          remains alive and the record is not removed.
         */
        GlanceLine_t& getLine()
        {
            return control().payload.line;
        }
    };

}

#endif
