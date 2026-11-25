#include <gui/common/FrontendApplication.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

void FrontendApplication::gotoTrackScreenNoTransition()
{
    if (!gotoWaitingPopup()) {
        FrontendApplicationBase::gotoTrackScreenNoTransition();
    }
}

bool FrontendApplication::gotoWaitingPopup()
{
    // Each goes back to the Track screen when it is done, which opens the
    // next one waiting.
    if (model.hasStepCard()) {
        gotoTrackWorkoutStepScreenNoTransition();
        return true;
    }
    switch (model.waitingPopup()) {
    case Model::Popup::Lap:
        gotoTrackLapScreenNoTransition();
        return true;
    case Model::Popup::Phase:
        gotoTrackIntervalsAlertScreenNoTransition();
        return true;
    case Model::Popup::RunAlert:
        gotoTrackRunAlertScreenNoTransition();
        return true;
    case Model::Popup::Completed:
        gotoTrackIntervalsWorkoutCompletedScreenNoTransition();
        return true;
    default:
        return false;
    }
}
