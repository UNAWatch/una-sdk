
#pragma once

#include "SDK/Interfaces/ISystem.hpp"
#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include "SDK/Simulator/OS/OS.hpp"

namespace SDK::Simulator::Mock
{

class System : public SDK::Interface::ISystem {
public:

    System() {}
    virtual ~System() = default;


    virtual void exit(int status = 0) override
    {
        static_cast<touchgfx::HALSDL2*>(touchgfx::HAL::getInstance())->stopApplication();
    }

    virtual uint32_t getTimeMs() override
    {
        return static_cast<uint32_t>(GetTickCount64());
    }

    virtual void delay(uint32_t ms) override
    {
        Sleep(ms);
    }

    virtual void yield() override {}
};

} // namespace SDK::Simulator::Mock