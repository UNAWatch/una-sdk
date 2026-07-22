#ifndef MAINVIEW_HPP
#define MAINVIEW_HPP

#include <gui_generated/main_screen/MainViewBase.hpp>
#include <gui/main_screen/MainPresenter.hpp>

#include "Stopwatch.hpp"

/**
 * @class MainView
 * @brief The single screen: the clock, the controls and the lap list.
 *
 * The screen has three shapes rather than three screens. Which one is showing
 * is derived from the state instead of being tracked separately.
 */
class MainView : public MainViewBase
{
public:
    MainView();
    virtual ~MainView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void handleTickEvent() override;

    virtual void lapListUpdateItem(LapListItem &item, int16_t itemIndex) override;

    /**
     * @brief Take in a new snapshot from the service.
     */
    void onStopwatchChanged(const Stopwatch::State &state);

    /** @brief The screen layout that goes with one size of the time readout. */
    struct TimeFace
    {
        touchgfx::Rect main;
        touchgfx::Rect frac;
        int16_t        lineY;
        touchgfx::Rect list;
    };

protected:
    virtual void handleKeyEvent(uint8_t key) override;

private:
    /** @brief What the screen is showing, derived from the state. */
    enum class Mode
    {
        Idle,       ///< Cleared, nothing timed yet
        Running,    ///< Counting
        Paused,     ///< Stopped with time on the clock
    };

    Mode mode() const;

    void refreshTime(bool force);
    void refreshControls();

    /**
     * @brief Pick the large or the compact time face.
     *
     * The large face only fits "MM:SS"; past an hour the reading grows by two
     * characters and no longer does, so the compact face covers that as well
     * as the case where the lap list needs the room.
     */
    void applyTimeFace(bool compact);

    /** @brief Move every widget to one face, whether or not it is the current one. */
    void setFaceLayout(bool compact);
    void refreshLaps();
    void refreshScrollIndicator(bool animate);

    void scrollLaps(int16_t delta);

    /**
     * @brief True when the state calls for the compact face.
     *
     * A pure predicate over the stopwatch, not over what is currently drawn, so
     * the scroll maths and the face layout can be computed in any order and
     * still agree.
     */
    bool wantsCompact() const;

    /** @brief Rows the lap list shows in the face the state calls for. */
    int16_t visibleLaps() const;

    /** @brief True when the list can be scrolled and the screen offers it. */
    bool isScrollable() const;

    /** @brief Highest row index the list can be scrolled to. */
    int16_t maxScrollTop() const;

    int16_t  mScrollTop;        ///< Index of the row at the top of the list
    uint16_t mIndicatorCount;   ///< Item count the scroll indicator was last given
    bool     mCompactFace;      ///< True while the compact time face is applied

    /**
     * @brief The layout as the Designer left it, captured in setupScreen().
     *
     * Reading it back rather than repeating the numbers here keeps the default
     * face and the Designer from drifting apart.
     */
    TimeFace mLargeFace;

    /** @brief The lap list rect the Designer holds, used by the compact face. */
    touchgfx::Rect mCompactList;

    /**
     * @brief A pause was sent and its snapshot has not arrived yet.
     *
     * Until it does, the mirror still reads as running. Ticking the display on
     * would run it past the moment the service actually stopped at, and the
     * snapshot would then yank the digits backwards.
     */
    bool mPausePending;
};

#endif // MAINVIEW_HPP
