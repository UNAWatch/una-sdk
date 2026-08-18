#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>
#include <gui/common/FrontendApplication.hpp>

#include "Commands.hpp"

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

    /// A new navigation state arrived from the service.
    virtual void onNavUpdate(const CustomMessage::NavState& /*nav*/) {}

    /// The outcome of a "save the current position" request.
    virtual void onTargetSaved(bool /*saved*/, float /*latitude*/,
                               float /*longitude*/) {}

protected:
    Model* model;


};

#endif // MODELLISTENER_HPP
