/**
 ******************************************************************************
 * @file    MockServiceControl.hpp
 * @date    18-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Mock for IServiceControl interface.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __SIMULATOR_KERNEL_SERVICE_CONTROL_HPP
#define __SIMULATOR_KERNEL_SERVICE_CONTROL_HPP

#include "Interfaces/IUserAppControl.hpp"

namespace Simulator {

class MockServiceControl : public Interface::IServiceControl,
                           public Interface::IGUIControl
{
public:
    MockServiceControl() : mContext()
    {}

    virtual ~MockServiceControl() = default;

    bool runGUI(std::shared_ptr<void> context) override
    {
        mContext = context;

        return static_cast<bool>(mContext);
    }

    void setContext(std::shared_ptr<void> context) override
    {
        mContext = context;
    }

    virtual std::shared_ptr<void> getContext()
    {
        return mContext;
    }

private:
    std::shared_ptr<void> mContext;
};

} /* namespace Simulator */

#endif /* __SIMULATOR_KERNEL_SERVICE_CONTROL_HPP */