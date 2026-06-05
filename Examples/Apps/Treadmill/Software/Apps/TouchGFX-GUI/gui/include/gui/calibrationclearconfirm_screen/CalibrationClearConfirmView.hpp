#ifndef CALIBRATIONCLEARCONFIRMVIEW_HPP
#define CALIBRATIONCLEARCONFIRMVIEW_HPP

#include <gui_generated/calibrationclearconfirm_screen/CalibrationClearConfirmViewBase.hpp>
#include <gui/calibrationclearconfirm_screen/CalibrationClearConfirmPresenter.hpp>

/**
 * Confirmation before clearing calibration data. R1 (amber) clears and advances
 * to the "Cleared" screen; R2 (white) cancels back to the Calibration menu.
 */
class CalibrationClearConfirmView : public CalibrationClearConfirmViewBase
{
public:
    CalibrationClearConfirmView() {}
    virtual ~CalibrationClearConfirmView() {}
    virtual void setupScreen();

protected:
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // CALIBRATIONCLEARCONFIRMVIEW_HPP
