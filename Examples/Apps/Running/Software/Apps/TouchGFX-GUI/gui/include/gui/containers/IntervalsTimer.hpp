#ifndef INTERVALSTIMER_HPP
#define INTERVALSTIMER_HPP

#include <gui_generated/containers/IntervalsTimerBase.hpp>
#include <ctime>

class IntervalsTimer : public IntervalsTimerBase
{
public:
    IntervalsTimer();
    virtual ~IntervalsTimer() {}

    virtual void initialize();

    /**
     * @brief Update timer for a time-based metric -- shows MM:SS + description label.
     *
     * Time is clamped to 59:59. If sec > 3599, display freezes at "59:59"
     * (non-nominal; restrict maximum interval duration in settings).
     *
     * @param sec    Seconds (remaining / elapsed / open depending on metric).
     * @param metric TIME_OPEN -> "Open", TIME_REMAINING -> "Remaining", TIME_ELAPSED -> "Elapsed".
     *               Passing DISTANCE here is undefined -- use setPhaseDistance() instead.
     */
    void setPhaseTime(std::time_t sec, Track::IntervalsMetric metric);

    /**
     * @brief Update timer for a distance-based metric -- shows distance + "km/mi remaining".
     *
     * @param distDisplay Already-converted display value in km or mi (presenter converts).
     * @param isImperial  Selects unit label: "mi" if true, "km" if false.
     */
    void setPhaseDistance(float distDisplay, bool isImperial);

    /**
     * @brief Update timer for alert screens -- MM:SS only, no description, no line.
     *
     * Time is clamped to 59:59. Description widget is hidden.
     */
    void setRemainingTime(std::time_t sec);

    /** @brief Change the accent color (line + timer text + description). */
    void setColor(touchgfx::colortype color);

    /** @brief Show or hide the separator line. */
    void setLineVisible(bool visible);

    /** @brief Show or hide the description label. */
    void setDescriptionVisible(bool visible);

private:
    /** @brief Clamp @p sec to [0, 3599] and display as MM:SS. */
    void setTimerClamped(std::time_t sec);

    void setTimerMS(uint8_t mm, uint8_t ss);
    void setTimerDist(float value);
    void setDescriptionId(TypedTextId id);
    void setDescriptionText(const touchgfx::Unicode::UnicodeChar* text);
};

#endif // INTERVALSTIMER_HPP
