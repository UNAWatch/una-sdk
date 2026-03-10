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

    virtual void updateHR(float hr, float tl) override;

    virtual void updateGPS(float lat, float lon, float alt) override;
    virtual void updateElevation(float elevation) override;
    virtual void updateAccelerometer(float x, float y, float z) override;
    virtual void updateStepCounter(uint32_t steps) override;
    virtual void updateFloorCounter(uint32_t floors) override;
    virtual void updateCompass(float heading) override;
    virtual void updateRTC(uint32_t time) override;
    virtual void updateStats(float serviceCpu, float guiCpu, float txMsgPerSec, float rxMsgPerSec, float txBytesPerSec, float rxBytesPerSec) override;
    virtual void updateBattery(float level) override;

    void exit() {
        model->exitApp();
    }

private:
    MainPresenter();

    MainView& view;
};

#endif // MAINPRESENTER_HPP
