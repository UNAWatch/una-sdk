#ifndef CALIBRATIONCLEAREDVIEW_HPP
#define CALIBRATIONCLEAREDVIEW_HPP

#include <gui_generated/calibrationcleared_screen/CalibrationClearedViewBase.hpp>
#include <gui/calibrationcleared_screen/CalibrationClearedPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <SDK/GUI/CountdownTimer.hpp>

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

    SDK::GUI::CountdownTimer                  mDismissTimer;
    touchgfx::Callback<CalibrationClearedView> mDismissCb;
};

#endif // CALIBRATIONCLEAREDVIEW_HPP
