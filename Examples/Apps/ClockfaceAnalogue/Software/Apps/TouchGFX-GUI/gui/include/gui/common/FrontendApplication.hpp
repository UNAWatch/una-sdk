#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include "SDK/Port/TouchGFX/TouchGFXCommandProcessor.hpp"

class FrontendHeap;

#if defined(SIMULATOR)
#include "SDK/Simulator/Kernel/Kernel.hpp"
#endif

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    /**
     * @brief One TouchGFX tick, which is one frame the kernel granted.
     *
     * The frame itself is paced by OSWrappers::waitForVSync blocking on the
     * kernel's EVENT_GUI_TICK; what happens here is draining the app's own
     * queue, which is how anything the service sends reaches the Model on the
     * GUI thread.
     */
    virtual void handleTickEvent()
    {
#if defined(SIMULATOR)
        SDK::Simulator::KernelHolder::Get().tick();
#endif
        SDK::TouchGFXCommandProcessor::GetInstance().callCustomMessageHandler();
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

private:
};

#endif // FRONTENDAPPLICATION_HPP
