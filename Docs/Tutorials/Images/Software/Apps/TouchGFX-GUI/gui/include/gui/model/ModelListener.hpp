#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <vector>
#include <string>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    virtual void onIdleTimeout() {}

    virtual void updateImageList(const std::vector<std::string>& filenames) {}
    virtual void updateImageLoaded(const std::string& filename, bool success) {}
    virtual void updateImageMetadata(const std::string& filename, uint32_t width, uint32_t height, uint32_t fileSize, const std::string& lastModified, uint32_t renderTimeMs) {}

protected:
    Model* model;

    
};

#endif // MODELLISTENER_HPP
