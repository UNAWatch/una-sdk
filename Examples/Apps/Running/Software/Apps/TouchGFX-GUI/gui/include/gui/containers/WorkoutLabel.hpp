#ifndef WORKOUT_LABEL_HPP
#define WORKOUT_LABEL_HPP

#include <touchgfx/widgets/TextAreaWithWildcard.hpp>
#include <touchgfx/TypedText.hpp>
#include <touchgfx/Unicode.hpp>
#include <texts/TextKeysAndLanguages.hpp>

#include <gui/common/WorkoutUi.hpp>

/**
 * A line of runtime text that keeps itself centred on a point: its own
 * buffer, a left-aligned template typography, and a width cut to fit.
 */
template <uint16_t N>
class WorkoutLabel : public touchgfx::TextAreaWithOneWildcard
{
public:
    WorkoutLabel()
    {
        setWildcard(mBuf);
    }

    /// @p typography: a left-aligned template, e.g. T_TMP_SEMIBOLD_35_L.
    void init(touchgfx::TypedTextId typography, touchgfx::colortype color)
    {
        mTypography = typography;
        setTypedText(touchgfx::TypedText(typography));
        setColor(color);
        setLinespacing(0);
    }

    /// Change the typography, keeping the text; e.g. a smaller size when
    /// the text is too wide.
    void setTypography(touchgfx::TypedTextId typography)
    {
        mTypography = typography;
        setTypedText(touchgfx::TypedText(typography));
        place();
    }

    /// Width the display allows for this line, at its centre.
    int16_t availableWidth() const
    {
        const int16_t h = static_cast<int16_t>(touchgfx::TypedText(mTypography).getFont()->getHeight());
        return WorkoutUi::lineWidth(static_cast<int16_t>(mCy - h / 2), static_cast<int16_t>(mCy + h / 2));
    }

    void setAnchor(int16_t cx, int16_t cy)
    {
        mCx = cx;
        mCy = cy;
        place();
    }

    /// Text from the text database.
    void setText(touchgfx::TypedTextId id)
    {
        touchgfx::Unicode::snprintf(mBuf, N, "%s", touchgfx::TypedText(id).getText());
        place();
    }

    /// ASCII text, e.g. from snprintf.
    void setText(const char* ascii)
    {
        touchgfx::Unicode::strncpy(mBuf, ascii, N - 1);
        mBuf[N - 1] = 0;
        place();
    }

    /// Text from a workout file, mapped to what the fonts can draw.
    void setFileText(const char* utf8)
    {
        WorkoutUi::toText(utf8, mBuf, N);
        place();
    }

    /// ASCII text added to the end, cut at the buffer's size.
    void append(const char* ascii)
    {
        const uint16_t len = touchgfx::Unicode::strlen(mBuf);
        if (len + 1 < N) {
            touchgfx::Unicode::strncpy(mBuf + len, ascii, static_cast<uint16_t>(N - 1 - len));
            mBuf[N - 1] = 0;
        }
        place();
    }

    touchgfx::Unicode::UnicodeChar* buffer() { return mBuf; }

    /// Cut to @p maxWidth pixels (0: to the display's width at this line),
    /// then centre again.
    void fit(int16_t maxWidth = 0)
    {
        if (maxWidth == 0) {
            const int16_t h = static_cast<int16_t>(touchgfx::TypedText(mTypography).getFont()->getHeight());
            maxWidth = WorkoutUi::lineWidth(static_cast<int16_t>(mCy - h / 2),
                                            static_cast<int16_t>(mCy + h / 2));
        }
        WorkoutUi::fitWidth(mBuf, mTypography, maxWidth);
        place();
    }

    /// Lay out again after the text in buffer() changed.
    void place()
    {
        invalidate();
        resizeToCurrentText();
        setXY(static_cast<int16_t>(mCx - getWidth() / 2), static_cast<int16_t>(mCy - getHeight() / 2));
        invalidate();
    }

private:
    touchgfx::Unicode::UnicodeChar mBuf[N] {};
    touchgfx::TypedTextId mTypography = touchgfx::TYPED_TEXT_INVALID;
    int16_t     mCx = 120;
    int16_t     mCy = 120;
};

#endif // WORKOUT_LABEL_HPP
