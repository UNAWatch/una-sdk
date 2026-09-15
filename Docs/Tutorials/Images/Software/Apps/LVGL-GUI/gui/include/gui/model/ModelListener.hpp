/**
 ******************************************************************************
 * @file    ModelListener.hpp
 * @brief   Events the Model raises towards the active screen.
 *
 * Images's model reports only the kernel's frame tick, which drives the
 * jump; later tutorials add methods here (sensor values, file results)
 * that the screen overrides.
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

    /// One call per kernel frame (10 Hz on the watch): the TouchGFX
    /// handleTickEvent(), delivered through the model.
    virtual void onFrame() {}

protected:
    Model* model;
};

#endif // MODEL_LISTENER_HPP
