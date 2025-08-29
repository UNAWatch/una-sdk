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

#include "API/Vibro.hpp"

namespace Mock {

/**
 * @brief Stub implementation of Interface::IVibro.
 */
class Vibro : public sdk::api::Vibro {
public:
    void play(uint8_t effect = Effect::STRONG_CLICK_100) override;
    void play(const Note melody[], uint8_t len, uint8_t loop = 0)  override;
    bool isPlaying()                                     override;
    void stop()                                          override;
};

} // namespace Stub
