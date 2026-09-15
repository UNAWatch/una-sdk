/**
 ******************************************************************************
 * @file    WheelMenu.cpp
 * @brief   The UNA activity apps' scroll-wheel menu (see WheelMenu.hpp).
 ******************************************************************************
 */

#include "SDK/GUI/LVGL/WheelMenu.hpp"

#include "SDK/GUI/LVGL/Draw.hpp"

namespace SDK::LVGL
{

namespace
{
namespace Color = SDK::GUI::Color;

constexpr int32_t kWheelY     = 87;
constexpr int32_t kWheelH     = 132;
constexpr int32_t kItemH      = 66;
// The TouchGFX wheel is configured with setSelectedItemMargin(0, 15), but the
// rendered wheel places the next item directly under the selected one: its
// text sits 66 px below the selected item's, not 81 (measured against the
// TouchGFX simulator). One item step is therefore the item height.
constexpr int32_t kItemGap    = 0;
constexpr int32_t kPitch      = kItemH + kItemGap;   // 66: one item step
constexpr int32_t kLensX      = 16;
constexpr int32_t kLensW      = 220;
// The TouchGFX MainMenuBackground is a radius-110 circle centred at (110, 33)
// of the 220 x 66 lens container, so only a 66 px band of it shows and the
// left and right edges are shallow arcs. Reproduced the same way here.
constexpr int32_t kLensDiscRadius = 110;
constexpr int32_t kLensDiscCy     = 33;

// Strip rest positions: slot 1 (the current item) lands at the top of the
// selection window, and at the bottom window's -kPitch (i.e. hidden above it).
constexpr int32_t kSelStripRestY = -kPitch;
constexpr int32_t kOutStripRestY = -2 * kPitch;

// TouchGFX MenuItemConfig geometry.
constexpr int32_t kToggleTextX = 21;
constexpr int32_t kToggleTextW = 128;
constexpr int32_t kToggleX     = 151;
constexpr int32_t kToggleY     = (kItemH - 30) / 2;
constexpr int32_t kCenterTipMsgOffsetY  = 3;    // CenterItemLayout::tip
constexpr int32_t kCenterTipHintOffsetY = -3;
constexpr int32_t kItemTipMsgOffsetY    = 4;    // ItemLayout::tip
constexpr int32_t kItemTipHintOffsetY   = -4;

/// Height a label takes for its current text and width.
int32_t textHeight(lv_obj_t* label)
{
    lv_obj_update_layout(label);
    return lv_obj_get_height(label);
}
} // namespace

WheelMenu::WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, const Fonts& fonts,
                     int16_t itemOffsetY)
    : mItems(items)
    , mCount(count)
    , mFonts(fonts)
    , mItemOffsetY(itemOffsetY)
    , mIndicator(parent, ScrollIndicator::kBig)
{
    // The lens: a clipping container the size of the band, with the disc inside.
    mLens     = Draw::container(parent, kLensX, kWheelY, kLensW, kItemH);
    mLensDisc = Draw::dot(mLens, kLensW / 2, kLensDiscCy, kLensDiscRadius, Color::TEAL_DARK);

    // The wheel area, then the two windows the strips are clipped to: the
    // selection window at the top, and below it the rest of the wheel.
    lv_obj_t* wheel     = Draw::container(parent, 0, kWheelY, 240, kWheelH);
    lv_obj_t* selWindow = Draw::container(wheel, 0, 0, 240, kItemH);
    lv_obj_t* outWindow = Draw::container(wheel, 0, kPitch, 240, kWheelH - kPitch);

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
    strip.obj = Draw::container(window, 0, restY, 240, 3 * kPitch);
    for (int k = 0; k < 3; ++k) {
        lv_obj_t* slotObj = Draw::container(strip.obj, 0, k * kPitch, 240, kItemH);
        Slot& s  = strip.slot[k];
        s.icon   = lv_image_create(slotObj);
        s.label  = Draw::label(slotObj, mFonts.item, "", 0, 0, 240);
        s.tip    = Draw::label(slotObj, mFonts.tip, "", 0, 0, 240);
        s.toggle = std::make_unique<Toggle>(slotObj, kToggleX, kToggleY);
        s.toggle->setVisible(false);
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

void WheelMenu::refresh()
{
    render();
}

void WheelMenu::setBackground(uint32_t color)
{
    lv_obj_set_style_bg_color(mLensDisc, Draw::rgb(color), LV_PART_MAIN);
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
    mMidFired  = false;
    mIndicator.animateTo(mSelected, mAnimationMs, direction);   // jumps when 0
    if (mAnimationMs == 0) {
        finishSlide();
        return;
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_values(&a, 0, kPitch);
    lv_anim_set_duration(&a, mAnimationMs);
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
    if (!self->mMidFired && value >= kPitch / 2) {
        self->fireMid();
    }
}

void WheelMenu::fireMid()
{
    mMidFired = true;
    if (mMidCb) {
        mMidCb(mMidCtx, mSelected);
    }
}

void WheelMenu::animReadyCb(lv_anim_t* a)
{
    static_cast<WheelMenu*>(a->var)->finishSlide();
}

void WheelMenu::finishSlide()
{
    lv_anim_delete(this, nullptr);
    if (!mMidFired) {
        fireMid();
    }
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
        renderSlot(mSelStrip.slot[k], item, true);
        renderSlot(mOutStrip.slot[k], item, false);
    }
}

void WheelMenu::renderSlot(Slot& slot, const Item& item, bool center)
{
    using Style = Item::Style;

    // Reset to the plain layout, then apply the style, as the TouchGFX
    // MainMenuItem::renderStyle does.
    Draw::setHidden(slot.tip, true);
    Draw::setHidden(slot.icon, true);
    slot.toggle->setVisible(false);

    const lv_font_t* font = center ? (item.centerFont ? item.centerFont : mFonts.center) : mFonts.item;
    lv_obj_set_style_text_font(slot.label, font, LV_PART_MAIN);
    lv_obj_set_style_text_align(slot.label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(slot.label, 0, 0);
    lv_obj_set_width(slot.label, 240);
    const char* text = (!center && item.itemText) ? item.itemText : item.text;
    lv_label_set_text(slot.label, text ? text : "");

    // A Toggle item is drawn as text + switch when selected, and as a Tip
    // reading ON/OFF (amber/teal) in the surrounding slot.
    Style style = item.style;
    if (style == Style::Toggle && !center) {
        style = Style::Tip;
    }

    switch (style) {
        case Style::Simple: {
            int32_t y = (kItemH - textHeight(slot.label)) / 2;
            if (!center) {
                y += mItemOffsetY;
            }
            lv_obj_set_y(slot.label, y);
            break;
        }

        case Style::Tip: {
            const int32_t half = kItemH / 2;
            const int32_t labelH = textHeight(slot.label);
            lv_obj_set_y(slot.label, (half - labelH) / 2 + (center ? kCenterTipMsgOffsetY : kItemTipMsgOffsetY));

            const bool    fromToggle = item.style == Style::Toggle;
            const char*   tipText    = fromToggle ? (item.toggleState ? "ON" : "OFF") : (item.tip ? item.tip : "");
            const uint32_t tipColor  = center ? Color::WHITE
                                      : fromToggle ? (item.toggleState ? Color::YELLOW_DARK : Color::TEAL)
                                                   : item.tipColor;
            lv_label_set_text(slot.tip, tipText);
            lv_obj_set_style_text_color(slot.tip, Draw::rgb(tipColor), LV_PART_MAIN);
            lv_obj_set_pos(slot.tip, 0, 0);
            lv_obj_set_width(slot.tip, 240);
            const int32_t tipH = textHeight(slot.tip);
            lv_obj_set_y(slot.tip, half + (half - tipH) / 2 + (center ? kCenterTipHintOffsetY : kItemTipHintOffsetY));
            Draw::setHidden(slot.tip, false);
            break;
        }

        case Style::Toggle: {
            lv_obj_set_style_text_align(slot.label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_set_pos(slot.label, kToggleTextX, 0);
            lv_obj_set_width(slot.label, kToggleTextW);
            lv_obj_set_y(slot.label, (kItemH - textHeight(slot.label)) / 2);
            slot.toggle->setState(item.toggleState);
            slot.toggle->setVisible(true);
            break;
        }

        case Style::Icon: {
            const lv_image_dsc_t* src    = center ? item.centerIcon : item.icon;
            const IconLayout&     layout = center ? item.centerLayout : item.itemLayout;
            if (src) {
                lv_image_set_src(slot.icon, src);
                lv_obj_set_pos(slot.icon, layout.iconX, layout.iconY);
                Draw::setHidden(slot.icon, false);
                lv_obj_set_style_text_align(slot.label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_obj_set_pos(slot.label, layout.textX, 0);
                lv_obj_set_width(slot.label, layout.textW);
            }
            lv_obj_set_y(slot.label, (kItemH - textHeight(slot.label)) / 2);
            break;
        }
    }
}

} // namespace SDK::LVGL
