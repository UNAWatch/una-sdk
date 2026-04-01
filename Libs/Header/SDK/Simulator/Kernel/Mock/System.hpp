
#pragma once

#include "SDK/Interfaces/ISystem.hpp"
#include <platform/hal/simulator/sdl2/HALSDL2.hpp>
#include "SDK/Simulator/OS/OS.hpp"
#include <chrono>

namespace SDK::Simulator::Mock
{
    class System {
    public:
        /**
        * @brief   Get milliseconds form system start.
        * @return  Timestamp in milliseconds.
        */
        static uint32_t GetTimeMs();
    private:
        /**
         * @brief Constructor.
         */
        System();

        /**
         * @brief Destructor.
         */
        virtual ~System() = default;
    };

    class SystemGUI : public SDK::Interface::ISystem {
    public:

        SystemGUI()          = default;
        virtual ~SystemGUI() = default;

        static bool isAppRunning();

        void     exit(int status = 0) override;
        uint32_t getTimeMs()          override;
        void     delay(uint32_t ms)   override;
        void     yield()              override;

	private:
        static bool mAppRunning;
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