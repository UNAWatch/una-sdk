#include <gui/containers/PauseIndicator.hpp>

PauseIndicator::PauseIndicator()
{
}

void PauseIndicator::initialize()
{
    PauseIndicatorBase::initialize();
}

void PauseIndicator::setTime(std::time_t seconds)
{
    const uint32_t h = static_cast<uint32_t>(seconds / 3600u);
    const uint32_t m = static_cast<uint32_t>((seconds % 3600u) / 60u);
    const uint32_t s = static_cast<uint32_t>(seconds % 60u);

    if (h > 0u) {
        touchgfx::Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
                                    "%u:%02u:%02u", h, m, s);
    } else {
        touchgfx::Unicode::snprintf(timerValueBuffer, TIMERVALUE_SIZE,
                                    "%02u:%02u", m, s);
    }

    timerValue.invalidate();
}
