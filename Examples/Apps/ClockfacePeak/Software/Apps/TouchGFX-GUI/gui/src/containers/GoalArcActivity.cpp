#include <gui/containers/GoalArcActivity.hpp>

GoalArcActivity::GoalArcActivity()
{

}

void GoalArcActivity::initialize()
{
    GoalArcActivityBase::initialize();
}

void GoalArcActivity::setProgress(uint32_t current, uint32_t total)
{
    // As on the steps ring, the range comes off the track, which is never
    // mutated; reading it off the fill would shrink it on every call.
    float from = 0.0f;
    float to   = 0.0f;
    background.getArcStart(from);
    background.getArcEnd(to);

    float filled = 0.0f;
    if (total != 0u) {
        const uint32_t pct = (current >= total)
                           ? 100u
                           : static_cast<uint32_t>((static_cast<uint64_t>(current) * 100u) / total);
        filled = ((to - from) * static_cast<float>(pct)) / 100.0f;
    }

    // The one line that differs from the steps ring: on the right of the face
    // the track's clockwise end is its visual bottom, so the fill grows back
    // from there towards twelve o'clock.
    progress.setArc(to - filled, to);

    progress.invalidate();
    background.invalidate();
}
