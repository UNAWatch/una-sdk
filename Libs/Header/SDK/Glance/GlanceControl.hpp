/**
 ******************************************************************************
 * @file    GlanceControl.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   API for use in Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_HPP
#define __GLANCE_CONTROL_HPP

#include "SDK/Glance/GlanceControlText.hpp"
#include "SDK/Glance/GlanceControlImage.hpp"
#include "SDK/Glance/GlanceControlLine.hpp"
#include "SDK/Glance/GlanceControlRect.hpp"

#include <cstdint>
#include <cstdbool>
#include <vector>

namespace SDK::Glance {

    class Align {
    public:
        // Returns top-left (origin) for obj placed inside area with H/V alignment.
        static GlancePoint_t placeWithin(GlancePoint_t  areaPos,
                                         GlanceSize_t   areaSize,
                                         GlanceSize_t   objSize,
                                         GlanceAlignH_t alignH,
                                         GlanceAlignV_t alignV)
        {
            // Signed deltas to avoid unsigned underflow
            int32_t dx = (int32_t)areaSize.w - (int32_t)objSize.w;
            int32_t dy = (int32_t)areaSize.h - (int32_t)objSize.h;

            // Horizontal placement (X): LEFT/CENTER/RIGHT lives in alignH
            int32_t offX;
            switch (alignH) {
                case GLANCE_ALIGN_H_LEFT:   offX = 0;      break;
                case GLANCE_ALIGN_H_CENTER: offX = dx / 2; break;
                case GLANCE_ALIGN_H_RIGHT:  offX = dx;     break;
                default:                    offX = 0;      break;
            }

            // Vertical placement (Y): TOP/CENTER/BOTTOM lives in alignV
            int32_t offY;
            switch (alignV) {
                case GLANCE_ALIGN_V_TOP:    offY = 0;      break;
                case GLANCE_ALIGN_V_CENTER: offY = dy / 2; break;
                case GLANCE_ALIGN_V_BOTTOM: offY = dy;     break;
                default:                    offY = 0;      break;
            }

            // Clamp negatives to avoid underflow when object is larger than area
            if (offX < 0) {
                offX = 0;
            }

            if (offY < 0) {
                offY = 0;
            }

            uint32_t x = (uint32_t)areaPos.x + (uint32_t)offX;
            uint32_t y = (uint32_t)areaPos.y + (uint32_t)offY;

            if (x > 0xFFFFu) {
                x = 0xFFFFu;
            }

            if (y > 0xFFFFu) {
                y = 0xFFFFu;
            }

            GlancePoint_t pos = { static_cast<uint16_t>(x), static_cast<uint16_t>(y) };

            return pos;
        }
    };

    /**
     * @brief Builder/container for a contiguous list of Glance controls.
     *
     * This class owns a dynamically-growing contiguous buffer of `GlanceControl_t`
     * and provides typed factory methods (`createText()`, `createImage()`, etc.)
     * that append a new record and return a lightweight view (e.g., `ControlText`)
     * bound to the internal storage and the index of the created element.
     *
     * @note The returned views are non-owning and are expected to reference the
     *       internal vector by (vector, index). Appending more items does not
     *       invalidate such views (they re-locate the element by index each time).
     *       However, any raw pointer obtained via `data()` may be invalidated by
     *       subsequent appends if reallocation occurs. Prefer to call `data()` only
     *       after you finish building the form for the current frame.
     */
    class Form {
    public:

        /**
         * @brief Constructs an empty form with an initial reserved capacity.
         *
         * The constructor reserves space for 16 elements to reduce early reallocations.
         * Capacity will grow automatically as needed on subsequent appends.
         */
        Form() noexcept
            : mControls{}
            , mWidth(0)
            , mHeight(0)
        {
            mControls.reserve(16);
        }

        /**
         * @brief Constructs an empty form with an initial reserved capacity.
         *
         * The constructor reserves space for 16 elements to reduce early reallocations.
         * Capacity will grow automatically as needed on subsequent appends.
         */
        explicit Form(uint16_t w, uint16_t h) noexcept
            : mControls{}
            , mWidth(w)
            , mHeight(h)

        {
            mControls.reserve(16);
        }

        void setWidth(uint16_t w)  { mWidth = w;  }
        void setHeight(uint16_t h) { mHeight = h; }

        uint16_t getWidth()        { return mWidth;  }
        uint16_t getHeight()       { return mHeight; }

        /**
         * @brief Current number of controls stored in the form.
         * @return The number of appended elements.
         *
         * @warning Calling `create*()` after taking a raw pointer from `data()`
         *          may invalidate that pointer due to vector reallocation.
         *          Fetch `data()`/`size()` after you finish building.
         */
        size_t size() { return mControls.size(); }

        /**
         * @brief Returns a raw pointer to the underlying contiguous array.
         * @return Pointer to the first element, or `nullptr` if the form is empty.
         *
         * @note The memory is owned by this `Form` and remains valid until the
         *       `Form` is destroyed or the internal vector reallocated (e.g., by
         *       calling another `create*()` that grows capacity).
         */
        GlanceControl_t* data() { return mControls.data(); }

        /**
         * @brief Appends a new TEXT control and returns a typed view for configuration.
         * @return A lightweight `ControlText` view bound to the newly appended record.
         */
        ControlText createText()  { return ControlText(&mControls, append(GLANCE_TYPE_TEXT)); }

        /**
         * @brief Appends a new IMAGE control and returns a typed view for configuration.
         * @return A lightweight `ControlImage` view bound to the newly appended record.
         */
        ControlImage createImage() { return ControlImage(&mControls, append(GLANCE_TYPE_IMAGE)); }

        /**
         * @brief Appends a new LINE control and returns a typed view for configuration.
         * @return A lightweight `ControlLine` view bound to the newly appended record.
         */
        ControlLine createLine()  { return ControlLine(&mControls, append(GLANCE_TYPE_LINE)); }

        /**
         * @brief Appends a new RECT control and returns a typed view for configuration.
         * @return A lightweight `ControlRectangle` view bound to the newly appended record.
         */
        ControlRectangle createRect()  { return ControlRectangle(&mControls, append(GLANCE_TYPE_RECT)); }

    private:
        /**
         * @brief Appends a zero-initialized control with the given type tag.
         *
         * The new record is value-initialized (`GlanceControl_t{}`), then its
         * `type` field is set to @p t, and it is pushed to the end of the vector.
         *
         * @param t The type tag to place into the new `GlanceControl_t`.
         * @return The index of the newly appended element (0-based).
         *
         * @complexity Amortized O(1). May trigger reallocation when capacity grows.
         */
        std::size_t append(GlanceType_t t)
        {
            GlanceControl_t c{};
            c.type = t;
            
            mControls.push_back(c);
            
            return mControls.size() - 1;
        }

        /** @brief Contiguous storage of control records owned by this form. */
        std::vector<GlanceControl_t> mControls;
        uint16_t                     mWidth;
        uint16_t                     mHeight;

    };

}

#endif
