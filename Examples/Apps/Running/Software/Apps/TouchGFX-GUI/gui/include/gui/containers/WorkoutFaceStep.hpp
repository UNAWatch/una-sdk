#ifndef WORKOUT_FACE_STEP_HPP
#define WORKOUT_FACE_STEP_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>

#include <gui/containers/Title.hpp>
#include <gui/containers/WorkoutLabel.hpp>
#include <gui/model/Model.hpp>

/**
 * The second workout face: the current step's target, its average so far
 * and, in a repeat, which pass this is. Each step is a lap, so the average
 * is the lap's: the same figure the Lap face shows and the FIT lap records.
 */
class WorkoutFaceStep : public touchgfx::Container
{
public:
    WorkoutFaceStep();

    /// The step from WORKOUT_STEP; its repeat from the latest WORKOUT_DATA.
    void setStep(const Model::WorkoutStepInfo& step);
    void setRepeat(uint32_t rep, uint32_t reps);

    /// The lap averages: pace in s/km or s/mi, heart rate in bpm. The pace is
    /// shown unless the step's target is a heart rate.
    void setAverages(float lapPace, float lapHr, bool heartRate);

private:
    Title            mTitle;
    WorkoutLabel<24> mTargetCaption;
    WorkoutLabel<32> mTarget;
    touchgfx::Box    mDivider;
    touchgfx::Box    mColumns;
    WorkoutLabel<16> mAvgCaption;
    WorkoutLabel<10> mAvg;
    WorkoutLabel<8>  mRepCaption;
    WorkoutLabel<12> mRep;
    bool             mHasRepeat = false;
};

#endif // WORKOUT_FACE_STEP_HPP
