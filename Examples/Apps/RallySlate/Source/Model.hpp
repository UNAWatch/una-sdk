#pragma once
#include "../Score.hpp"
#include "SDK/Interfaces/IGuiLifeCycleCallback.hpp"
#include "SDK/Kernel/Kernel.hpp"

class Model : public SDK::Interface::IGuiLifeCycleCallback {
public:
    Model();
    Rally::Score score;
    bool corrupt = false;
    bool recovered = false;
    bool dirty = false;
    bool hasSave() const { return mSlot >= 0; }
    bool add(unsigned side);
    bool undo();
    void reset(uint8_t target);
    void exitApp();
    void bind(void (*refresh)(void*), void* context) { mRefresh = refresh; mContext = context; }
private:
    const SDK::Kernel& mKernel;
    uint32_t mSerial = 0;
    int mSlot = -1;
    void (*mRefresh)(void*) = nullptr;
    void* mContext = nullptr;
    void load();
    void save();
    void onStart() override {}
    void onResume() override { if (mRefresh) mRefresh(mContext); }
    void onSuspend() override {}
    void onStop() override {}
};
