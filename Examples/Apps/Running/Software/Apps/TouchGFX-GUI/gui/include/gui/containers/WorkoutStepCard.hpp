#ifndef WORKOUT_STEP_CARD_HPP
#define WORKOUT_STEP_CARD_HPP

#include <touchgfx/containers/Container.hpp>

#include <gui/containers/Title.hpp>
#include <gui/containers/WorkoutLabel.hpp>
#include <gui/model/Model.hpp>

/**
 * One workout step in full: its duration large, in the colour of its kind,
 * then its target, its kind or repeat, and its notes over two lines. Used by
 * the next-step card when a step starts and by the next-step face.
 */
class WorkoutStepCard : public touchgfx::Container
{
public:
    WorkoutStepCard();

    /// The step that has just started: titled with its kind, with its repeat.
    void showCurrent(const Model::WorkoutStepInfo& step);

    /// The step after the current one: titled NEXT, with its kind.
    void showNext(const Model::WorkoutStepInfo& step);

private:
    void show(const Model::WorkoutStepInfo& step, bool next);

    Title            mTitle;
    WorkoutLabel<16> mValue;
    WorkoutLabel<8>  mUnit;
    WorkoutLabel<32> mTarget;
    WorkoutLabel<24> mSub;
    WorkoutLabel<SDK::Workout::kStepNotesBytes> mNotes1;
    WorkoutLabel<SDK::Workout::kStepNotesBytes> mNotes2;
};

#endif // WORKOUT_STEP_CARD_HPP
