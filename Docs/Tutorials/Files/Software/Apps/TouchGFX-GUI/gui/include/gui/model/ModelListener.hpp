#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>

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

    virtual void onSettingsUpdate(float decimalCounter, CustomMessage::ActivityType activityType, CustomMessage::DisplayMode displayMode) {}

protected:
    Model* model;

    
};

#endif // MODELLISTENER_HPP
