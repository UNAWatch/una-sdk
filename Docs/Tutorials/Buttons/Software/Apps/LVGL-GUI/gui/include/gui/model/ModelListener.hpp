/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 *
 * Buttons' model has nothing to report; later tutorials add virtual methods
 * here (sensor values, file results) that the screen overrides.
 ******************************************************************************
 */

#ifndef MODEL_LISTENER_HPP
#define MODEL_LISTENER_HPP

class Model;

class ModelListener
{
public:
    ModelListener() : model(nullptr) {}
    virtual ~ModelListener() = default;

    void bind(Model* m) { model = m; }

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
