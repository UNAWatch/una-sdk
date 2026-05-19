#ifndef TRACKDISCARDCONFIRMATIONVIEW_HPP
#define TRACKDISCARDCONFIRMATIONVIEW_HPP

#include <gui_generated/trackdiscardconfirmation_screen/TrackDiscardConfirmationViewBase.hpp>
#include <gui/trackdiscardconfirmation_screen/TrackDiscardConfirmationPresenter.hpp>

class TrackDiscardConfirmationView : public TrackDiscardConfirmationViewBase
{
public:
    TrackDiscardConfirmationView();
    virtual ~TrackDiscardConfirmationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKDISCARDCONFIRMATIONVIEW_HPP
