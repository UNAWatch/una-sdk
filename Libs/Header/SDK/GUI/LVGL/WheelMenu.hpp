/**
 ******************************************************************************
 * @file    WheelMenu.hpp
 * @brief   The UNA activity apps' scroll-wheel menu: a highlighted centre
 *          item over a teal lens, the next item peeking below, and a position
 *          indicator on the left bezel.
 *
 * Geometry follows the TouchGFX apps' MainMenu: wheel at y = 87, items 66 px
 * tall and stacked without a gap, lens at (16, 87) 220 x 66.
 *
 * Item styles: Simple (centred text), Tip (text with a hint line below),
 * Toggle (text with an on/off switch; shown as Tip with ON/OFF when not
 * selected) and Icon (bitmap beside the text).
 *
 * The slide works the way TouchGFX's ScrollWheelWithSelectionStyle does. Two
 * strips hold the previous, current and next items: one rendered in the large
 * selected style and clipped to the 66 px selection window, the other in the
 * small style and clipped to the area below. Both strips move together by one
 * item pitch over the animation time, so the incoming item rises through the
 * gap in small type and appears in the selection window in large type. Items
 * are described once (the caller may change their text and call refresh());
 * the strips are re-rendered from the new selection when the slide lands.
 *
 * Fonts are the app's: pass the faces for the selected item, the surrounding
 * items and the hint line. Each Item may override the selected face.
 ******************************************************************************
 */

#ifndef SDK_GUI_LVGL_WHEEL_MENU_HPP
#define SDK_GUI_LVGL_WHEEL_MENU_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"
#include "SDK/GUI/LVGL/ScrollIndicator.hpp"
#include "SDK/GUI/LVGL/Toggle.hpp"

namespace SDK::LVGL
{

class WheelMenu
{
public:
    /// The faces the wheel renders with.
    struct Fonts {
        const lv_font_t* center;   ///< selected item text (the activity apps: SemiBold 30)
        const lv_font_t* item;     ///< surrounding items' text (Medium 18)
        const lv_font_t* tip;      ///< hint line of a Tip item (Italic 18)
    };

    /// Icon-style geometry for one slot (TouchGFX ItemLayout::icon).
    struct IconLayout {
        int16_t iconX, iconY;   ///< icon position within the 240 x 66 slot
        int16_t textX, textW;   ///< left-aligned text box beside the icon
    };

    struct Item {
        enum class Style : uint8_t { Simple, Tip, Toggle, Icon };

        Style            style      = Style::Simple;
        const char*      text       = nullptr;   ///< main text (may contain '\n')
        const char*      itemText   = nullptr;   ///< text when not selected; nullptr = text
        const lv_font_t* centerFont = nullptr;   ///< selected face; nullptr = Fonts::center

        // Tip: hint line under the text. For Toggle the hint is ON/OFF.
        const char* tip      = nullptr;
        uint32_t    tipColor = SDK::GUI::Color::WHITE;   ///< hint colour when not selected

        // Toggle
        bool toggleState = false;

        // Icon
        const lv_image_dsc_t* centerIcon = nullptr;
        IconLayout            centerLayout { 20, 3, 87, 153 };
        const lv_image_dsc_t* icon       = nullptr;
        IconLayout            itemLayout   { 46, 7, 102, 100 };
    };

    /// Slide time the activity apps use.
    static constexpr uint32_t kDefaultAnimationMs = 400;

    /**
     * @param items        Menu entries, kept alive (and possibly edited) by the caller.
     * @param count        Number of entries.
     * @param fonts        Faces for the selected item, the others and the hint line.
     * @param itemOffsetY  Vertical nudge of a Simple surrounding item's text.
     */
    WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, const Fonts& fonts,
              int16_t itemOffsetY = 0);
    ~WheelMenu();

    WheelMenu(const WheelMenu&)            = delete;
    WheelMenu& operator=(const WheelMenu&) = delete;

    /// Jump to an item with no animation.
    void select(uint16_t index);
    /// Slide to the following / preceding item.
    void next();
    void prev();
    /// Re-render after the caller changed item texts or toggle states.
    void refresh();

    /// The selected item; during a slide, the one being slid to.
    uint16_t selected() const { return mSelected; }
    uint16_t count() const { return mCount; }

    /// Lens colour behind the selected item.
    void setBackground(uint32_t color);

    /// Duration of a slide; 0 makes next()/prev() jump.
    void setAnimationMs(uint32_t ms) { mAnimationMs = ms; }

    /**
     * @brief Called once per slide when it is half way, with the item being
     *        slid to. The activity apps recolour the lens and the button hints
     *        at this point, not when the key is pressed. Fired at the end
     *        instead if a slide is cut short.
     */
    using SlideMidCallback = void (*)(void* ctx, uint16_t target);
    void setSlideMidCallback(SlideMidCallback cb, void* ctx) { mMidCb = cb; mMidCtx = ctx; }

    ScrollIndicator& indicator() { return mIndicator; }

private:
    /// One strip of three slots: previous, current, next.
    struct Slot {
        lv_obj_t* label = nullptr;
        lv_obj_t* tip   = nullptr;
        lv_obj_t* icon  = nullptr;
        std::unique_ptr<Toggle> toggle;
    };
    struct Strip {
        lv_obj_t* obj = nullptr;
        Slot slot[3];
    };

    void buildStrip(Strip& strip, lv_obj_t* window, int32_t restY);
    void render();
    void renderSlot(Slot& slot, const Item& item, bool center);
    void slide(int direction);
    void finishSlide();
    void fireMid();
    static void animExecCb(void* var, int32_t value);
    static void animReadyCb(lv_anim_t* a);

    const Item* mItems;
    uint16_t    mCount;
    Fonts       mFonts;
    uint16_t    mSelected = 0;   ///< target of the current slide, else the shown item
    uint16_t    mShown    = 0;   ///< item the strips are rendered around
    int16_t     mItemOffsetY;
    uint32_t    mAnimationMs = kDefaultAnimationMs;
    bool        mSliding  = false;
    bool        mMidFired = false;
    int         mDirection = 0;
    SlideMidCallback mMidCb  = nullptr;
    void*            mMidCtx = nullptr;

    lv_obj_t* mLens     = nullptr;   ///< clipping band behind the selected item
    lv_obj_t* mLensDisc = nullptr;   ///< the radius-110 disc showing through it
    Strip     mSelStrip;   ///< large style, clipped to the selection window
    Strip     mOutStrip;   ///< small style, clipped to the area below
    ScrollIndicator mIndicator;
};

} // namespace SDK::LVGL

#endif // SDK_GUI_LVGL_WHEEL_MENU_HPP
