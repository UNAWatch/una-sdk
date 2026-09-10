#ifndef GOALARCACTIVITY_HPP
#define GOALARCACTIVITY_HPP

#include <gui_generated/containers/GoalArcActivityBase.hpp>

/**
 * @brief The right-hand goal ring: the day's active minutes against target.
 *
 * The mirror of @c GoalArcSteps in amber. On this side of the face the track's
 * clockwise start is its visual top, so the fill grows from the track's end
 * rather than its start -- the same split the kernel's own face makes between
 * ActivityBarTop and ActivityBarBottom, and the reason the two rings cannot be
 * one container used twice.
 *
 * All geometry and both colours come from the generated base.
 */
class GoalArcActivity : public GoalArcActivityBase
{
public:
    GoalArcActivity();
    virtual ~GoalArcActivity() {}

    virtual void initialize();

    /**
     * @brief Fill the arc to @p current out of @p total.
     *
     * A total of zero draws nothing, and anything past the goal is held at
     * full, exactly as on the steps ring.
     */
    void setProgress(uint32_t current, uint32_t total);

protected:
};

#endif // GOALARCACTIVITY_HPP
