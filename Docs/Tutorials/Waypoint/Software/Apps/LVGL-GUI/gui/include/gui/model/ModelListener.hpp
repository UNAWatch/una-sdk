/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

#include "Commands.hpp"

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

    /// A new navigation state arrived from the service.
    virtual void onNavUpdate(const CustomMessage::NavState& nav) { (void)nav; }

    /// The outcome of a "save the current position" request.
    virtual void onTargetSaved(CustomMessage::SaveOutcome outcome, float latitude, float longitude)
    {
        (void)outcome; (void)latitude; (void)longitude;
    }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
