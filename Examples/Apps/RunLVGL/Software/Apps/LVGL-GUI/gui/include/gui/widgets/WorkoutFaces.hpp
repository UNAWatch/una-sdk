/**
 ******************************************************************************
 * @file    WorkoutFaces.hpp
 * @brief   The workout faces of the Track screen and the step card.
 *
 * Ports of the Run app's WorkoutFaceGauge, WorkoutFaceStep and
 * WorkoutStepCard containers. Each builds on a 240 x 240 container of its own
 * that the Track screen shows or hides as a face.
 ******************************************************************************
 */

#ifndef WORKOUT_FACES_HPP
#define WORKOUT_FACES_HPP

#include <cstdint>
#include <memory>

#include "lvgl.h"

#include "gui/model/Model.hpp"
#include "gui/widgets/Widgets.hpp"
#include "gui/widgets/WorkoutLabel.hpp"

/**
 * The first workout face: the live value on a three-part arc (slow, in range,
 * fast) with a pointer, what is left of the step below it, or, for a step
 * with no target, what is left large with the live pace below.
 */
class WorkoutFaceGauge
{
public:
    explicit WorkoutFaceGauge(lv_obj_t* parent);

    lv_obj_t* root() const { return mRoot; }

    /// @p livePace: the live pace, already in s/km or s/mi.
    void set(const Model::WorkoutFace& face, bool imperial, float livePace);

    /// The pointer: one triangle at points relative to its host's origin.
    struct Pointer {
        lv_point_precise_t p[3] = {};
        lv_color_t         color = {};
    };

private:
    void setArcVisible(bool visible);
    void placePointer(float arc);
    static void formatRemaining(const Model::WorkoutFace& face, bool imperial, char* buf, size_t cap,
                                const char*& caption);

    lv_obj_t*    mRoot    = nullptr;
    lv_obj_t*    mSlow    = nullptr;
    lv_obj_t*    mBand    = nullptr;
    lv_obj_t*    mFast    = nullptr;
    lv_obj_t*    mPointer = nullptr;
    Pointer      mPointerDsc;
    WorkoutLabel mValue;
    WorkoutLabel mUnit;
    WorkoutLabel mSecond;
    WorkoutLabel mCaption;
};

/**
 * The second workout face: the current step's target, its average so far
 * and, in a repeat, which pass this is. Each step is a lap, so the average
 * is the lap's: the same figure the Lap face shows and the FIT lap records.
 */
class WorkoutFaceStep
{
public:
    explicit WorkoutFaceStep(lv_obj_t* parent);

    lv_obj_t* root() const { return mRoot; }

    /// The step from WORKOUT_STEP; its repeat from the latest WORKOUT_DATA.
    void setStep(const Model::WorkoutStepInfo& step);
    void setRepeat(uint32_t rep, uint32_t reps);

    /// The lap averages: pace in s/km or s/mi, heart rate in bpm. The pace is
    /// shown unless the step's target is a heart rate.
    void setAverages(float lapPace, float lapHr, bool heartRate);

private:
    lv_obj_t*    mRoot    = nullptr;
    lv_obj_t*    mColumns = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
    WorkoutLabel mTargetCaption;
    WorkoutLabel mTarget;
    WorkoutLabel mAvgCaption;
    WorkoutLabel mAvg;
    WorkoutLabel mRepCaption;
    WorkoutLabel mRep;
};

/**
 * One workout step in full: its duration large, in the colour of its kind,
 * then its target, its kind or repeat, and its notes over two lines. Used by
 * the next-step card when a step starts and by the next-step face.
 */
class WorkoutStepCard
{
public:
    explicit WorkoutStepCard(lv_obj_t* parent);

    lv_obj_t* root() const { return mRoot; }

    /// The step that has just started: titled with its kind, with its repeat.
    void showCurrent(const Model::WorkoutStepInfo& step);

    /// The step after the current one: titled NEXT, with its kind.
    void showNext(const Model::WorkoutStepInfo& step);

private:
    void show(const Model::WorkoutStepInfo& step, bool next);

    lv_obj_t*    mRoot = nullptr;
    std::unique_ptr<Widgets::Title> mTitle;
    WorkoutLabel mValue;
    WorkoutLabel mUnit;
    WorkoutLabel mTarget;
    WorkoutLabel mSub;
    WorkoutLabel mNotes1;
    WorkoutLabel mNotes2;
};

#endif // WORKOUT_FACES_HPP
