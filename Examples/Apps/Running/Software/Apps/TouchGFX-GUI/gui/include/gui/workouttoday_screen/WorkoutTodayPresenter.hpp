#ifndef WORKOUTTODAYPRESENTER_HPP
#define WORKOUTTODAYPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class WorkoutTodayView;

class WorkoutTodayPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    WorkoutTodayPresenter(WorkoutTodayView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~WorkoutTodayPresenter() {}

    virtual void onIdleTimeout() override { model->exitApp(); }

    /// Arm today's workout for the next run.
    void start();
    /// Show today's workout in full, coming back here.
    void showDetails();

private:
    WorkoutTodayPresenter();

    WorkoutTodayView& view;
};

#endif // WORKOUTTODAYPRESENTER_HPP
