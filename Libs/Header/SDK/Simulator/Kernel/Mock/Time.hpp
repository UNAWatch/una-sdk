
#pragma once

#include "SDK/Interfaces/ITime.hpp"

namespace SDK::Simulator::Mock
{

class Time : public SDK::Interface::ITime {
public:

    Time() = default;
    virtual ~Time() = default;

private:

};

} // namespace SDK::Simulator::Mock