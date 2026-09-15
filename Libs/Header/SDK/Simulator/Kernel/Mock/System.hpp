
#pragma once

#include "SDK/Interfaces/ISystem.hpp"
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

        /**
         * @brief   Register the function exit() calls to end the GUI main loop.
         *
         * The TouchGFX simulators leave this unset and exit() stops the TouchGFX
         * HAL directly. A simulator built on another toolkit registers the
         * function that ends its own loop (see the LVGL simulator host).
         */
        static void SetStopHandler(void (*handler)());

        void     exit(int status = 0) override;
        uint32_t getTimeMs()          override;
        void     delay(uint32_t ms)   override;
        void     yield()              override;

	private:
        static bool mAppRunning;
        static void (*mStopHandler)();
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