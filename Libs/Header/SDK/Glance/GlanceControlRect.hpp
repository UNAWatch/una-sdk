/**
 ******************************************************************************
 * @file    GlanceControlRect.hpp
 * @date    18-September-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Rectangle control for Glance applications
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __GLANCE_CONTROL_RECT_HPP
#define __GLANCE_CONTROL_RECT_HPP

#include "SDK/Glance/GlanceControlBase.hpp"

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <stdlib.h>

namespace SDK::Glance {

/**
 * @brief Typed view over a rectangle control record.
 *
 * Легка обгортка навколо елемента у векторі `GlanceControl_t`.
 * Зміни записуються безпосередньо в буфер форми. Обгортка не володіє ресурсами.
 */
class ControlRectangle : public Control {
public:
    /**
     * @brief Bind this view to an existing control slot.
     * @param controls Reference to the owning buffer.
     * @param idx      Index of the rectangle item in the buffer.
     */
    ControlRectangle(std::vector<GlanceControl_t>* controls, std::size_t idx)
        : Control(controls, idx)
    {}

    /// @brief Virtual dtor from base; nothing to do.
    ~ControlRectangle() override = default;

    /**
     * @brief Initialize rectangle in one call.
     * @param pos       Top-left position.
     * @param size      Size (width/height).
     * @param lineColor RGB222 line color.
     * @param bgColor   RGB222 background color.
     * @param fill      Whether to fill background.
     * @return *this for fluent chaining.
     */
    ControlRectangle& init(GlancePoint_t pos,
                           GlanceSize_t  size,
                           uint8_t       lineColor,
                           uint8_t       bgColor,
                           bool          fill = true)
    {
        auto& rect = getRect();
        
        rect.pos     = pos;
        rect.size    = size;
        rect.color   = lineColor;
        rect.bgColor = bgColor;
        rect.fill    = fill;
        
        invalidate();
        
        return *this;
    }

    /**
     * @brief Set new position
     * @param p       Top-left position.
     * @return *this for fluent chaining.
     */
    ControlRectangle& pos(GlancePoint_t p)
    {
        getRect().pos = p;
        invalidate();
        return *this;
    }

    /**
     * @brief Returns current top-left position
     * @return Current top-left position
     */
    GlancePoint_t pos()
    {
        return getRect().pos;
    }

    /**
     * @brief Set size
     * @param sz      Size (width/height).
     * @return *this for fluent chaining.
     */
    ControlRectangle& size(GlanceSize_t sz)
    {
        getRect().size = sz;
        invalidate();
        return *this;
    }

    /**
     * @brief Returns size
     * @return Current size (w/h).
     */
    GlanceSize_t size()
    {
        return getRect().size;
    }
    
    /**
     * @brief Sets line color
     * @param c RGB222 line color.
     * @return *this for fluent chaining.
     */
    ControlRectangle& color(uint8_t c)
    {
        getRect().color = c;
        invalidate();
        return *this;
    }

    /**
     * @brief Sets background color
     * @param c   RGB222 background color.
     * @return *this for fluent chaining.
     */
    ControlRectangle& bgColor(uint8_t c)
    {
        getRect().bgColor = c;
        invalidate();
        return *this;
    }

    /**
     * @brief Sets fill background flag
     * @param f Whether to fill background
     * @return *this for fluent chaining.
     */
     ControlRectangle& fill(bool f = true)
    {
        getRect().fill = f;
        invalidate();
        return *this;
    }

private:
    /// @brief Non-const access to the rectangle payload in the underlying record.
    GlanceRect_t& getRect()
    {
        return control().payload.rect;
    }
};


}

#endif
