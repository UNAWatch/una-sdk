/**
 ******************************************************************************
 * @file    BuzzerStub.hpp
 * @date    14-July-2025
 * @author  Oleksandr Tymoshenko <oleksandr.tymoshenko@droid-technologies.com>
 * @brief   Buzzer interface stub.
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

 #pragma once

#include "IBuzzer.hpp"

namespace Stub {

/**
 * @brief Stub implementation of Interface::IBuzzer.
 */
class Buzzer : public Interface::IBuzzer {
public:
    bool play()                                     override;
    bool play(Note_t note)                          override;
    bool play(const Note_t melody[], uint32_t len)  override;
    bool isPlaying()                                override;
    void stop()                                     override;
};

} // namespace Stub
