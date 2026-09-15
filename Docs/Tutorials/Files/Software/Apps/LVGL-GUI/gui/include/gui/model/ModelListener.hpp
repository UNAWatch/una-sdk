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

    /// The service's current settings, in answer to a request or after a save.
    virtual void onSettingsUpdate(float decimalCounter, CustomMessage::ActivityType activityType,
                                  CustomMessage::DisplayMode displayMode)
    {
        (void)decimalCounter; (void)activityType; (void)displayMode;
    }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
