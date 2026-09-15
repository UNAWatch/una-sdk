/**
 ******************************************************************************
 * @file    Title.hpp
 * @brief   Screen title at the top: text over a short grey rule, where the
 *          UNA activity apps place theirs.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_TITLE_HPP
#define SDK_GUI_LVGL_TITLE_HPP

#include <cstdint>

#include "lvgl.h"

namespace SDK::LVGL
{

class Title
{
public:
    /// @param font  The title face; the activity apps use an italic 18 px.
    Title(lv_obj_t* parent, const lv_font_t* font, const char* text);
    void setText(const char* text);
    void setColor(uint32_t color);
    void setVisible(bool visible);

private:
    lv_obj_t* mLine  = nullptr;
    lv_obj_t* mLabel = nullptr;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_TITLE_HPP
