#ifndef CALIBRATIONDATAVIEW_HPP
#define CALIBRATIONDATAVIEW_HPP

#include <gui_generated/calibrationdata_screen/CalibrationDataViewBase.hpp>
#include <gui/calibrationdata_screen/CalibrationDataPresenter.hpp>
#include <gui/model/Model.hpp>

/**
 * Calibration "View Data": a scrollable per-bin table. Each row is a cadence
 * range (e.g. "80 - 84 SPM") and a fill-toward-validity percentage, colour-coded
 * 100% green / 1-99% amber / 0% red. L1/L2 page the list; R2 returns.
 */
class CalibrationDataView : public CalibrationDataViewBase
{
public:
    CalibrationDataView() {}
    virtual ~CalibrationDataView() {}
    virtual void setupScreen();

    /// Seed the table from the Model's calibration snapshot.
    void setData(const Model::CalibrationView& data);

protected:
    static constexpr int16_t kVisibleRows = 5;   ///< Rows visible in the viewport
    static constexpr int16_t kPage        = 4;   ///< Rows advanced per L1/L2 press

    Model::CalibrationView mData {};
    int16_t                mTop = 0;             ///< First visible bin index

    virtual void scrollListUpdateItem(BinListItem& item, int16_t itemIndex) override;
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // CALIBRATIONDATAVIEW_HPP
