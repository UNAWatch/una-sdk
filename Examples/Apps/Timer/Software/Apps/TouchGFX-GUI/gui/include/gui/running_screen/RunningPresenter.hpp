#ifndef RUNNINGPRESENTER_HPP
#define RUNNINGPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class RunningView;

/**
 * @brief Presenter for the Running screen -- skeleton placeholder.
 */
class RunningPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    RunningPresenter(RunningView& v);

    virtual void activate();
    virtual void deactivate();
    virtual ~RunningPresenter() {}

private:
    RunningPresenter();

    RunningView& view;
};

#endif // RUNNINGPRESENTER_HPP
