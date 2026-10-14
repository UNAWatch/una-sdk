#ifndef WORKOUTDETAILSPRESENTER_HPP
#define WORKOUTDETAILSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class WorkoutDetailsView;

class WorkoutDetailsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    WorkoutDetailsPresenter(WorkoutDetailsView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~WorkoutDetailsPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onWorkoutDetails() override;

    /// Back to where Details was opened from.
    void back();

private:
    WorkoutDetailsPresenter();

    WorkoutDetailsView& view;
};

#endif // WORKOUTDETAILSPRESENTER_HPP
