/**
 ******************************************************************************
 * @file    WorkoutLabel.hpp
 * @brief   A line of runtime text that keeps itself centred on a point.
 *
 * The Run app's WorkoutLabel: an LVGL label sized to its text and aligned on
 * its centre, so it stays centred as the text changes, with the cut-to-fit
 * and file-text helpers the workout screens use.
 ******************************************************************************
 */

#ifndef WORKOUT_LABEL_HPP
#define WORKOUT_LABEL_HPP

#include <cstdint>
#include <cstring>

#include "lvgl.h"

#include "gui/WorkoutUi.hpp"
#include "gui/theme/Theme.hpp"

class WorkoutLabel
{
public:
    /// Longest text a label holds, in bytes.
    static constexpr size_t kMaxBytes = 160;

    WorkoutLabel() = default;

    /// Create the label on @p parent, a container at the screen's origin.
    void init(lv_obj_t* parent, Theme::Font font, uint32_t color, int32_t cx = 120, int32_t cy = 120)
    {
        mFont  = Theme::font(font);
        mLabel = Theme::label(parent, font, "", 0, 0, LV_SIZE_CONTENT, LV_TEXT_ALIGN_LEFT, color);
        setAnchor(cx, cy);
    }

    lv_obj_t* obj() const { return mLabel; }
    const lv_font_t* font() const { return mFont; }

    /// Change the face, keeping the text; e.g. a smaller size when the text
    /// is too wide.
    void setFont(Theme::Font font)
    {
        mFont = Theme::font(font);
        lv_obj_set_style_text_font(mLabel, mFont, LV_PART_MAIN);
    }

    void setColor(uint32_t color)
    {
        lv_obj_set_style_text_color(mLabel, Theme::rgb(color), LV_PART_MAIN);
    }

    void setVisible(bool visible) { Theme::setHidden(mLabel, !visible); }

    void setAnchor(int32_t cx, int32_t cy)
    {
        mCy = cy;
        lv_obj_align(mLabel, LV_ALIGN_CENTER, cx - SDK::LVGL::kCx, cy - SDK::LVGL::kCy);
    }

    /// ASCII text, e.g. from snprintf.
    void setText(const char* text) { lv_label_set_text(mLabel, text ? text : ""); }

    /// Text from a workout file, mapped to what the fonts can draw.
    void setFileText(const char* utf8)
    {
        char buf[kMaxBytes];
        WorkoutUi::toText(utf8, buf, sizeof(buf));
        setText(buf);
    }

    const char* text() const { return lv_label_get_text(mLabel); }

    /// Width of the text, in pixels.
    int32_t width() const { return WorkoutUi::textWidth(text(), mFont); }

    /// Width the display allows for this line, at its centre.
    int32_t availableWidth() const
    {
        const int32_t h = lv_font_get_line_height(mFont);
        return WorkoutUi::lineWidth(mCy - h / 2, mCy + h / 2);
    }

    /// Cut to @p maxWidth pixels (0: to the display's width at this line).
    void fit(int32_t maxWidth = 0)
    {
        char buf[kMaxBytes];
        std::strncpy(buf, text(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        WorkoutUi::fitWidth(buf, sizeof(buf), mFont, maxWidth > 0 ? maxWidth : availableWidth());
        setText(buf);
    }

private:
    lv_obj_t*        mLabel = nullptr;
    const lv_font_t* mFont  = nullptr;
    int32_t          mCy    = 120;
};

#endif // WORKOUT_LABEL_HPP
