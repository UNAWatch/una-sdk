#ifndef TITLE_HPP
#define TITLE_HPP

#include <gui_generated/containers/TitleBase.hpp>

/**
 * @brief Screen title titleText container.
 *
 * Wraps a TextAreaWithOneWildcard to display a localised or raw Unicode
 * string as the screen heading.
 *
 * Usage:
 * @code
 *   title.set(T_SCREEN_HEADING);        // from typed-titleText ID
 *   title.set(myUnicodeBuffer);         // from raw buffer
 *   title.setVisible(false);            // hide when not needed
 * @endcode
 */
class Title : public TitleBase
{
public:
    Title();
    virtual ~Title() {}

    virtual void initialize();

    /** @brief Set title from a localised typed-titleText ID. */
    void set(TypedTextId msgId);

    /** @brief Set title from a raw Unicode string. Ignored if @p messageText is null. */
    void set(touchgfx::Unicode::UnicodeChar* messageText);

    /** @brief Set title from a UTF-8 / ASCII C-string. Ignored if @p messageText is null. */
    void set(const char* messageText);

    /** @brief Change the accent color (line + titleText). */
    void setColor(touchgfx::colortype color);
};

#endif // TITLE_HPP
