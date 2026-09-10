#ifndef GOALARCSTEPS_HPP
#define GOALARCSTEPS_HPP

#include <gui_generated/containers/GoalArcStepsBase.hpp>

/**
 * @brief The left-hand goal ring: the day's steps against their target.
 *
 * A dark track with a teal arc over it, both on the rim circle the design
 * draws them on. The fill grows from the bottom of the track towards twelve
 * o'clock, which is the direction the design shows and the reason this and
 * @c GoalArcActivity are two containers rather than one used twice: they are
 * mirror images, and a mirrored fill is not a mirrored angle range.
 *
 * Nothing about the geometry is written down here. The centre, the radius, the
 * stroke, the two angles and both colours all come from the generated base, so
 * retuning the ring is an edit in the Designer with no matching edit in code.
 */
class GoalArcSteps : public GoalArcStepsBase
{
public:
    GoalArcSteps();
    virtual ~GoalArcSteps() {}

    virtual void initialize();

    /**
     * @brief Fill the arc to @p current out of @p total.
     *
     * A total of zero draws nothing: there is no goal to be a fraction of.
     * Anything past the goal is held at full, so an exceeded target reads as
     * met rather than wrapping round.
     */
    void setProgress(uint32_t current, uint32_t total);

protected:
};

#endif // GOALARCSTEPS_HPP
