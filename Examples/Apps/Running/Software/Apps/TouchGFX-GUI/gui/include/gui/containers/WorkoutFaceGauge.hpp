#ifndef WORKOUT_FACE_GAUGE_HPP
#define WORKOUT_FACE_GAUGE_HPP

#include <touchgfx/containers/Container.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/canvas/Circle.hpp>
#include <touchgfx/widgets/canvas/PainterABGR2222.hpp>

#include <gui/containers/WorkoutLabel.hpp>
#include <gui/model/Model.hpp>

/**
 * The first workout face: how the runner is doing against the step's
 * target, and what is left of the step.
 *
 * With a target, an arc across the top shows the band as its middle third
 * and each outer third as the margin beyond it, faster or higher on the
 * right; a dot marks the gauge value, which is also shown large in the
 * zone's colour. Without one, the arc is hidden: what is left of the step is
 * the large number and the live pace goes below.
 */
class WorkoutFaceGauge : public touchgfx::Container
{
public:
    WorkoutFaceGauge();

    /// @p livePace: the live pace, already in s/km or s/mi.
    void set(const Model::WorkoutFace& face, bool imperial, float livePace);

private:
    void setArcVisible(bool visible);
    void placeDot(float arc);
    void formatRemaining(const Model::WorkoutFace& face, bool imperial, char* buf, size_t cap,
                         touchgfx::TypedTextId& caption);

    touchgfx::Circle          mSlow, mBand, mFast, mDot;
    touchgfx::PainterABGR2222 mSlowPainter, mBandPainter, mFastPainter, mDotPainter;
    touchgfx::Box             mDivider;

    WorkoutLabel<10> mValue;    ///< Gauge value, or what is left without a target
    WorkoutLabel<6>  mUnit;
    WorkoutLabel<12> mSecond;   ///< What is left, or the live pace without a target
    WorkoutLabel<16> mCaption;
};

#endif // WORKOUT_FACE_GAUGE_HPP
