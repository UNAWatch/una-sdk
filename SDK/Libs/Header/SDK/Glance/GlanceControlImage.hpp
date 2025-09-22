/**
 ******************************************************************************
 * @file    GlanceControlImage.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Image control for Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_IMAGE_HPP
#define __GLANCE_CONTROL_IMAGE_HPP

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
     * @brief Typed view over an image control record.
     *
     * Lightweight wrapper bound to a specific element inside a contiguous
     * storage of @c GlanceControl_t records. Mutating operations write
     * directly into the underlying record and call @c invalidate() to mark
     * the control as needing redraw.
     *
     * @note This class does not own the image buffer. The pointer passed via
     *       @c init() / @c setImage() must remain valid for the lifetime of
     *       the control or until changed.
     */
    class ControlImage : public Control {
    public:
        /**
         * @brief Binds this view to an existing control record.
         *
         * @param controls Reference to the owning vector of control records.
         * @param idx      Index of the image record inside @p controls.
         */
        ControlImage(std::vector<GlanceControl_t>& controls, std::size_t idx)
            : Control(controls, idx)
        {}

        /// @brief Virtual dtor from base; nothing to do.
        ~ControlImage() override = default;

        /**
         * @brief Initializes image in a single call.
         *
         * Sets position, size and backing buffer, and invalidates the control.
         *
         * @param pos  Top-left position.
         * @param size Image size (width/height).
         * @param buff Pointer to image data (must remain valid while used).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlImage& init(GlancePoint_t pos, GlanceSize_t size, const uint8_t* buff)
        {
            GlanceImage_t& image = getImage();

            image.pos  = pos;
            image.size = size;
            image.buff = buff;

            invalidate();

            return *this;
        }

        /**
         * @brief Updates the image buffer pointer.
         *
         * @param buff Pointer to image data (caller retains ownership).
         * @return Reference to @c *this for fluent chaining.
         */
        ControlImage& setImage(const uint8_t* buff)
        {
            getImage().buff = buff;

            invalidate();

            return *this;
        }

        /**
         * @brief Sets the top-left position of the image.
         *
         * @param pos New position.
         * @return Reference to @c *this for fluent chaining.
         */
        ControlImage& pos(GlancePoint_t pos)
        {
            getImage().pos = pos;

            invalidate();

            return *this;
        }

        /**
         * @brief Gets the current top-left position of the image.
         * @return Current position.
         */
        GlancePoint_t pos()
        {
            return getImage().pos;
        }

        /**
         * @brief Sets the size of the image.
         *
         * @param size New size (width/height).
         * @return Reference to @c *this for fluent chaining.
         */
        /// @brief Resize for types that have GlanceSize_t.
        ControlImage& size(GlanceSize_t size)
        {
            getImage().size = size;

            invalidate();

            return *this;
        }

    private:
        /**
         * @brief Accessor for the underlying image payload of the bound record.
         * @return Reference to @c GlanceImage_t inside the current @c GlanceControl_t.
         *
         * @warning The returned reference remains valid while the owning storage
         *          is alive and the record is not removed.
         */
        GlanceImage_t& getImage()
        {
            return control().payload.image;
        }
    };

}

#endif
