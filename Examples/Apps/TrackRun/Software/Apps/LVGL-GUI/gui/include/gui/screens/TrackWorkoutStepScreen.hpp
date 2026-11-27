/**
 ******************************************************************************
 * @file    TrackWorkoutStepScreen.hpp
 * @brief   The step card: the step that has just started, for 3 s, then back
 *          to the Track screen.
 *
 * Port of the Run app's TrackWorkoutStepView/Presenter. A step that starts
 * while the card shows replaces it and restarts the 3 s.
 ******************************************************************************
 */

#ifndef TRACK_WORKOUT_STEP_SCREEN_HPP
#define TRACK_WORKOUT_STEP_SCREEN_HPP

#include <memory>

#include "gui/screens/Screen.hpp"
#include "gui/widgets/WorkoutFaces.hpp"

class TrackWorkoutStepScreen : public Screen
{
public:
    explicit TrackWorkoutStepScreen(Model& model);
    ~TrackWorkoutStepScreen() override;

    void onShow() override;

    // ModelListener
    void onWorkoutStep(bool next) override;
    void onIntervalsWorkoutCompleted() override;

protected:
    void build() override;

private:
    static constexpr uint32_t kShowMs = 3000;

    void showStep();
    static void dismissCb(lv_timer_t* timer);

    std::unique_ptr<WorkoutStepCard> mCard;
    lv_timer_t* mTimer = nullptr;
};

#endif // TRACK_WORKOUT_STEP_SCREEN_HPP
