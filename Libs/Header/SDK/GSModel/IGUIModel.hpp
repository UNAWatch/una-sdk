#ifndef __IMODEL_HPP__
#define __IMODEL_HPP__

#include "SDK/Interfaces/IKernel.hpp"
#include <variant>
#include <string>
#include <stdint.h>

// Templated interface:
//  G2S — тип подій GUI->Service (те, що надсилаємо в sendToService)
//  GH  — тип обробника подій на GUI-боці (handler інтерфейс)
template<typename G2S, typename GH>
class IGUIModel {
public:
    IGUIModel();
    virtual ~IGUIModel();

    void setGUIHandler(const IKernel* kernel, GH* handler);

    // Обробка черги Service->GUI (S2G) — сигнатура лишається віртуальною
    virtual void checkS2GEvents() = 0;

    // Надсилання даних GUI->Service (G2S) — тепер тип параметра шаблонний
    virtual bool sendToService(const G2S& data) = 0;

protected:
    GH*            mGUIHandler;   // шаблонний тип хендлера
    const IKernel* mGUIKernel;    // ядро GUI-шару
};

// ---------- Implementation (must stay in header for templates) ----------

template<typename G2S, typename GH>
IGUIModel<G2S, GH>::IGUIModel()
    : mGUIHandler(nullptr)
    , mGUIKernel(nullptr)
{}

template<typename G2S, typename GH>
IGUIModel<G2S, GH>::~IGUIModel() = default;

template<typename G2S, typename GH>
void IGUIModel<G2S, GH>::setGUIHandler(const IKernel* kernel, GH* handler)
{
    mGUIKernel  = kernel;
    mGUIHandler = handler;
}

#endif // __IMODEL_HPP__

