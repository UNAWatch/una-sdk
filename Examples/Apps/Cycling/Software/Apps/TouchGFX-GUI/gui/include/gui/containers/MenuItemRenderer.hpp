#ifndef MENUITEMRENDERER_HPP
#define MENUITEMRENDERER_HPP

#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Bitmap.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>

/**
 * @brief Stateless rendering helpers shared between menu item types.
 *
 * Header-only -- no .cpp required.
 * All methods are static. Not intended to be instantiated.
 */
struct MenuItemRenderer
{
    /**
     * @brief Write content into a wildcard TextArea.
     *
     * Text source priority: textId (TypedText) > rawText > clear.
     *
     * Always writes to @p buffer -- clears it when neither source is provided.
     * This is important for scroll-wheel items, where the same container is
     * reused for different items and must not retain stale text.
     *
     * Font is updated if typeId is valid.
     *
     * @param area      Target TextArea widget.
     * @param buffer    Wildcard buffer belonging to the widget.
     * @param bufSize   Buffer capacity (in UnicodeChar units).
     * @param textId    Localized text ID; TYPED_TEXT_INVALID = fall through.
     * @param typeId    Text style (font, alignment); screens should always provide this
     *                  to prevent stale style from a previously rendered item.
     * @param rawText   Raw Unicode string; nullptr = fall through.
     * @param charText  UTF-8 / ASCII C-string (lowest priority); nullptr = fall through.
     */
    static void applyText(
        touchgfx::TextAreaWithOneWildcard &area,
        touchgfx::Unicode::UnicodeChar *buffer, uint16_t bufSize,
        TypedTextId textId,
        TypedTextId typeId,
        const touchgfx::Unicode::UnicodeChar *rawText,
        const char *charText = nullptr)
    {
        if (textId != TYPED_TEXT_INVALID) {
            touchgfx::Unicode::snprintf(buffer, bufSize, "%s",
                touchgfx::TypedText(textId).getText());
        } else if (rawText != nullptr) {
            touchgfx::Unicode::strncpy(buffer, rawText, bufSize);
        } else if (charText != nullptr) {
            touchgfx::Unicode::fromUTF8(
                reinterpret_cast<const uint8_t*>(charText), buffer, bufSize);
        } else {
            buffer[0] = 0;  // no content -- clear to prevent stale text from a previous item
        }

        if (typeId != TYPED_TEXT_INVALID) {
            area.setTypedText(touchgfx::TypedText(typeId));
        }
    }

    /**
     * @brief Write content + color into a tip TextArea.
     *
     * Same as applyText(), plus sets the text color.
     */
    static void applyTip(
        touchgfx::TextAreaWithOneWildcard &area,
        touchgfx::Unicode::UnicodeChar *buffer, uint16_t bufSize,
        TypedTextId textId,
        TypedTextId typeId,
        const touchgfx::Unicode::UnicodeChar *rawText,
        touchgfx::colortype color,
        const char *charText = nullptr)
    {
        applyText(area, buffer, bufSize, textId, typeId, rawText, charText);
        area.setColor(color);
    }

    /**
     * @brief Set a static bitmap on an Image widget.
     *
     * Does nothing if iconId == BITMAP_INVALID.
     */
    static void applyStaticIcon(touchgfx::Image &icon, BitmapId iconId)
    {
        if (iconId != BITMAP_INVALID) {
            icon.setBitmap(touchgfx::Bitmap(iconId));
        }
    }

    /**
     * @brief Create a dynamic bitmap from raw pixel data and set it on an Image.
     *
     * Deletes the previous dynamic bitmap (if any) before creating a new one.
     * Assumes ABGR2222 pixel format.
     *
     * @param icon        Target Image widget.
     * @param dynamicId   In/out: tracks the current dynamic bitmap ID for cleanup.
     * @param extIconData Raw pixel data pointer; nullptr = only deletes previous.
     * @param w           Bitmap width in pixels.
     * @param h           Bitmap height in pixels.
     */
    static void applyDynamicIcon(
        touchgfx::Image &icon,
        BitmapId &dynamicId,
        const uint8_t *extIconData,
        int16_t w, int16_t h)
    {
        if (dynamicId != BITMAP_INVALID) {
            touchgfx::Bitmap::dynamicBitmapDelete(dynamicId);
            dynamicId = BITMAP_INVALID;
        }

        if (extIconData != nullptr) {
            dynamicId = touchgfx::Bitmap::dynamicBitmapCreateExternal(
                w, h, extIconData, touchgfx::Bitmap::ABGR2222);
            icon.setBitmap(touchgfx::Bitmap(dynamicId));
        }
    }

    /**
     * @brief Resize a TextArea to its content height and center it vertically.
     *
     * Uses resizeHeightToCurrentText(), which measures the rendered height of
     * the TypedText string (including line wrapping).  Correct for multi-line
     * or variable-length text.
     *
     * @param area            Target TextArea.
     * @param containerHeight Height of the parent container in pixels.
     */
    static void centerTextY(touchgfx::TextArea &area, int16_t containerHeight)
    {
        area.resizeHeightToCurrentText();
        area.setY((containerHeight - area.getHeight()) / 2);
    }

    /**
     * @brief Align @p tipArea's baseline to the baseline of the already-positioned @p mainText.
     *
     * Used in INLINE style so that the smaller hint label sits on the same
     * optical line as the larger main text, regardless of font size difference.
     *
     * Algorithm:
     *   mainBaseline = mainText.getY() + mainFont->getBaseline()
     *   tipArea.Y    = mainBaseline    - tipFont->getBaseline()
     *
     * resizeHeightToCurrentText() is called on @p tipArea before positioning so
     * the widget bounds reflect the actual rendered height.
     *
     * Falls back to centering @p tipArea at the same mid-Y as @p mainText when
     * either font is unavailable.
     *
     * @param mainText  Main text area -- must already have Y and font set.
     * @param tipArea   Tip text area to position.
     */
    static void alignBaselineY(const touchgfx::TextArea &mainText,
                               touchgfx::TextArea       &tipArea)
    {
        const touchgfx::Font *mainFont = mainText.getTypedText().getFont();
        const touchgfx::Font *tipFont  = tipArea.getTypedText().getFont();

        tipArea.resizeHeightToCurrentText();

        if (mainFont != nullptr && tipFont != nullptr)
        {
            const int16_t baseline = mainText.getY()
                                   + static_cast<int16_t>(mainFont->getBaseline());
            tipArea.setY(baseline - static_cast<int16_t>(tipFont->getBaseline()));
        }
        else
        {
            // Fallback: align vertical mid-points
            const int16_t mainMid = mainText.getY() + mainText.getHeight() / 2;
            tipArea.setY(mainMid - tipArea.getHeight() / 2);
        }
    }
};

#endif // MENUITEMRENDERER_HPP
