/**
 ******************************************************************************
 * @file    Stopwatch.hpp
 * @date    17-07-2026
 * @author  Denys Saienko <denys.saienko@droid-technologies.com>
 * @brief   Stopwatch state and timekeeping logic shared by service and GUI.
 ******************************************************************************
 *
 * The whole state is a plain struct so it can travel in a single message and
 * be extrapolated by the GUI without further traffic: the kernel hands both
 * processes the same monotonic millisecond tick, so a start timestamp taken
 * by the service stays meaningful in the GUI.
 *
 ******************************************************************************
 */

#ifndef STOPWATCH_HPP
#define STOPWATCH_HPP

#include <cstdint>
#include <cstddef>

namespace Stopwatch
{

/**
 * @brief Maximum number of laps retained.
 *
 * Bounded so State fits the largest kernel message pool block (256 bytes).
 */
constexpr size_t kMaxLaps = 50;

/**
 * @brief Complete stopwatch state.
 *
 * Laps hold the elapsed time at each lap boundary, not the duration of the
 * lap. Durations are derived on display, which keeps the running total exact
 * instead of accumulating rounding across laps.
 */
struct State
{
    uint32_t accumMs;           ///< Elapsed time banked by previous runs
    uint32_t startMs;           ///< Tick at the last resume; only valid while running
    uint32_t laps[kMaxLaps];    ///< Elapsed time at each lap boundary
    uint8_t  lapCount;          ///< Number of valid entries in laps
    bool     running;           ///< True while the clock is advancing
};

/**
 * @brief Elapsed time of a state at a given tick.
 *
 * Unsigned arithmetic makes the subtraction correct across the 32-bit tick
 * wrap that happens roughly every 49.7 days.
 *
 * @param state State to evaluate.
 * @param nowMs Current monotonic tick.
 * @retval Elapsed milliseconds.
 */
inline uint32_t elapsed(const State &state, uint32_t nowMs)
{
    return state.running ? state.accumMs + (nowMs - state.startMs)
                         : state.accumMs;
}

/**
 * @brief Duration of a single lap.
 *
 * @param state State to evaluate.
 * @param index Lap index, zero based.
 * @retval Milliseconds between this lap boundary and the previous one, or 0
 *         when the index is out of range.
 */
inline uint32_t lapDuration(const State &state, uint8_t index)
{
    if (index >= state.lapCount) {
        return 0;
    }
    const uint32_t previous = (index == 0) ? 0 : state.laps[index - 1];
    return state.laps[index] - previous;
}

/**
 * @class Core
 * @brief Stopwatch transitions over a caller-supplied clock.
 *
 * The clock is passed in rather than read internally so the logic stays
 * testable on the host and so the caller decides which tick a transition is
 * stamped with.
 */
class Core
{
public:
    Core()
        : mState{}
    {}

    /**
     * @brief Start or resume the clock. No effect if already running.
     * @param nowMs Current monotonic tick.
     */
    void start(uint32_t nowMs)
    {
        if (mState.running) {
            return;
        }
        mState.startMs = nowMs;
        mState.running = true;
    }

    /**
     * @brief Pause the clock, banking the time run so far.
     * @param nowMs Current monotonic tick.
     */
    void pause(uint32_t nowMs)
    {
        if (!mState.running) {
            return;
        }
        mState.accumMs = elapsed(mState, nowMs);
        mState.running = false;
    }

    /**
     * @brief Clear the elapsed time and every lap.
     */
    void reset()
    {
        mState = State{};
    }

    /**
     * @brief Record a lap boundary at the current elapsed time.
     * @param nowMs Current monotonic tick.
     * @retval true  Lap recorded.
     * @retval false Clock is not running, or the lap store is full.
     */
    bool lap(uint32_t nowMs)
    {
        if (!mState.running || mState.lapCount >= kMaxLaps) {
            return false;
        }
        mState.laps[mState.lapCount++] = elapsed(mState, nowMs);
        return true;
    }

    bool isRunning() const { return mState.running; }

    const State &state() const { return mState; }

private:
    State mState;
};

} // namespace Stopwatch

#endif // STOPWATCH_HPP
