#include <gui/main_screen/MainView.hpp>
#include <gui/common/TimeFormat.hpp>

#include <texts/TextKeysAndLanguages.hpp>

#include "SDK/GUI/Button.hpp"

// Row height of the lap list, as set by the Designer.
static constexpr int16_t kRowHeight = 18;

static constexpr int16_t kScrollSteps = 6;

// The compact face has no Designer counterpart, so it is the only geometry
// spelled out here; the large one is read back from the screen at setup.
//
// The reading shrinks and rises to just under the title, ending left of the R1
// icons (x 190..210) rather than under them. The line then sits at y=99 and
// the lap list starts directly below it, filling the rest of the face.
// Widths come from the generated font tables: SemiBold 40 needs 152 px for the
// longest "H:MM:SS". The tenth is left-aligned right off the seconds (main's
// right edge is x=165), so its box only needs room for the one digit.
static const MainView::TimeFace kCompactFace = {
    {50, 44, 115, 51},    // main
    {166, 68, 20, 20},    // tenths
    99                    // line; the list keeps its Designer rect here
};

// The lap list is the one widget whose Designer rect belongs to the *compact*
// face: the generator sizes the drawable pool from the height it is given, so
// the project has to hold the tallest state or the extra rows have nothing to
// draw with. The three-row rect that goes with the large reading is therefore
// the one spelled out here.
static constexpr int16_t kLargeListHeight = 54;
static const touchgfx::Rect kLargeListRect = {58, 159, 125, kLargeListHeight};

// The compact face is needed the moment the laps outgrow what the large
// reading leaves room for, so the threshold follows the large list's row
// capacity instead of being a literal that could drift from it.
static constexpr uint8_t kCompactFromLaps = kLargeListHeight / kRowHeight + 1;

// Time-readout geometry for the hour form of each face. Past an hour the
// reading grows to HH:MM:SS and drops the tenths, so it needs more width
// than "MM:SS" leaves and a smaller face to find it. Widths are from the
// generated font tables: HH:MM:SS is 178 px at SemiBold 40 and 156 px at
// SemiBold 35.
//   - Large: SemiBold 60 -> 40, centred across the display.
//   - Compact: SemiBold 40 -> 35, right-aligned to stop short of the R1 icons
//     at x=190.
struct MainLayout
{
    uint16_t       textId;
    touchgfx::Rect rect;
};
static const MainLayout kLargeHoursMain   = { T_TMP_SEMIBOLD_40,   {20, 98, 200, 61} };
static const MainLayout kCompactHoursMain = { T_TMP_SEMIBOLD_35_R, {29, 44, 156, 51} };

MainView::MainView()
    : mScrollTop(0)
    , mIndicatorCount(0)
    , mCompactFace(false)
    , mHoursShown(false)
    , mPausePending(false)
{

}

void MainView::setupScreen()
{
    MainViewBase::setupScreen();

    // Take the default face from the widgets themselves, so moving them in the
    // Designer is enough and nothing has to be mirrored in code.
    mLargeFace.main  = timeMainText.getRect();
    mLargeFace.frac  = timeFracText.getRect();
    mLargeFace.lineY = line.getY();
    mLargeFace.list  = kLargeListRect;

    // The Designer rect is the extended one, so the compact face borrows it.
    mCompactList = lapList.getRect();

    // Apply the large, no-hours layout outright: the screen opens on it, and
    // the list still carries the taller Designer rect until told otherwise.
    setLayout(false, false);

    title.set(T_TEXT_STOPWATCH);

    // kSmall keeps the rail in the gap between the L1 and L2 button arcs.
    // kBig spans the whole left bezel and would draw straight over them.
    scrollIndicator.setConfig(ScrollIndicator::kSmall);

    lapList.setNumberOfItems(0);

    refreshTime(true);
    refreshControls();
    refreshScrollIndicator(false);
}

void MainView::tearDownScreen()
{
    MainViewBase::tearDownScreen();
}

MainView::Mode MainView::mode() const
{
    const Stopwatch::State &state = presenter->stopwatch();

    if (state.running) {
        return Mode::Running;
    }
    if (state.accumMs == 0 && state.lapCount == 0) {
        return Mode::Idle;
    }
    return Mode::Paused;
}

void MainView::handleTickEvent()
{
    MainViewBase::handleTickEvent();

    // Only a running clock changes on its own. Paused and idle screens are
    // redrawn by the snapshot that put them there.
    if (presenter->stopwatch().running && !mPausePending) {
        refreshTime(false);
    }
}

