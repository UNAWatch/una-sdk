#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

/**
 * @class MainView
 * @brief The Retro face: a dot-matrix clock over the day's three readings.
 *
 * Everything is type. There is no artwork behind the digits and no canvas
 * widget on the screen -- the two rules are one-pixel boxes -- so the face
 * costs a handful of blits and whatever the glyphs come to.
 *
 * The one thing this class computes rather than reads from the Designer is
 * where the three clock widgets sit: see layoutClock().
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
     * or resumed. The frame the kernel grants is not involved: nothing here is
     * sampled per frame.
     */
    void setTime(const WallTime &time);

    /** @brief Charge level pushed up from the service, 0-100. */
    void setBatteryLevel(uint8_t level);

    /** @brief The day's three readings, as the service last reported them. */
    void setHealth(const DailyHealth &health);

    /**
     * @brief Switch the clock between the 24- and 12-hour forms.
     *
     * Redraws the clock and the date line, because both carry the difference:
     * the clock gains a colon and loses its leading zero, and the date line
     * gains the meridiem.
     */
    void setClockFormat(bool is12h);

private:
    /**
     * @brief Centre the hour, separator and minute as one group on the face.
     *
     * The design centres the whole clock rather than pinning the digits either
     * side of a fixed midpoint, so the group moves as its parts change width:
     * between the two formats, and within the 12-hour form between a one- and
     * a two-digit hour. Measuring the parts and centring them reproduces every
     * one of those cases from one rule, and both fonts are monospaced, so the
     * measurements are exact rather than approximate.
     *
     * In the 24-hour form there is no colon; the separator becomes the plain
     * gap the design leaves in its place.
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
    static const int16_t kClockY = 58;
    static const int16_t kClockHeight = 76;

    /// What the design leaves between hour and minute in the 24-hour form,
    /// where there is no colon to separate them.
    static const int16_t kGap24 = 11;

    WallTime mShown;    ///< Reading currently on the display
    bool     mIs12h;    ///< Format the clock is currently drawn in
};

#endif // MAINVIEW_HPP
