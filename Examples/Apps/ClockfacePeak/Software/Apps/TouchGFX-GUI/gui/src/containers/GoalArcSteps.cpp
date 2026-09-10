#include <gui/containers/GoalArcSteps.hpp>

GoalArcSteps::GoalArcSteps()
{

}

void GoalArcSteps::initialize()
{
    GoalArcStepsBase::initialize();
}

void GoalArcSteps::setProgress(uint32_t current, uint32_t total)
{
    // The range is read off the track, which is never mutated. Reading it off
    // the fill instead would shrink it on every call, because the fill is
    // exactly what this function has just narrowed.
    float from = 0.0f;
    float to   = 0.0f;
    background.getArcStart(from);
    background.getArcEnd(to);

    // TouchGFX sweeps clockwise from twelve o'clock, so on the left of the
    // face the track's start is its visual bottom -- which is where the design
    // fills from.
    float filled = 0.0f;
    if (total != 0u) {
        const uint32_t pct = (current >= total)
                           ? 100u
                           : static_cast<uint32_t>((static_cast<uint64_t>(current) * 100u) / total);
        filled = ((to - from) * static_cast<float>(pct)) / 100.0f;
    }

    progress.setArc(from, from + filled);

    progress.invalidate();
    background.invalidate();
}
