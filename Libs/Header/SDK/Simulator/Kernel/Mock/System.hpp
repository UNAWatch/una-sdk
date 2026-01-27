
#pragma once

#include "SDK/Interfaces/ISystem.hpp"
#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include "SDK/Simulator/OS/OS.hpp"

namespace SDK::Simulator::Mock
{

    class SystemGUI : public SDK::Interface::ISystem {
    public:

        SystemGUI()          = default;
        virtual ~SystemGUI() = default;

        bool isAppRunning() const;

        void     exit(int status = 0) override;
        uint32_t getTimeMs()          override;
        void     delay(uint32_t ms)   override;
        void     yield()              override;

	private:
		bool mAppRunning = true;
    };

    class SystemService : public SDK::Interface::ISystem
    {
    public:
        SystemService()          = default;
        virtual ~SystemService() = default;

        bool isAppRunning() const;

        void     exit(int status = 0) override;
        uint32_t getTimeMs()          override;
        void     delay(uint32_t ms)   override;
        void     yield()              override;

    private:
        bool mAppRunning = true;
    };

} // namespace SDK::Simulator::Mock