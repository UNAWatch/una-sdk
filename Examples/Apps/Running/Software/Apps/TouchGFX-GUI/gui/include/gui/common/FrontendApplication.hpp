#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

class FrontendHeap;

#if defined(SIMULATOR)
#include "SDK/Simulator/Kernel/Kernel.hpp"
#include "SDK/Simulator/Components/InstanceSensorLayer.hpp"
#endif

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent()
    {
#if defined(SIMULATOR)
        SDK::Simulator::KernelHolder::Get().tick();
#endif
        SDK::TouchGFXCommandProcessor::GetInstance().callCustomMessageHandler();
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

    // Get keys event in Model to simulate backend actions 
    void handleKeyEvent(uint8_t key)
    {
#if defined(SIMULATOR)
        Instance::SensorLayer::getInstance().handlerButtons(key);
        if (SDK::Simulator::KernelHolder::Get().keyFilter(key)) {
            model.handleKeyEvent(key);
            FrontendApplicationBase::handleKeyEvent(key);
        }
#else
        model.handleKeyEvent(key);
        FrontendApplicationBase::handleKeyEvent(key);
#endif
    }

    /// The Track screen, or the step card first if a step started while
    /// another screen showed: the Track screen would otherwise draw the new
    /// step for a frame before the card replaced it.
    void gotoTrackScreenNoTransition();

private:
};

#endif // FRONTENDAPPLICATION_HPP
