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

    /**
     * @brief The service published a new stopwatch state.
     * @param state The new state.
     */
    virtual void onStopwatchChanged(const Stopwatch::State &state) { (void)state; }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
