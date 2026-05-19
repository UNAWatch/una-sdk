#ifndef MENUITEMCONFIG_HPP
#define MENUITEMCONFIG_HPP

#include <touchgfx/hal/Types.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Color.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

// =============================================================================
// CenterItemLayout
// =============================================================================

/**
 * @brief Geometry layout for MainMenuCenterItem (default container: 240 x 66 px).
 *
 * Fields are grouped by style. Only the sub-struct relevant to the item's style
 * is used during rendering; the rest are ignored.
 *
 * Set once per menu via MainMenu::setCenterItemLayout().
 * Override per individual item via MenuItemConfig::centerLayout.
 * Omitting both uses the built-in defaults.
 *
 * @code
 * static CenterItemLayout myLayout;
 * myLayout.inl.mainX = 37;
 * myLayout.inl.mainW = 130;
 * item.centerLayout = &myLayout;
 * @endcode
 */
struct CenterItemLayout
{
    struct {
        int16_t textX = 21;   ///< Text X when toggle is visible
        int16_t textW = 128;  ///< Text width when toggle is visible
        int16_t x     = 151;  ///< Toggle widget X; Y is always centred vertically
    } toggle;

    struct {
        int16_t msgOffsetY  = 3;   ///< Main text Y nudge from centre of top half  (+ = down)
        int16_t hintOffsetY = -3;  ///< Hint text Y nudge from centre of bottom half (+ = down)
    } tip;

    struct {
        int16_t mainX = 10;   ///< Main text X
        int16_t mainW = 110;  ///< Main text width
        int16_t tipX  = 130;  ///< Hint label X
        int16_t tipW  = 110;  ///< Hint label width
    } inl;

    struct {
        int16_t x     = 20;   ///< Icon X
        int16_t y     = 3;    ///< Icon Y
        int16_t w     = 60;   ///< Icon width
        int16_t h     = 60;   ///< Icon height
        int16_t textX = 87;   ///< Text X when icon is shown
        int16_t textW = 153;  ///< Text width when icon is shown
    } icon;
};


// =============================================================================
// ItemLayout
// =============================================================================

/**
 * @brief Geometry layout for MainMenuItem / surrounding items (default container: 240 x 60 px).
 *
 * Fields are grouped by style. Only the sub-struct relevant to the item's style
 * is used during rendering; the rest are ignored.
 *
 * Set once per menu via MainMenu::setItemLayout().
 * Override per individual item via MenuItemConfig::itemLayout.
 * Omitting both uses the built-in defaults.
 *
 * @code
 * static ItemLayout myLayout;
 * myLayout.inl.mainX = 70;
 * myLayout.inl.mainW = 50;
 * item.itemLayout = &myLayout;
 * @endcode
 */
struct ItemLayout
{
    struct {
        int16_t msgOffsetY = 0;  ///< Main text Y nudge from vertical centre (+ = down)
    } simple;

    struct {
        int16_t msgOffsetY  = 4;   ///< Main text Y nudge from centre of top half  (+ = down)
        int16_t hintOffsetY = -4;  ///< Hint text Y nudge from centre of bottom half (+ = down)
    } tip;

    struct {
        int16_t mainX = 10;   ///< Main text X
        int16_t mainW = 110;  ///< Main text width
        int16_t tipX  = 130;  ///< Hint label X
        int16_t tipW  = 110;  ///< Hint label width
    } inl;

    struct {
        int16_t x     = 46;   ///< Icon X
        int16_t y     = 7;    ///< Icon Y
        int16_t w     = 46;   ///< Icon width
        int16_t h     = 46;   ///< Icon height
        int16_t textX = 102;  ///< Text X when icon is shown
        int16_t textW = 100;  ///< Text width when icon is shown
    } icon;
};


// =============================================================================
// MenuItemConfig
// =============================================================================

/**
 * @brief Configuration for a single menu item.
 *
 * Owned by the screen. Passed into MainMenuCenterItem / MainMenuItem
 * via apply() inside the wheel update callback.
 *
 * Text source priority:  msgId (TypedText) > msgText (UnicodeChar*) > msgChar (UTF-8)
 * Layout priority:       cfg.centerLayout / cfg.itemLayout
 *                        > MainMenu::setCenterItemLayout / setItemLayout()
 *                        > built-in defaults
 */
struct MenuItemConfig
{
    /**
     * @brief Visual style -- defines which widgets are shown.
     */
    enum Style
    {
        SIMPLE   = 0,  ///< Centred text only
        TOGGLE,        ///< Text + interactive toggle switch  (MainMenuCenterItem only)
        TIP,           ///< Main text + smaller hint below
        INLINE,        ///< Main text + hint on the same line, baseline-aligned
        ICON,          ///< Static bitmap icon + text
        ICON_EXT,      ///< External (dynamic) bitmap icon + text  (MainMenuCenterItem only)
    };

    Style style = SIMPLE;

    // ---- Main text ----------------------------------------------------------
    TypedTextId msgId     = TYPED_TEXT_INVALID; ///< Localized text (highest priority)
    TypedTextId msgIdType = TYPED_TEXT_INVALID; ///< Text style; INVALID = use Designer default
    const touchgfx::Unicode::UnicodeChar *msgText = nullptr; ///< Raw Unicode fallback
    const char                           *msgChar = nullptr; ///< UTF-8 C-string (lowest priority)

    // ---- Tip / hint text  (TIP / INLINE styles) -----------------------------
    TypedTextId tipId     = TYPED_TEXT_INVALID;
    TypedTextId tipIdType = TYPED_TEXT_INVALID; ///< Tip style; INVALID = use Designer default
    const touchgfx::Unicode::UnicodeChar *tipText = nullptr; ///< Raw Unicode fallback
    const char                           *tipChar = nullptr; ///< UTF-8 C-string (lowest priority)
    touchgfx::colortype tipColor = touchgfx::Color::getColorFromRGB(192, 192, 192);

    // ---- Icon  (ICON / ICON_EXT styles) -------------------------------------
    BitmapId       iconId  = BITMAP_INVALID; ///< Static bitmap  (ICON)
    const uint8_t *extIcon = nullptr;        ///< Raw pixels, 60x60 ABGR2222  (ICON_EXT)

    // ---- Toggle  (TOGGLE style) ---------------------------------------------
    bool toggleState = false;

    // ---- Per-item layout overrides  (optional) ------------------------------
    const CenterItemLayout *centerLayout = nullptr; ///< nullptr = use menu-level default
    const ItemLayout       *itemLayout   = nullptr; ///< nullptr = use menu-level default
};

#endif // MENUITEMCONFIG_HPP
