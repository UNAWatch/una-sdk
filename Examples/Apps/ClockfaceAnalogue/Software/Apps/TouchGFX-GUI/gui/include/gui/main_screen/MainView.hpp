#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include <touchgfx/widgets/canvas/Line.hpp>

/**
 * @class MainView
 * @brief The analogue face: two hands over a printed dial.
 *
 * Everything that does not move -- the tick ring -- is one image, so the only
 * widgets on screen are the ones whose appearance actually changes.
 *
 * The hands are drawn in the Designer, pointing at twelve o'clock, and this
 * class only turns them. Nothing about how a hand looks -- its length, its
 * thickness, where the bar stops and the stem carries on, the round ends, the
 * colour -- is written down here; all of it is read back from the widgets at
 * setup and rotated about the hub. Retuning the design is therefore an edit in
 * the Designer with no matching edit in code.
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /**
     * @brief Point the hands and set the date, if the reading has changed.
     *
     * Called when the service reports a new minute, and when the face is built
     * or resumed. The frame the kernel grants is not involved: nothing here is
     * sampled per frame.
     */
    void setTime(const WallTime &time);

    /** @brief Charge level pushed up from the service, 0-100. */
    void setBatteryLevel(uint8_t level);

    /** @brief Show the struck-through speaker only while alerts are silenced. */
    void setAlertsMuted(bool muted);

private:
    /** Strokes per hand: the narrow stem at the hub and the wide bar. */
    static const unsigned kStrokesPerHand = 2;

    /**
     * @brief One stroke of a hand, as the Designer drew it.
     *
     * The endpoints are kept relative to the hub, which is what makes them
     * rotatable; the widget itself keeps everything else.
     */
    struct Stroke
    {
        touchgfx::Line *line;
        float sx, sy;   ///< Start, offset from the hub
        float ex, ey;   ///< End, offset from the hub
    };

    /** Remember where the Designer put a stroke, relative to the hub. */
    void captureStroke(Stroke &stroke, touchgfx::Line &line);

    /** Point a hand, 0 degrees being where it was drawn and positive clockwise. */
    void setHandAngle(Stroke *strokes, float degrees);

    WallTime mShown;    ///< Reading currently on the display

    // The hands are canvas widgets, not bitmaps: each stroke is rasterised
    // from its endpoints at whatever angle it is given, so the outline stays
    // exact at every minute of the day. Rotating a bitmap instead resamples
    // artwork drawn at one angle into all the others, which on this panel
    // costs both the edge and the motion -- measured, a texture-mapped hand
    // wanders about 0.28 px off a straight line against 0.06 px here, and
    // under nearest-neighbour sampling it does not move at all for some of
    // the hour hand's half-degree steps and then jumps two of them at once.
    //
    // Rasterising them needs the screen's canvas buffer, sized in the
    // .touchgfx (CanvasBufferSize). Sweeping all 720 hand positions with
    // TouchGFX's own memory report on peaks at 1872 bytes and never splits a
    // draw, so the 3600 the screen declares covers the worst case. Re-measure
    // after retuning the strokes: a wider or longer one costs more outline.
    float  mPivotX;                     ///< Hub centre, as drawn
    float  mPivotY;
    Stroke mHour[kStrokesPerHand];
    Stroke mMinute[kStrokesPerHand];
};

#endif // MAINVIEW_HPP
