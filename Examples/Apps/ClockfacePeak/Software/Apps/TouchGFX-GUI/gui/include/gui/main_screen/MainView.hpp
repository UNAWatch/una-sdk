#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @class MainView
 * @brief The Peak face: two goal rings round a stacked clock and three rows.
 *
 * The rings are the only canvas widgets on the face; everything else is type,
 * two one-pixel rules and three icons.
 *
 * Two things this class computes rather than reads from the Designer, and for
 * the same reason in both cases -- the design centres a group whose parts
 * change width:
 *
 *  - the clock, whose hour loses a digit in the 12-hour form (layoutClock);
 *  - the date line, which is two colours and so two widgets (layoutDate).
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

    /** @brief The daily targets the two rings are fractions of. */
    void setGoals(const DailyGoals &goals);

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
     * side of a fixed midpoint, so the group moves as its parts change width.
     * Measuring the parts and centring them reproduces every case from one
     * rule, and Poppins here is the repo's tabular-figures build, so the
     * measurements do not shift as the digits do.
     *
     * The separator is a fixed width per format rather than the colon's own
     * advance, because the design leaves more room around the colon than the
     * glyph itself takes, and a plain gap where the colon is not drawn.
     */
    void layoutClock();

    /**
     * @brief Centre the day name and the rest of the date as one group.
     *
     * The line is two colours -- an amber day name, the rest in silver -- and
     * one TextArea has one colour, so it is two widgets laid out end to end
     * and centred together.
     */
    void layoutDate();

    /** Move a text area, repainting what it leaves as well as where it lands. */
    static void place(touchgfx::TextArea &area, int16_t x, int16_t y,
                      int16_t width, int16_t height);

    /** Rewrite the clock's two number buffers from the reading on screen. */
    void updateClockText();

    /** Rewrite the date line's two buffers from the reading on screen. */
    void updateDateText();

    /// Vertical placement, from the design. The horizontal placement of the
    /// clock and the date is computed; these are not.
    static const int16_t kClockY = 59;
    static const int16_t kClockHeight = 77;
    static const int16_t kDateY = 51;
    static const int16_t kDateHeight = 24;

    /// The room the design leaves between hour and minute: the colon sits in
    /// it in the 12-hour form, and it is left empty in the 24-hour one.
    static const int16_t kSeparator12 = 24;
    static const int16_t kSeparator24 = 5;

    WallTime   mShown;  ///< Reading currently on the display
    ClockStyle mStyle;  ///< Settings the clock and date are drawn in
};

#endif // MAINVIEW_HPP
