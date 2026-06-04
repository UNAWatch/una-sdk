#ifndef TRACKCALIBRATEINTROPRESENTER_HPP
#define TRACKCALIBRATEINTROPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackCalibrateIntroView;

class TrackCalibrateIntroPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackCalibrateIntroPresenter(TrackCalibrateIntroView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~TrackCalibrateIntroPresenter() {}

private:
    TrackCalibrateIntroPresenter();

    TrackCalibrateIntroView& view;
};

#endif // TRACKCALIBRATEINTROPRESENTER_HPP
