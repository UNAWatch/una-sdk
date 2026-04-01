
#ifndef __APP_SENSOR_LISTENER_HPP
#define __APP_SENSOR_LISTENER_HPP

#include <cstdint>
#include <cstddef>
#include <memory>

#include "SDK/Simulator/App/DualAppComm.hpp"
#include "SDK/Interfaces/ISensorDataListener.hpp"

namespace App
{

class SensorListener : public SDK::Interface::ISensorDataListener
{
public:
    SensorListener(SDK::App::DualAppComm& comm, bool isService);
    ~SensorListener() override = default;

    void onSdlNewData(uint16_t                 handle,
                      const SDK::Sensor::Data* base,
                      uint16_t                 count,
                      uint16_t                 stride) override;

private:
    bool sendMessage(SDK::MessageBase* msg);

    SDK::App::DualAppComm& mComm;
    const bool        mIsService;
};

} // namespace App

#endif // __APP_SENSOR_LISTENER_HPP
