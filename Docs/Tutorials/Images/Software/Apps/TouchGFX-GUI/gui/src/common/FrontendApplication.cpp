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
