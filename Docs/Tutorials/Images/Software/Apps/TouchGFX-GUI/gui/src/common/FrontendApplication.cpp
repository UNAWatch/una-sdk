#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <gui/imagelist_screen/ImageListView.hpp>
#include <gui/imagelist_screen/ImageListPresenter.hpp>
#include <gui/imageviewer_screen/ImageViewerView.hpp>
#include <gui/imageviewer_screen/ImageViewerPresenter.hpp>
#include <gui/imageprop_screen/ImagePropView.hpp>
#include <gui/imageprop_screen/ImagePropPresenter.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{

}

void FrontendApplication::gotoImageListScreenNoTransition()
{
    transitionCallback = touchgfx::Callback<FrontendApplicationBase>(this, &FrontendApplicationBase::gotoImageListScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &transitionCallback;
}

void FrontendApplication::gotoImageListScreenNoTransitionImpl()
{
    touchgfx::makeTransition<ImageListView, ImageListPresenter, touchgfx::NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoImageViewerScreenNoTransition()
{
    transitionCallback = touchgfx::Callback<FrontendApplicationBase>(this, &FrontendApplicationBase::gotoImageViewerScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &transitionCallback;
}

void FrontendApplication::gotoImageViewerScreenNoTransitionImpl()
{
    touchgfx::makeTransition<ImageViewerView, ImageViewerPresenter, touchgfx::NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoImagePropScreenNoTransition()
{
    transitionCallback = touchgfx::Callback<FrontendApplicationBase>(this, &FrontendApplicationBase::gotoImagePropScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &transitionCallback;
}

void FrontendApplication::gotoImagePropScreenNoTransitionImpl()
{
    touchgfx::makeTransition<ImagePropView, ImagePropPresenter, touchgfx::NoTransition, Model >(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