void MainView::refreshTime(bool force)
{
    const uint32_t ms = presenter->elapsedMs();

    applyLayout(wantsCompact(), TimeFormat::hasHours(ms));

    touchgfx::Unicode::UnicodeChar main[TIMEMAINTEXT_SIZE];
    touchgfx::Unicode::UnicodeChar frac[TIMEFRACTEXT_SIZE];

    TimeFormat::mainField(ms, main, TIMEMAINTEXT_SIZE);
    TimeFormat::fracField(ms, frac, TIMEFRACTEXT_SIZE);

    // Redraw a field only when its digits differ: at 10 Hz the big field is
    // unchanged nine ticks out of ten.
    if (force || touchgfx::Unicode::strncmp(main, timeMainTextBuffer, TIMEMAINTEXT_SIZE) != 0) {
        touchgfx::Unicode::strncpy(timeMainTextBuffer, main, TIMEMAINTEXT_SIZE);
        timeMainText.invalidate();
    }

    if (force || touchgfx::Unicode::strncmp(frac, timeFracTextBuffer, TIMEFRACTEXT_SIZE) != 0) {
        touchgfx::Unicode::strncpy(timeFracTextBuffer, frac, TIMEFRACTEXT_SIZE);
        timeFracText.invalidate();
    }
}

void MainView::applyLayout(bool compact, bool hours)
{
    if (compact == mCompactFace && hours == mHoursShown) {
        return;
    }
    setLayout(compact, hours);
}

void MainView::setLayout(bool compact, bool hours)
{
    mCompactFace = compact;
    mHoursShown  = hours;

    // The states occupy different rectangles. Invalidating only after the move
    // repaints the new area and leaves the old one holding stale pixels, so
    // the outgoing rectangles are invalidated first.
    timeMainText.invalidate();
    timeFracText.invalidate();
    line.invalidate();
    lapList.invalidate();

    const TimeFace &face = compact ? kCompactFace : mLargeFace;

    // Main reading: the hour form has its own smaller font and wider rect; the
    // normal form is the face's own (Designer-sourced large, or compact).
    if (hours) {
        const MainLayout &m = compact ? kCompactHoursMain : kLargeHoursMain;
        timeMainText.setTypedText(touchgfx::TypedText(m.textId));
        timeMainText.setPosition(m.rect.x, m.rect.y, m.rect.width, m.rect.height);
    } else {
        timeMainText.setTypedText(touchgfx::TypedText(
                compact ? T_TMP_SEMIBOLD_40_R : T_TMP_SEMIBOLD_60_R));
        timeMainText.setPosition(face.main.x, face.main.y, face.main.width, face.main.height);
    }

    // Hundredths are dropped once the reading shows hours -- there is no room
    // for them and sub-second precision past an hour is pointless.
    timeFracText.setVisible(!hours);
    if (!hours) {
        timeFracText.setTypedText(touchgfx::TypedText(
                compact ? T_TMP_MEDIUM_16_L : T_TMP_SEMIBOLD_20_L));
        timeFracText.setPosition(face.frac.x, face.frac.y, face.frac.width, face.frac.height);
    }

    // Line and list follow the face alone; the hour form does not move them.
    line.setY(face.lineY);
    const touchgfx::Rect &list = compact ? mCompactList : mLargeFace.list;
    lapList.setPosition(list.x, list.y, list.width, list.height);

    timeMainText.invalidate();
    timeFracText.invalidate();
    line.invalidate();
    lapList.invalidate();
}

void MainView::refreshControls()
{
    const Mode current = mode();
    const bool running = current == Mode::Running;
    const bool paused  = current == Mode::Paused;

    // Every icon is placed in the Designer; only its visibility changes here.
    actionPlayR1Icon.setVisible(!running);
    actionPauseR1Icon.setVisible(running);
    actionR2Icon.setVisible(running);   // lap
    actionL2Icon.setVisible(paused);    // reset

    actionPlayR1Icon.invalidate();
    actionPauseR1Icon.invalidate();
    actionR2Icon.invalidate();
    actionL2Icon.invalidate();

    // The line separates the reading from the lap list, so it only earns its
    // place once there is a list.
    line.setVisible(presenter->stopwatch().lapCount > 0);
    line.invalidate();

    // Each arc carries the colour of the icon sitting next to it: the pause
    // icon is amber and the play icon teal, reset is white.
    buttons.setR1(running ? Buttons::AMBER : Buttons::TEAL);
    buttons.setL2(paused ? Buttons::WHITE : Buttons::NONE);

    // R2 changes meaning with the clock: white to take a lap while it runs,
    // red to leave the app once it is stopped.
    buttons.setR2(running ? Buttons::WHITE : Buttons::RED);

    // L1 stays dark even though it scrolls: the scroll indicator occupies the
    // same stretch of bezel and is the navigation cue on its own.
    buttons.setL1(Buttons::NONE);
}

bool MainView::wantsCompact() const
{
    // The face is chosen by the laps alone: it shrinks to make room for the
    // list. The hour form is a separate modifier applied within either face.
    return presenter->stopwatch().lapCount >= kCompactFromLaps;
}

int16_t MainView::visibleLaps() const
{
    // Derived from the face the state calls for, not from the widget's live
    // height, so it is correct even before the face has been applied.
    const int16_t height = wantsCompact() ? mCompactList.height : kLargeListHeight;
    return height / kRowHeight;
}

bool MainView::isScrollable() const
{
    // Scrolling belongs to the running clock: once it stops, the list stays on
    // screen but the scroll affordances give way to the reset action.
    return mode() == Mode::Running &&
           presenter->stopwatch().lapCount > visibleLaps();
}

