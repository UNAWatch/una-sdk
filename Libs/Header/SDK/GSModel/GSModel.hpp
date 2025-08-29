#ifndef __GSMODEL_HPP
#define __GSMODEL_HPP

#include "SDK/GSModel/IGUIModel.hpp"
#include "SDK/CircularBuffer.hpp"

#include <type_traits>
#include <variant>

// Шаблон класу, але дозволені тільки ці 2 типи
template<typename S2G, typename G2S, typename SH, typename GH>
class GSModel : public IGUIModel<G2S, GH>
{
public:
    GSModel(const IKernel& kernel, SH& handler)
        : mServiceKernel(kernel)
        , mS2GQueue(kernel)
        , mG2SQueue(kernel)
        , mServiceHandler(handler)
    {
        mS2GQueue.init();
        mG2SQueue.init();
    }

    ~GSModel() override = default;

    // GUI -> Service
    void checkG2SEvents()
    {
        G2S data{};

        mServiceKernel.app.unLock();
        bool status = mG2SQueue.pop(data, 1000);
        mServiceKernel.app.lock();

        if (!status) return;

        std::visit([this](const auto& event) {
            mServiceHandler.handleEvent(event);
        }, data);
    }

    bool sendToService(const G2S& data) override
    {
        return mG2SQueue.push(data);
    }

    // Service -> GUI
    void checkS2GEvents() override
    {
        S2G data{};

        this->mGUIKernel->app.unLock();
        bool status = mS2GQueue.pop(data, 1000);
        this->mGUIKernel->app.lock();

        if (!status) return;

        if (this->mGUIHandler) {
            std::visit([this](const auto& event) {
                this->mGUIHandler->handleEvent(event);
            }, data);
        }
    }

    bool sendToGUI(const S2G& data)
    {
        return mS2GQueue.push(data);
    }

private:
    const IKernel&          mServiceKernel;
    CircularBuffer<S2G, 20> mS2GQueue;
    CircularBuffer<G2S, 20> mG2SQueue;
    SH&                     mServiceHandler;
};

#endif // __MODEL_HPP
