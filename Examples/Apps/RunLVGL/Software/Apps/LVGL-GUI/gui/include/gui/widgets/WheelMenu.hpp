/**
 ******************************************************************************
 * @file    WheelMenu.hpp
 * @brief   The Run app's scroll-wheel menu: a highlighted centre item over a
 *          teal lens, the next item peeking below, and a position indicator.
 *
 * Matches the TouchGFX MainMenu/MainMenuLayout geometry: wheel at y = 87,
 * items 66 px tall and stacked without a gap, lens at (16, 87) 220 x 66.
 *
 * Item styles follow MenuItemConfig::Style: Simple (centred text), Tip (text
 * with a hint line below), Toggle (text with an on/off switch; the item shows
 * as Tip with ON/OFF when not selected) and Icon (bitmap beside the text).
 *
 * The slide works the way TouchGFX's ScrollWheelWithSelectionStyle does. Two
 * strips hold the previous, current and next items: one rendered in the large
 * selected style and clipped to the 66 px selection window, the other in the
 * small style and clipped to the area below the gap. Both strips move together
 * by one item pitch over App::Config::kMenuAnimationMs, so the incoming item
 * rises through the gap in small type and appears in the selection window in
 * large type, exactly as on the TouchGFX wheel. Items are described once (the
 * caller may change their text and call refresh()); the strips are re-rendered
 * from the new selection when the slide lands.
 ******************************************************************************
 */

#ifndef WHEEL_MENU_HPP
#define WHEEL_MENU_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "SDK/GUI/Color.hpp"
#include "gui/theme/Theme.hpp"
#include "gui/widgets/Widgets.hpp"

class WheelMenu
{
public:
    /// Icon-style geometry for one slot (TouchGFX ItemLayout::icon).
    struct IconLayout {
        int16_t iconX, iconY;   ///< icon position within the 240 x 66 slot
        int16_t textX, textW;   ///< left-aligned text box beside the icon
    };

    struct Item {
        enum class Style : uint8_t { Simple, Tip, Toggle, Icon };

        Style       style      = Style::Simple;
        const char* text       = nullptr;     ///< main text (may contain '\n')
        const char* itemText   = nullptr;     ///< text when not selected; nullptr = text
        Theme::Font centerFont = Theme::Font::SemiBold30;

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

    /**
     * @param items        Menu entries, kept alive (and possibly edited) by the caller.
     * @param count        Number of entries.
     * @param itemOffsetY  Vertical nudge of a Simple surrounding item's text.
     */
    WheelMenu(lv_obj_t* parent, const Item* items, uint16_t count, int16_t itemOffsetY = 0);
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

    /**
     * @brief Called once per slide when it is half way, with the item being
     *        slid to. The Run app recolours the lens and the button hints at
     *        this point, not when the key is pressed. Fired at the end instead
     *        if a slide is cut short.
     */
    using SlideMidCallback = void (*)(void* ctx, uint16_t target);
    void setSlideMidCallback(SlideMidCallback cb, void* ctx) { mMidCb = cb; mMidCtx = ctx; }

    Widgets::ScrollIndicator& indicator() { return mIndicator; }

private:
    /// One strip of three slots: previous, current, next.
    struct Slot {
        lv_obj_t* label = nullptr;
        lv_obj_t* tip   = nullptr;
        lv_obj_t* icon  = nullptr;
        std::unique_ptr<Widgets::Toggle> toggle;
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
    uint16_t    mSelected = 0;   ///< target of the current slide, else the shown item
    uint16_t    mShown    = 0;   ///< item the strips are rendered around
    int16_t     mItemOffsetY;
    bool        mSliding  = false;
    bool        mMidFired = false;
    int         mDirection = 0;
    SlideMidCallback mMidCb  = nullptr;
    void*            mMidCtx = nullptr;

    lv_obj_t* mLens     = nullptr;   ///< clipping band behind the selected item
    lv_obj_t* mLensDisc = nullptr;   ///< the radius-110 disc showing through it
    Strip     mSelStrip;   ///< large style, clipped to the selection window
    Strip     mOutStrip;   ///< small style, clipped to the area below the gap
    Widgets::ScrollIndicator mIndicator;
};

#endif // WHEEL_MENU_HPP
