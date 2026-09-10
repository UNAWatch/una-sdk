#ifndef MAINPRESENTER_HPP
#define MAINPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class MainView;

class MainPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    MainPresenter(MainView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~MainPresenter() {}

    WallTime currentTime() const { return model->currentTime(); }

    uint32_t steps() const { return model->steps(); }

    bool is12h() const { return model->is12h(); }

    // ModelListener
    virtual void onTime(const WallTime &time) override;
    virtual void onSteps(uint32_t steps) override;
    virtual void onClockFormat(bool is12h) override;

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
