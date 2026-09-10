/**
 ******************************************************************************
 * @file    WheelMenu.hpp
 * @brief   The Run app's scroll-wheel menu: a highlighted centre item over a
 *          teal lens, the next item peeking below, and a position indicator.
 *
 * Matches the TouchGFX MainMenu/MainMenuLayout geometry: wheel at y = 87,
 * items 66 px tall, 15 px gap, lens at (16, 87) 220 x 66.
 *
 * The slide works the way TouchGFX's ScrollWheelWithSelectionStyle does. Two
 * strips hold the previous, current and next items: one rendered in the large
 * selected style and clipped to the 66 px selection window, the other in the
 * small style and clipped to the area below the gap. Both strips move together
 * by one item pitch over App::Config::kMenuAnimationMs, so the incoming item
 * rises through the gap in small type and appears in the selection window in
 * large type, exactly as on the TouchGFX wheel. Items are described once; the
 * strips are re-rendered from the new selection when the slide lands.
 ******************************************************************************
 */

#ifndef WHEEL_MENU_HPP
#define WHEEL_MENU_HPP

#include <cstdint>

#include "lvgl.h"

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
        const char*          text        = nullptr;
        Theme::Font          centerFont  = Theme::Font::SemiBold30;
        const lv_image_dsc_t* centerIcon = nullptr;   ///< icon shown when selected
        IconLayout           centerLayout { 20, 3, 87, 153 };
        const lv_image_dsc_t* icon       = nullptr;   ///< icon shown as the next item
        IconLayout           itemLayout   { 46, 7, 102, 100 };
    };

    /**
     * @param items        Menu entries, kept alive by the caller.
     * @param count        Number of entries.
     * @param itemOffsetY  Vertical nudge of the surrounding (next) item's text.
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

    /// The selected item; during a slide, the one being slid to.
    uint16_t selected() const { return mSelected; }
    uint16_t count() const { return mCount; }

    /// Lens colour behind the selected item.
    void setBackground(uint32_t color);

    Widgets::ScrollIndicator& indicator() { return mIndicator; }

private:
    /// One strip of three slots: previous, current, next.
    struct Strip {
        lv_obj_t* obj = nullptr;
        lv_obj_t* label[3] = {};
        lv_obj_t* icon[3]  = {};
    };

    void buildStrip(Strip& strip, lv_obj_t* window, int32_t restY);
    void render();
    void renderSlot(lv_obj_t* label, lv_obj_t* icon, const Item& item, bool center);
    void slide(int direction);
    void finishSlide();
    static void animExecCb(void* var, int32_t value);
    static void animReadyCb(lv_anim_t* a);

    const Item* mItems;
    uint16_t    mCount;
    uint16_t    mSelected = 0;   ///< target of the current slide, else the shown item
    uint16_t    mShown    = 0;   ///< item the strips are rendered around
    int16_t     mItemOffsetY;
    bool        mSliding  = false;
    int         mDirection = 0;

    lv_obj_t* mLens = nullptr;
    Strip     mSelStrip;   ///< large style, clipped to the selection window
    Strip     mOutStrip;   ///< small style, clipped to the area below the gap
    Widgets::ScrollIndicator mIndicator;
};

#endif // WHEEL_MENU_HPP
