#include <gui/imagelist_screen/ImageListView.hpp>
#include <gui/imagelist_screen/ImageListPresenter.hpp>
#include <gui/model/AppTypes.hpp>

ImageListPresenter::ImageListPresenter(ImageListView& v)
    : view(v)
{

}

void ImageListPresenter::activate()
{
    // Request image list from backend
    AppType::G2BEvent::RequestImageList request;
    model->sendEventToBackend(request);
}

void ImageListPresenter::deactivate()
{

}

void ImageListPresenter::updateImageList(const std::vector<std::string>& filenames)
{
    view.updateImageList(filenames);
}

void ImageListPresenter::gotoImageMenu()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}

void ImageListPresenter::back()
{
    static_cast<FrontendApplication*>(Application::getInstance())->gotoImageMenuScreenNoTransition();
}
