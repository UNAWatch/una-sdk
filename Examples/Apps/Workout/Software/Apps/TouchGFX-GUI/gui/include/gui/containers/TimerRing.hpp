#ifndef TIMERRING_HPP
#define TIMERRING_HPP

#include <gui_generated/containers/TimerRingBase.hpp>

/**
 * @brief Animated arc-based ring for countdown / count-up timers.
 *
 * Wraps two Circle widgets from the Designer -- `track` (background ring)
 * and `progress` (animated arc) -- plus their PainterABGR2222 painters.
 *
 * Arc direction (always clockwise):
 *   FILL  -- progress.End moves from track.Start toward track.End as value grows.
 *   DRAIN -- progress.Start moves from track.Start toward track.End as value shrinks;
 *            progress.End stays fixed at track.End.
 *
 * Animation is tick-driven at a configurable speed (units per second).
 * At 60 fps, speed = 60 means one full unit per tick.
 *
 * Designer requirements:
 *   Circle            "track"            -- background ring
 *   Circle            "progress"         -- animated progress arc
 *   PainterABGR2222   "trackPainter"     -- track colour
 *   PainterABGR2222   "progressPainter"  -- progress colour
 *
 * @code
 * ring.setMaxValue(60);           // 60-second countdown
 * ring.setSpeed(1.0f);            // 1 unit/s -> 60 s to complete
 * ring.setMode(TimerRing::DRAIN);
 * ring.setProgressColor(0xC08000);
 * ring.setValue(60);              // start full
 * ring.setCompleteCallback(onDoneCb);
 * ring.animateTo(0);              // begin countdown
 * @endcode
 */
class TimerRing : public TimerRingBase
{
public:
    /** Determines which arc endpoint moves as the value changes. */
    enum Mode
    {
        FILL,   ///< End moves track.Start -> track.End  (0 -> max: empty -> full)
        DRAIN,  ///< Start moves track.Start -> track.End  (max -> 0: full -> empty)
    };

    TimerRing();
    virtual ~TimerRing() {}

    virtual void initialize()      override;
    virtual void handleTickEvent() override;

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** Maximum value; maps to 100 % of the arc. Must be > 0. Default: 100. */
    void setMaxValue(int32_t max);

    /**
     * @brief Set the display refresh rate in Hz.
     *
     * Must be called before animateTo() to ensure correct speed calculation.
     * The system may run anywhere from 10 Hz upward, so this value must be
     * provided by the screen rather than hardcoded in the component.
     *
     * @param fps  Frames per second (e.g. 10.0f, 30.0f, 60.0f).
     */
    void setFPS(float fps);

    /**
     * @brief Animation speed in value-units per second.
     *
     * Examples (at any fps):
     *   speed = 1,  max = 60   -> 60-second countdown
     *   speed = 30, max = 100  -> ~3.3-second animation
     */
    void setSpeed(float unitsPerSecond);

    /** Set which arc endpoint moves (FILL or DRAIN). Default: FILL. */
    void setMode(Mode mode);

    /** Set the progress arc colour at runtime. */
    void setProgressColor(touchgfx::colortype color);

    /** Set the track (background ring) colour at runtime. */
    void setTrackColor(touchgfx::colortype color);

    /**
     * @brief Enable looping: when the target is reached the animation restarts
     *        automatically from the opposite end.
     *
     * The complete callback fires on every cycle. Default: disabled.
     */
    void setLoopEnabled(bool enabled);

    // -------------------------------------------------------------------------
    // Control
    // -------------------------------------------------------------------------

    /**
     * @brief Instantly position the arc at @p value with no animation.
     * Clamps to [0, maxValue]. Stops any running animation.
     */
    void setValue(int32_t value);

    /**
     * @brief Start animating toward @p target at the configured speed.
     *
     * Registers this widget as a timer. Stops automatically when the target
     * is reached. Calling animateTo() while already animating redirects to
     * the new target without resetting the fractional accumulator.
     */
    void animateTo(int32_t target);

    /**
     * @brief Freeze animation; target is preserved.
     * Call resume() to continue from the current position.
     */
    void pause();

    /** Resume a paused animation. No-op if not paused. */
    void resume();

    /** Stop animation and discard the current target. */
    void stop();

    // -------------------------------------------------------------------------
    // Getters
    // -------------------------------------------------------------------------

    int32_t getValue()    const { return mValue; }
    int32_t getTarget()   const { return mTarget; }
    float   getProgress() const; ///< Normalised [0.0 ... 1.0]
    bool    isAnimating() const { return mAnimating; }
    bool    isPaused()    const { return mPaused; }
    Mode    getMode()     const { return mMode; }

    // -------------------------------------------------------------------------
    // Callbacks
    // -------------------------------------------------------------------------

    /**
     * @brief Called on every tick where the integer value changes.
     * Argument: new value.
     */
    void setValueChangedCallback(touchgfx::GenericCallback<int32_t>& cb)
    {
        mpValueChangedCb = &cb;
    }

    /**
     * @brief Called when the animation reaches its target.
     * Argument: final value (equals the target that was set).
     * In loop mode this fires on every cycle.
     */
    void setCompleteCallback(touchgfx::GenericCallback<int32_t>& cb)
    {
        mpCompleteCb = &cb;
    }

private:
    int32_t mValue       = 0;
    int32_t mTarget      = 0;
    int32_t mMaxValue    = 100;
    float   mSpeed       = 10.0f;
    float   mFPS         = 10.0f;
    float   mAccumulator = 0.0f; ///< Sub-tick fractional remainder
    bool    mAnimating   = false;
    bool    mPaused      = false;
    bool    mLoop        = false;
    Mode    mMode        = FILL;

    touchgfx::GenericCallback<int32_t> *mpValueChangedCb = nullptr;
    touchgfx::GenericCallback<int32_t> *mpCompleteCb     = nullptr;

    /** Recompute progress arc angles from mValue and repaint. */
    void updateArc();
};

#endif // TIMERRING_HPP
