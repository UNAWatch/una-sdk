#include <gui/imageprop_screen/ImagePropView.hpp>
#include <gui/imageprop_screen/ImagePropPresenter.hpp>

ImagePropPresenter::ImagePropPresenter(ImagePropView& v)
    : view(v)
{

}

void ImagePropPresenter::activate()
{

}

void ImagePropPresenter::deactivate()
{

}

void ImagePropPresenter::gotoImageMenu()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}

void ImagePropPresenter::back()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}
