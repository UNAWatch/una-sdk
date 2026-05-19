#include <gui/containers/IntervalsTimer.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

static const std::time_t kMaxIntervalSec = 3599; // 59:59

IntervalsTimer::IntervalsTimer()
{
}

void IntervalsTimer::initialize()
{
    IntervalsTimerBase::initialize();
}

// =============================================================================
// Public API
// =============================================================================

void IntervalsTimer::setPhaseTime(std::time_t sec, Track::IntervalsMetric metric)
{
    setTimerClamped(sec);

    switch (metric) {
    case Track::IntervalsMetric::TIME_OPEN:
        setDescriptionId(T_TEXT_OPEN);
        break;
    case Track::IntervalsMetric::TIME_REMAINING:
        setDescriptionId(T_TEXT_REMAINING);
        break;
    case Track::IntervalsMetric::TIME_ELAPSED:
        setDescriptionId(T_TEXT_ELAPSED);
        break;
    default:
        break;
    }
}

void IntervalsTimer::setPhaseDistance(float distDisplay, bool isImperial)
{
    setTimerDist(distDisplay);

    touchgfx::Unicode::UnicodeChar buf[24];
    Unicode::snprintf(buf, 24, "%s %s",
        touchgfx::TypedText(isImperial ? T_TEXT_MI : T_TEXT_KM).getText(),
        touchgfx::TypedText(T_TEXT_REMAINING_LC).getText());
    setDescriptionText(buf);
}

void IntervalsTimer::setRemainingTime(std::time_t sec)
{
    setTimerClamped(sec);
    setDescriptionId(TYPED_TEXT_INVALID); // hide description
}

void IntervalsTimer::setColor(touchgfx::colortype color)
{
    timerText.setColor(color);
    timerText.invalidate();

    descriptionText.setColor(color);
    descriptionText.invalidate();

    linePainter.setColor(color);
    line.invalidate();
}

void IntervalsTimer::setLineVisible(bool visible)
{
    line.setVisible(visible);
    line.invalidate();
}

void IntervalsTimer::setDescriptionVisible(bool visible)
{
    descriptionText.setVisible(visible);
    descriptionText.invalidate();
}

// =============================================================================
// Private helpers
// =============================================================================

void IntervalsTimer::setTimerClamped(std::time_t sec)
{
    if (sec > kMaxIntervalSec) {
        sec = kMaxIntervalSec;
    }
    setTimerMS(static_cast<uint8_t>(sec / 60),
               static_cast<uint8_t>(sec % 60));
}

void IntervalsTimer::setTimerMS(uint8_t mm, uint8_t ss)
{
    Unicode::snprintf(timerTextBuffer, TIMERTEXT_SIZE, "%02u:%02u", mm, ss);
    timerText.invalidate();
}

void IntervalsTimer::setTimerDist(float value)
{
    if (value < 0.0f) value = 0.0f;
    if (value < 100.0f) {
        Unicode::snprintfFloat(timerTextBuffer, TIMERTEXT_SIZE, "%05.02f", value);
    } else {
        Unicode::snprintfFloat(timerTextBuffer, TIMERTEXT_SIZE, "%05.01f", value);
    }
    timerText.invalidate();
}

void IntervalsTimer::setDescriptionId(TypedTextId id)
{
    if (id == TYPED_TEXT_INVALID) {
        descriptionTextBuffer[0] = 0;
        descriptionText.setVisible(false);
    } else {
        Unicode::snprintf(descriptionTextBuffer, DESCRIPTIONTEXT_SIZE, "%s",
            touchgfx::TypedText(id).getText());
        descriptionText.setVisible(true);
    }
    descriptionText.invalidate();
}

void IntervalsTimer::setDescriptionText(const touchgfx::Unicode::UnicodeChar* text)
{
    Unicode::strncpy(descriptionTextBuffer, text, DESCRIPTIONTEXT_SIZE);
    descriptionTextBuffer[DESCRIPTIONTEXT_SIZE - 1] = 0;
    descriptionText.setVisible(true);
    descriptionText.invalidate();
}
