/**
 ******************************************************************************
 * @file    VibroStub.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Vibro motor interface stub.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#pragma once

#include "IVibro.hpp"

namespace Stub {

/**
 * @brief Stub implementation of Interface::IVibro.
 */
class Vibro : public Interface::IVibro {
public:
    bool play(uint8_t effect = Effect_t::STRONG_CLICK_100) override;
    bool play(Note_t melody[], uint8_t len)                override;
    bool stop()                                            override;
    bool isPlaying()                                       override;
};

} // namespace Stub
