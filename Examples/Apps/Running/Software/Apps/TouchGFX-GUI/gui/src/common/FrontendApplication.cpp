#include <gui/common/FrontendApplication.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

void FrontendApplication::gotoTrackScreenNoTransition()
{
    if (model.hasStepCard()) {
        // The card goes back to the Track screen when it is done.
        gotoTrackWorkoutStepScreenNoTransition();
    } else {
        FrontendApplicationBase::gotoTrackScreenNoTransition();
    }
}
