/**
 ******************************************************************************
 * @file    Draw.cpp
 * @brief   Drawing helpers for an LVGL GUI process on the watch.
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL::Draw
{

namespace
{
lv_style_t sScreenStyle;
lv_style_t sPlainStyle;   // no background, border, padding or radius

lv_obj_t* plainObject(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_add_style(o, &sPlainStyle, LV_PART_MAIN);
    lv_obj_remove_flag(o, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    return o;
}
} // namespace

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

void setHidden(lv_obj_t* obj, bool hidden)
{
    if (hidden) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* container(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    return plainObject(parent, x, y, w, h);
}

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, const char* text,
                int32_t x, int32_t y, int32_t w, lv_text_align_t align, uint32_t color)
{
    lv_obj_t* lbl = lv_label_create(parent);
    lv_obj_add_style(lbl, &sPlainStyle, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, rgb(color), LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, align, LV_PART_MAIN);
    lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_width(lbl, w);
    lv_label_set_text(lbl, text);
    return lbl;
}

lv_obj_t* box(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, int32_t radius)
{
    lv_obj_t* b = plainObject(parent, x, y, w, h);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(b, rgb(color), LV_PART_MAIN);
    lv_obj_set_style_radius(b, radius, LV_PART_MAIN);
    return b;
}

lv_obj_t* hline(lv_obj_t* parent, int32_t x, int32_t y, int32_t w, uint32_t color)
{
    return box(parent, x, y, w, 3, color, LV_RADIUS_CIRCLE);
}

lv_obj_t* vline(lv_obj_t* parent, int32_t x, int32_t y, int32_t h, uint32_t color)
{
    return box(parent, x, y, 3, h, color, LV_RADIUS_CIRCLE);
}

lv_obj_t* image(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y)
{
    lv_obj_t* img = lv_image_create(parent);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, x, y);
    return img;
}

lv_obj_t* imageTinted(lv_obj_t* parent, const lv_image_dsc_t* src, int32_t x, int32_t y, uint32_t color)
{
    lv_obj_t* img = image(parent, src, x, y);
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, LV_PART_MAIN);
    tint(img, color);
    return img;
}

void tint(lv_obj_t* img, uint32_t color)
{
    lv_obj_set_style_image_recolor(img, rgb(color), LV_PART_MAIN);
}

lv_obj_t* dot(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, uint32_t color)
{
    return box(parent, cx - radius, cy - radius, 2 * radius, 2 * radius, color, LV_RADIUS_CIRCLE);
}

lv_obj_t* arc(lv_obj_t* parent, int32_t cx, int32_t cy, int32_t radius, int32_t width,
              int32_t startDeg, int32_t endDeg, uint32_t color, bool rounded)
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
    lv_obj_set_style_arc_rounded(a, rounded, LV_PART_MAIN);
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

} // namespace SDK::LVGL::Draw
