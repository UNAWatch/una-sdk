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

    /** @brief The countdown state changed (start / pause / resume / reset / stop). */
    virtual void onStateChanged() {}

    /** @brief The recents list changed. */
    virtual void onRecentsChanged(const std::vector<Timer>& list) {}

    /** @brief The countdown reached zero. Jump to the Fired screen by default. */
    virtual void onFired(const Timer& timer)
    {
        model->application().gotoFiredScreenNoTransition();
    }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
