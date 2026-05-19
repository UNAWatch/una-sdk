#ifndef TRACKSTARTCONFIRMATIONVIEW_HPP
#define TRACKSTARTCONFIRMATIONVIEW_HPP

#include <gui_generated/trackstartconfirmation_screen/TrackStartConfirmationViewBase.hpp>
#include <gui/trackstartconfirmation_screen/TrackStartConfirmationPresenter.hpp>

class TrackStartConfirmationView : public TrackStartConfirmationViewBase
{
public:
    TrackStartConfirmationView();
    virtual ~TrackStartConfirmationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
protected:
    virtual void handleKeyEvent(uint8_t key) override;
};

#endif // TRACKSTARTCONFIRMATIONVIEW_HPP
