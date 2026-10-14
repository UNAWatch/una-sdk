#ifndef WORKOUTMENUPRESENTER_HPP
#define WORKOUTMENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class WorkoutMenuView;

class WorkoutMenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    WorkoutMenuPresenter(WorkoutMenuView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~WorkoutMenuPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

    /// Arm the chosen workout for the next run.
    void start();
    /// Show the chosen workout in full, coming back here.
    void showDetails();

private:
    WorkoutMenuPresenter();

    WorkoutMenuView& view;
};

#endif // WORKOUTMENUPRESENTER_HPP
