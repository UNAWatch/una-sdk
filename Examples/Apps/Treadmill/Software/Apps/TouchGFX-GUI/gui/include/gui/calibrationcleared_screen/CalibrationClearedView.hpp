#ifndef CALIBRATIONCLEAREDVIEW_HPP
#define CALIBRATIONCLEAREDVIEW_HPP

#include <gui_generated/calibrationcleared_screen/CalibrationClearedViewBase.hpp>
#include <gui/calibrationcleared_screen/CalibrationClearedPresenter.hpp>
#include <gui/containers/CountdownTimer.hpp>
#include <touchgfx/Callback.hpp>

/**
 * Brief "Cleared" acknowledgement after clearing calibration data; auto-dismisses
 * back to the Calibration menu.
 */
class CalibrationClearedView : public CalibrationClearedViewBase
{
public:
    CalibrationClearedView();
    virtual ~CalibrationClearedView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

private:
    void onDismiss();

    CountdownTimer                            mDismissTimer;
    touchgfx::Callback<CalibrationClearedView> mDismissCb;
};

#endif // CALIBRATIONCLEAREDVIEW_HPP
