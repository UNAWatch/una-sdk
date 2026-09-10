/**
 ******************************************************************************
 * @file    WheelMenu.cpp
 * @brief   The Run app's scroll-wheel menu (see WheelMenu.hpp).
 ******************************************************************************
 */

#include "gui/widgets/WheelMenu.hpp"
#include "gui/model/Model.hpp"

using namespace SDK::GUI;

namespace
{
constexpr int32_t kWheelY     = 87;
constexpr int32_t kWheelH     = 132;
constexpr int32_t kItemH      = 66;
constexpr int32_t kItemGap    = 15;
constexpr int32_t kPitch      = kItemH + kItemGap;   // 81: one item step
constexpr int32_t kLensX      = 16;
constexpr int32_t kLensW      = 220;
constexpr int32_t kLensRadius = 22;   // stands in for the clipped radius-110 circle

// Strip rest positions: slot 1 (the current item) lands at the top of the
// selection window, and at the bottom window's -kPitch (i.e. hidden above it).
constexpr int32_t kSelStripRestY = -kPitch;
constexpr int32_t kOutStripRestY = -2 * kPitch;
} // namespace

WheelMenu::WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY)
    : mItems(items)
    , mCount(count)
    , mItemOffsetY(itemOffsetY)
    , mIndicator(parent, Widgets::ScrollIndicator::kBig)
{
    mLens = Theme::container(parent, kLensX, kWheelY, kLensW, kItemH);
    lv_obj_set_style_bg_opa(mLens, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mLens, Theme::rgb(Color::TEAL_DARK), LV_PART_MAIN);
    lv_obj_set_style_radius(mLens, kLensRadius, LV_PART_MAIN);

    // The wheel area, then the two windows the strips are clipped to: the
    // selection window at the top, and below the gap the rest of the wheel.
    lv_obj_t* wheel     = Theme::container(parent, 0, kWheelY, 240, kWheelH);
    lv_obj_t* selWindow = Theme::container(wheel, 0, 0, 240, kItemH);
    lv_obj_t* outWindow = Theme::container(wheel, 0, kPitch, 240, kWheelH - kPitch);

    buildStrip(mSelStrip, selWindow, kSelStripRestY);
    buildStrip(mOutStrip, outWindow, kOutStripRestY);

    mIndicator.setCount(count);
    render();
}

WheelMenu::~WheelMenu()
{
    lv_anim_delete(this, nullptr);
}

void WheelMenu::buildStrip(Strip& strip, lv_obj_t* window, int32_t restY)
{
    strip.obj = Theme::container(window, 0, restY, 240, 3 * kPitch);
    for (int k = 0; k < 3; ++k) {
        lv_obj_t* slot = Theme::container(strip.obj, 0, k * kPitch, 240, kItemH);
        strip.icon[k]  = lv_image_create(slot);
        strip.label[k] = Theme::label(slot, Theme::Font::Medium18, "", 0, 0, 240);
    }
}

void WheelMenu::select(uint16_t index)
{
    if (mCount == 0) {
        return;
    }
    if (mSliding) {
        finishSlide();
    }
    mSelected = mShown = static_cast<uint16_t>(index % mCount);
    mIndicator.setActive(mSelected);
    render();
}

void WheelMenu::next()
{
    slide(+1);
}

void WheelMenu::prev()
{
    slide(-1);
}

void WheelMenu::setBackground(uint32_t color)
{
    lv_obj_set_style_bg_color(mLens, Theme::rgb(color), LV_PART_MAIN);
}

void WheelMenu::slide(int direction)
{
    if (mCount <= 1) {
        return;
    }
    if (mSliding) {
        finishSlide();   // a second press mid-slide lands the first, then starts anew
    }
    mDirection = direction;
    mSelected  = static_cast<uint16_t>((mShown + mCount + direction) % mCount);
    mSliding   = true;
    mIndicator.animateTo(mSelected, App::Config::kMenuAnimationMs);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, kPitch);
    lv_anim_set_duration(&a, App::Config::kMenuAnimationMs);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);   // TouchGFX: linearEaseOut
    lv_anim_set_exec_cb(&a, &WheelMenu::animExecCb);
    lv_anim_set_completed_cb(&a, &WheelMenu::animReadyCb);
    lv_anim_start(&a);
}

void WheelMenu::animExecCb(void* var, int32_t value)
{
    auto* self = static_cast<WheelMenu*>(var);
    // Sliding to the next item moves the strips up; to the previous, down.
    const int32_t shift = -self->mDirection * value;
    lv_obj_set_y(self->mSelStrip.obj, kSelStripRestY + shift);
    lv_obj_set_y(self->mOutStrip.obj, kOutStripRestY + shift);
}

void WheelMenu::animReadyCb(lv_anim_t* a)
{
    static_cast<WheelMenu*>(a->var)->finishSlide();
}

void WheelMenu::finishSlide()
{
    lv_anim_delete(this, nullptr);
    mSliding = false;
    mShown   = mSelected;
    render();
    lv_obj_set_y(mSelStrip.obj, kSelStripRestY);
    lv_obj_set_y(mOutStrip.obj, kOutStripRestY);
}

void WheelMenu::render()
{
    if (mCount == 0) {
        return;
    }
    // Slot 0 = previous, 1 = current, 2 = next, all around the shown item.
    for (int k = 0; k < 3; ++k) {
        const Item& item = mItems[(mShown + mCount + k - 1) % mCount];
        renderSlot(mSelStrip.label[k], mSelStrip.icon[k], item, true);
        renderSlot(mOutStrip.label[k], mOutStrip.icon[k], item, false);
    }
}

void WheelMenu::renderSlot(lv_obj_t* label, lv_obj_t* icon, const Item& item, bool center)
{
    const lv_font_t*      font    = center ? Theme::font(item.centerFont) : Theme::font(Theme::Font::Medium18);
    const lv_image_dsc_t* iconSrc = center ? item.centerIcon : item.icon;
    const IconLayout&     layout  = center ? item.centerLayout : item.itemLayout;

    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_text(label, item.text ? item.text : "");

    // TouchGFX's centerTextY: the text box is sized to the text and centred in
    // the 66 px slot, then nudged by the layout offset for the surrounding item.
    const int32_t textH = lv_font_get_line_height(font);
    int32_t y = (kItemH - textH) / 2;
    if (!center) {
        y += mItemOffsetY;
    }

    if (iconSrc) {
        lv_image_set_src(icon, iconSrc);
        lv_obj_set_pos(icon, layout.iconX, layout.iconY);
        lv_obj_remove_flag(icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(label, layout.textX, y);
        lv_obj_set_width(label, layout.textW);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    } else {
        lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(label, 0, y);
        lv_obj_set_width(label, 240);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
}
