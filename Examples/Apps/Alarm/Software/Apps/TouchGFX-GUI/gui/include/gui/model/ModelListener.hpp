#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>
#include <vector>

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

    virtual void onAlarmListUpdated(const std::vector<Alarm>& list) {}

    virtual void onAlarmActivated(const Alarm& alarm) {
        // Switch to Alarm screen from any other screen of this app
        model->application().gotoRingingScreenNoTransition();
    }

protected:
    Model* model;

    
};

#endif // MODELLISTENER_HPP
