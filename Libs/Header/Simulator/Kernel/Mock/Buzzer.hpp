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

#include "API/Buzzer.hpp"

namespace Mock 
{

/**
 * @brief Stub implementation of Interface::IBuzzer.
 */
class Buzzer : public sdk::api::Buzzer {
public:
    bool play()                                     override;
    bool play(Note note)                            override;
    bool play(const Note melody[], uint8_t len)     override;
    bool isPlaying()                                override;
    void stop()                                     override;
};

} // namespace Mock
