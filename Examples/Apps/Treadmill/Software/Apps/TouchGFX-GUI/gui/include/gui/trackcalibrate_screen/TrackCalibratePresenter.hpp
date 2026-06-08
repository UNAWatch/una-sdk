#ifndef TRACKCALIBRATEPRESENTER_HPP
#define TRACKCALIBRATEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class TrackCalibrateView;

/**
 * Post-run "Calibrate & Save" (§5): lets the user enter the actual treadmill
 * console distance, or skip and keep the estimate. The value is forwarded to
 * the Service, which folds it into the FIT distance and (phase 2) the delta LUT.
 */
class TrackCalibratePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    TrackCalibratePresenter(TrackCalibrateView& v);

    virtual void activate();
    virtual void deactivate();

    virtual ~TrackCalibratePresenter() {}

    /// Apply the user-entered actual distance (metres).
    void applyCalibration(float meters);
    /// Keep the estimated distance (no correction).
    void skipCalibration();

private:
    TrackCalibratePresenter();

    TrackCalibrateView& view;
};

#endif // TRACKCALIBRATEPRESENTER_HPP
