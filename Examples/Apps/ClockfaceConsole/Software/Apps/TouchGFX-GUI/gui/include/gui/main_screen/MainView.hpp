#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @class MainView
 * @brief The Console face: an oversized day of month over a mono clock.
 *
 * The leanest of the four faces -- all type, one rule and one icon, no canvas
 * widgets and no bitmap background.
 *
 * The three stacked date parts are each centred across the full width, so none
 * of them needs laying out. The clock does: see layoutClock().
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /**
     * @brief Set the clock and the three date parts, if the reading changed.
     *
     * Called when the service reports a new minute, and when the face is built
     * or resumed. Nothing here is sampled per frame.
     */
    void setTime(const WallTime &time);

    /** @brief The day's step count, as the service last reported it. */
    void setSteps(uint32_t steps);

    /**
     * @brief Switch the clock between the 24- and 12-hour forms.
     *
     * The clock gains a colon and loses its leading zero, and the meridiem
     * label beside it comes and goes with the format.
     */
    void setClockFormat(bool is12h);

private:
    /**
     * @brief Centre the hour, separator, minute and meridiem as one group.
     *
     * The design centres the whole clock, so the group moves as its parts
     * change width: between the formats, within the 12-hour form between a
     * one- and a two-digit hour, and again because the 12-hour form carries a
     * meridiem the 24-hour one does not. Measuring the parts and centring them
     * reproduces every case from one rule, and IBM Plex Mono is monospaced, so
     * the measurements are exact.
     *
     * In the 24-hour form there is no colon; the separator becomes the plain
     * gap the design leaves in its place.
     */
    void layoutClock();

    /** Move a text area, repainting what it leaves as well as where it lands. */
    static void place(touchgfx::TextArea &area, int16_t x, int16_t y,
                      int16_t width, int16_t height);

    /** Rewrite the clock's two number buffers from the reading on screen. */
    void updateClockText();

    /** Rewrite the three date parts from the reading on screen. */
    void updateDateText();

    /// Vertical placement, from the design. The horizontal placement of the
    /// clock group is computed; these are not.
    static const int16_t kClockY = 155;
    static const int16_t kClockHeight = 47;

    /// The meridiem sits on the clock's baseline rather than its box top, so
    /// it gets its own offset from the same band.
    static const int16_t kMeridiemDrop = 19;
    static const int16_t kMeridiemHeight = 22;

    /// The gap the design leaves between the minute and the meridiem.
    static const int16_t kMeridiemGap = 2;

    /// The room the design leaves between hour and minute: the colon takes its
    /// own cell in the 12-hour form, and a plain gap in the 24-hour one.
    static const int16_t kSeparator12 = 22;
    static const int16_t kSeparator24 = 6;

    WallTime mShown;    ///< Reading currently on the display
    bool     mIs12h;    ///< Format the clock is currently drawn in
};

#endif // MAINVIEW_HPP
