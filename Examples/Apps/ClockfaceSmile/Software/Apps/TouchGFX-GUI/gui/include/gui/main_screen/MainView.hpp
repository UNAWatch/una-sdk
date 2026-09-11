#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @class MainView
 * @brief The Smile face: a teal field over three metric columns.
 *
 * The teal field and its curved lower edge are one bitmap, so the face costs
 * one full-screen blit plus the type over it and has no canvas widgets at all.
 *
 * The three columns are each centred in a fixed box, which is what the design
 * does, so nothing about them has to be laid out at runtime. The clock is the
 * one group that does -- see layoutClock().
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /**
     * @brief Set the clock and the date line, if the reading has changed.
     *
     * Called when the service reports a new minute, and when the face is built
     * or resumed. Nothing here is sampled per frame.
     */
    void setTime(const WallTime &time);

    /** @brief Charge level pushed up from the service, 0-100. */
    void setBatteryLevel(uint8_t level);

    /** @brief The day's three readings, as the service last reported them. */
    void setHealth(const DailyHealth &health);

    /**
     * @brief Adopt the clock format and the date order the watch is set to.
     *
     * Redraws the clock and the date line, because both carry the difference:
     * the clock gains a colon and loses its leading zero, the date line gains
     * the meridiem, and the day and month swap places.
     */
    void setClockStyle(const ClockStyle &style);

private:
    /**
     * @brief Centre the hour, separator and minute as one group.
     *
     * The design centres the whole clock rather than pinning the digits either
     * side of a fixed midpoint, so the group moves as its parts change width:
     * between the formats, and within the 12-hour form between a one- and a
     * two-digit hour. Poppins here is the repo's tabular-figures build, so the
     * measurements do not shift as the digits do.
     *
     * The separator is a fixed width per format rather than the colon's own
     * advance, because the design leaves more room around the colon than the
     * glyph takes, and a plain gap where the colon is not drawn.
     */
    void layoutClock();

    /** Move a text area, repainting what it leaves as well as where it lands. */
    static void place(touchgfx::TextArea &area, int16_t x, int16_t width);

    /** Rewrite the clock's two number buffers from the reading on screen. */
    void updateClockText();

    /** Rewrite the date line from the reading on screen. */
    void updateDateText();

    /// Vertical placement of the clock band, from the design. The horizontal
    /// placement is computed; this is not.
    static const int16_t kClockY = 37;
    static const int16_t kClockHeight = 77;

    /// The room the design leaves between hour and minute: the colon sits in
    /// it in the 12-hour form, and it is left empty in the 24-hour one.
    static const int16_t kSeparator12 = 24;
    static const int16_t kSeparator24 = 5;

    WallTime   mShown;  ///< Reading currently on the display
    ClockStyle mStyle;  ///< Settings the clock and date are drawn in
};

#endif // MAINVIEW_HPP
