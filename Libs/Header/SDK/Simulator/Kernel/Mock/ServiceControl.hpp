/**
 ******************************************************************************
 * @file    ServiceControl.hpp
 * @date    18-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Mock for IServiceControl interface.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "SDK/Interfaces/IAppControl.hpp"

namespace SDK::Simulator::Mock
{

class ServiceControl : public SDK::Interface::IServiceControl,
                           public SDK::Interface::IGUIControl
{
public:
    ServiceControl() : mContext()
    {}

    virtual ~ServiceControl() = default;

    virtual bool runGUI() override
    {
        return static_cast<bool>(mContext);
    }

    virtual void setContext(std::shared_ptr<void> context) override
    {
        mContext = context;
    }

    virtual std::shared_ptr<void> getContext() override
    {
        return mContext;
    }

private:
    std::shared_ptr<void> mContext;
};

} // namespace SDK::Simulator::Mock
