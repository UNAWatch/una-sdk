/**
 ******************************************************************************
 * @file    Theme.cpp
 * @brief   Shared colours, fonts and styles of the RunLVGL GUI.
 ******************************************************************************
 */

#include "gui/theme/Theme.hpp"

namespace Theme
{

namespace
{
lv_style_t sScreenStyle;
lv_style_t sLabelStyle;
} // namespace

void init()
{
    lv_style_init(&sScreenStyle);
    lv_style_set_bg_color(&sScreenStyle, black());
    lv_style_set_bg_opa(&sScreenStyle, LV_OPA_COVER);
    lv_style_set_text_color(&sScreenStyle, white());
    lv_style_set_pad_all(&sScreenStyle, 0);
    lv_style_set_border_width(&sScreenStyle, 0);
    lv_style_set_radius(&sScreenStyle, 0);

    lv_style_init(&sLabelStyle);
    lv_style_set_text_color(&sLabelStyle, white());
    lv_style_set_bg_opa(&sLabelStyle, LV_OPA_TRANSP);
}

void applyScreen(lv_obj_t* screen)
{
    lv_obj_add_style(screen, &sScreenStyle, LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, const char* text)
{
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &sLabelStyle, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_label_set_text(lbl, text);
    return lbl;
}

} // namespace Theme
