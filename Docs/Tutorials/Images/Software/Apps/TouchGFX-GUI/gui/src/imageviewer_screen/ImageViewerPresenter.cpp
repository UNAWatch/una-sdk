#include <gui/imageviewer_screen/ImageViewerView.hpp>
#include <gui/imageviewer_screen/ImageViewerPresenter.hpp>

ImageViewerPresenter::ImageViewerPresenter(ImageViewerView& v)
    : view(v)
{

}

void ImageViewerPresenter::activate()
{

}

void ImageViewerPresenter::deactivate()
{

}

void ImageViewerPresenter::gotoImageMenu()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}

void ImageViewerPresenter::back()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}
