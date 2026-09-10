/**
 ******************************************************************************
 * @file    Theme.cpp
 * @brief   Palette, fonts and drawing helpers shared by every RunLVGL screen.
 ******************************************************************************
 */

#include "gui/theme/Theme.hpp"
#include "gui/Assets.hpp"

namespace Theme
{

namespace
{
lv_style_t sScreenStyle;
lv_style_t sPlainStyle;   // no background, border, padding or radius
} // namespace

const lv_font_t* font(Font f)
{
    switch (f) {
        case Font::Italic18:   return &poppins_italic_18;
        case Font::Medium18:   return &poppins_medium_18;
        case Font::Medium25:   return &poppins_medium_25;
        case Font::Regular14:  return &poppins_regular_14;
        case Font::Regular16:  return &poppins_regular_16;
        case Font::Regular18:  return &poppins_regular_18;
        case Font::SemiBold20: return &poppins_semibold_20;
        case Font::SemiBold25: return &poppins_semibold_25;
        case Font::SemiBold30: return &poppins_semibold_30;
        case Font::SemiBold35: return &poppins_semibold_35;
        case Font::SemiBold40: return &poppins_semibold_40;
        case Font::SemiBold60: return &poppins_semibold_60;
    }
    return &poppins_regular_18;
}

void init()
{
    lv_style_init(&sScreenStyle);
    lv_style_set_bg_color(&sScreenStyle, rgb(SDK::GUI::Color::BLACK));
    lv_style_set_bg_opa(&sScreenStyle, LV_OPA_COVER);
    lv_style_set_text_color(&sScreenStyle, rgb(SDK::GUI::Color::WHITE));
    lv_style_set_pad_all(&sScreenStyle, 0);
    lv_style_set_border_width(&sScreenStyle, 0);
    lv_style_set_radius(&sScreenStyle, 0);

    lv_style_init(&sPlainStyle);
    lv_style_set_bg_opa(&sPlainStyle, LV_OPA_TRANSP);
    lv_style_set_pad_all(&sPlainStyle, 0);
    lv_style_set_border_width(&sPlainStyle, 0);
    lv_style_set_outline_width(&sPlainStyle, 0);
    lv_style_set_radius(&sPlainStyle, 0);
}

void applyScreen(lv_obj_t* screen)
{
    lv_obj_add_style(screen, &sScreenStyle, LV_PART_MAIN);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* container(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t* c = lv_obj_create(parent);
    lv_obj_add_style(c, &sPlainStyle, LV_PART_MAIN);
    lv_obj_remove_flag(c, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    return c;
}

lv_obj_t* label(lv_obj_t* parent, Font f, const char* text,
                int32_t x, int32_t y, int32_t w, lv_text_align_t align, uint32_t color)
{
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &sPlainStyle, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, font(f), LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, align, LV_PART_MAIN);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_width(lbl, w);
    lv_label_set_text(lbl, text);
    return lbl;
}

namespace
{
lv_obj_t* bar(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    lv_obj_t* b = lv_obj_create(parent);
    lv_obj_add_style(b, &sPlainStyle, LV_PART_MAIN);
    lv_obj_remove_flag(b, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, rgb(color), LV_PART_MAIN);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    return b;
}
} // namespace

lv_obj_t* hline(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, uint32_t color)
{
    return bar(parent, x, y, w, 3, color);
}

lv_obj_t* vline(lv_obj_t* parent, int32_t x, int32_t y, int32_t h, uint32_t color)
{
    return bar(parent, x, y, 3, h, color);
}

lv_obj_t* image(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y)
{
    lv_obj_t* img = lv_image_create(parent);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, x, y);
    return img;
}

lv_obj_t* dot(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, uint32_t color)
{
    return bar(parent, cx - radius, cy - radius, 2 * radius, 2 * radius, color);
}

lv_obj_t* arc(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, int32_t width,
              int32_t startDeg, int32_t endDeg, uint32_t color)
{
    // lv_arc draws its arc inside its own box: outer edge at size/2, so size
    // the widget from the centre-line radius plus half the stroke.
    const int32_t outer = radius + (width + 1) / 2;
    lv_obj_t* a = lv_arc_create(parent);
    lv_obj_remove_flag(a, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE));
    lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(a, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(a, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(a, true, LV_PART_MAIN);
    lv_obj_set_style_arc_color(a, rgb(color), LV_PART_MAIN);
    lv_obj_set_size(a, 2 * outer, 2 * outer);
    lv_obj_set_pos(a, cx - outer, cy - outer);
    lv_arc_set_mode(a, LV_ARC_MODE_NORMAL);
    lv_arc_set_rotation(a, 0);
    setArc(a, startDeg, endDeg);
    return a;
}

void setArc(lv_obj_t* arcObj, int32_t startDeg, int32_t endDeg)
{
    lv_arc_set_bg_angles(arcObj, arcAngle(startDeg), arcAngle(endDeg));
}

void setArcColor(lv_obj_t* arcObj, uint32_t color)
{
    lv_obj_set_style_arc_color(arcObj, rgb(color), LV_PART_MAIN);
}

} // namespace Theme
