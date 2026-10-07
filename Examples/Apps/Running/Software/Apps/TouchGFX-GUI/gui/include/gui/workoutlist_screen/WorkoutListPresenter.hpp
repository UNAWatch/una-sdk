#ifndef WORKOUTLISTPRESENTER_HPP
#define WORKOUTLISTPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class WorkoutListView;

class WorkoutListPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    WorkoutListPresenter(WorkoutListView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~WorkoutListPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }
    virtual void onWorkoutList() override;

    void chooseWorkout(uint16_t index);
    /// The Free run row: no workout for the next run.
    void clearWorkout();

private:
    WorkoutListPresenter();

    WorkoutListView& view;
};

#endif // WORKOUTLISTPRESENTER_HPP
