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

    virtual void onTimerListUpdated(const std::vector<Timer>& list) {}

    virtual void onTimerActivated(const Timer& timer) {
        // Switch to Timer screen from any other screen of this app
        model->application().gotoFiredScreenNoTransition();
    }

protected:
    Model* model;

    
};

#endif // MODELLISTENER_HPP