int16_t MainView::maxScrollTop() const
{
    const int16_t count   = presenter->stopwatch().lapCount;
    const int16_t visible = visibleLaps();
    return (count > visible) ? (count - visible) : 0;
}

void MainView::refreshScrollIndicator(bool animate)
{
    // The rail keeps the lap list company from the very first lap, so it does
    // not pop into existence halfway through a run. Scrolling itself belongs
    // to the running clock, which is why a pause takes the whole thing away.
    scrollIndicator.setVisible(presenter->stopwatch().lapCount > 0 &&
                               mode() == Mode::Running);

    // One item per reachable window position. While every lap is on screen
    // there is only one, and the widget draws the rail without a handle.
    const uint16_t count = isScrollable()
                         ? static_cast<uint16_t>(maxScrollTop() + 1)
                         : 1;

    if (count != mIndicatorCount) {
        mIndicatorCount = count;
        // setCount() snaps the handle back to the first item. A new lap lands
        // here, so the handle is put straight back to the end of the rail --
        // without animation, since it never visually left it.
        scrollIndicator.setCount(count);
        scrollIndicator.setActiveId(static_cast<uint16_t>(mScrollTop));
    } else if (animate) {
        scrollIndicator.animateToId(mScrollTop, kScrollSteps);
    } else {
        scrollIndicator.setActiveId(static_cast<uint16_t>(mScrollTop));
    }

    scrollIndicator.invalidate();
}

void MainView::refreshLaps()
{
    // The list itself holds the count from the last refresh, so it is the
    // authoritative "previous" -- no shadow of the lap count is needed.
    const int16_t previousLapCount = lapList.getNumberOfItems();
    const int16_t count            = presenter->stopwatch().lapCount;

    lapList.setNumberOfItems(count);

    if (count > previousLapCount) {
        // A lap was just taken: follow it so the newest row stays in view.
        // animateToItem() only scrolls when the target lies outside the
        // current window, so it is aimed at the newest row rather than at the
        // top of the window, which is often already visible.
        mScrollTop = maxScrollTop();
        lapList.animateToItem(static_cast<int16_t>(count - 1), kScrollSteps);
    } else if (count < previousLapCount) {
        // The list shrank, which means a reset.
        mScrollTop = 0;
        lapList.animateToItem(0, 0);
    }

    refreshScrollIndicator(count > previousLapCount);
    lapList.invalidate();
}

void MainView::scrollLaps(int16_t delta)
{
    if (!isScrollable()) {
        return;
    }

    const int16_t maxTop = maxScrollTop();

    int16_t top = mScrollTop + delta;
    if (top < 0) {
        top = 0;
    } else if (top > maxTop) {
        top = maxTop;
    }

    if (top == mScrollTop) {
        return;
    }

    const bool down = top > mScrollTop;
    mScrollTop = top;

    // Aim at the edge the window is moving towards: that row is by definition
    // outside the current window, which is what makes the list actually scroll.
    lapList.animateToItem(down ? static_cast<int16_t>(mScrollTop + visibleLaps() - 1)
                               : mScrollTop,
                          kScrollSteps);
    refreshScrollIndicator(true);
}

void MainView::lapListUpdateItem(LapListItem &item, int16_t itemIndex)
{
    const Stopwatch::State &state = presenter->stopwatch();

    if (itemIndex < 0 || itemIndex >= state.lapCount) {
        item.clear();
        return;
    }

    const uint8_t index = static_cast<uint8_t>(itemIndex);
    item.setLap(index + 1, Stopwatch::lapDuration(state, index));
}

void MainView::onStopwatchChanged(const Stopwatch::State &)
{
    mPausePending = false;

    // These three are independent now that visibleLaps() is derived from the
    // state rather than the live widget height. refreshTime() runs first only
    // so the list is already in its new place before refreshLaps() repopulates
    // it -- an ordering that saves a frame, not one correctness depends on.
    refreshTime(true);
    refreshLaps();
    refreshControls();
}

void MainView::handleKeyEvent(uint8_t key)
{
    namespace Btn = SDK::GUI::Button;

    switch (key) {
        case Btn::R1:
            if (presenter->stopwatch().running) {
                // Only hold the display if the command is actually on its way,
                // otherwise no snapshot would ever come to release it.
                mPausePending = presenter->pause();
            } else {
                presenter->start();
            }
            break;

        // R2 takes a lap while the clock runs; once it is stopped the same
        // button turns red and becomes the way out.
        case Btn::R2:
            if (mode() == Mode::Running) {
                presenter->lap();
            } else {
                presenter->exit();
            }
            break;

        case Btn::L1:
            scrollLaps(-1);
            break;

        case Btn::L2:
            // The design gives L2 to reset once the clock is stopped; while it
            // runs the button is free to scroll the list.
            if (mode() == Mode::Paused) {
                presenter->reset();
            } else {
                scrollLaps(1);
            }
            break;

        default:
            break;
    }
}

